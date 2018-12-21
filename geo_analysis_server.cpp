//
// Created by Christoph Anneser on 10.12.18.
//

//uncomment for debug output
//#define __DEBUG

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "geoanalysis.grpc.pb.h"
#include "geoanalysis.pb.h"
#include <s2/s2contains_point_query.h>
#include <s2/s2latlng.h>
#include <s2/s2region_coverer.h>
#include <s2/s2cell_union.h>
#include "data.h"
#include "s2/s2edge_crosser.h"
#include "s2/s2shape_index.h"
#include "s2/s2shapeutil_shape_edge.h"
#include <unordered_map>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using geo::geoanalyser;
using geo::NeighborhoodCounts;
using geo::Point;
using geo::PointRequest;
using geo::NeighborhoodCountsResponse;
using geo::UserLocationsRequest;
using geo::UsersResponse;
using geo::PolgonRequest;
using geo::StatusResponse;
using geo::UserLocation;
using geo::GranularityLevel;
using geo::HeatMap;

struct pair_hash {
    auto operator () (const S2CellId &cell_id) const {
        return cell_id.id();
    }
};

// Server logic and implementation
class GreeterServiceImpl final : public geoanalyser::Service {
    using PositionUserIdType = std::pair<S2CellId, std::string>;
    using CellMap = std::unordered_map<S2CellId, uint32_t, pair_hash >;

public:
    GreeterServiceImpl(std::vector<std::string> neighborhood_names, MutableS2ShapeIndex *index_ptr) {
        _neighborhood_names = std::move(neighborhood_names);
        _index_ptr = index_ptr;
    }

    Status SetLastUserLocations(::grpc::ServerContext *context, const ::geo::UserLocationsRequest *request,
                                ::geo::StatusResponse *response) override {
        if (request->location().size() != request->userid().size()) {
            return Status::CANCELLED;
        };

        _user_location_ids.clear();

        // copy user ids into private field _user_ids
        for (auto i = 0; i < request->location().size(); i++) {
            auto s2point = S2LatLng::FromDegrees(request->location()[i].latitude(),
                                                 request->location()[i].longitude()).Normalized().ToPoint();
            S2CellId cell(s2point);
            _user_location_ids.emplace_back(std::make_pair(cell, request->userid()[i]));
        }

        //sort user locations
        std::sort(_user_location_ids.begin(), _user_location_ids.end(),
                  [](const PositionUserIdType &lhs, const PositionUserIdType &rhs) {
                      return lhs.first.id() < rhs.first.id();
                  });

        response->set_status("Latest locations set");
        return Status::OK;
    }

    Status GetUsersInPolygon(ServerContext *context, const PolgonRequest *request, UsersResponse *response) override {
        // 1. build a loop of the request's points
        std::vector<S2Point> points;
        for (const auto &point : request->vertices()) {
            auto s2point = S2LatLng::FromDegrees(point.latitude(), point.longitude()).Normalized().ToPoint();
            points.emplace_back(s2point);
        }

        S2Loop loop(points);

        // 2. create region covering of this loop -> cell union
        S2RegionCoverer::Options options;
        options.set_max_cells(200);
        options.set_max_level(30);
        S2RegionCoverer region_coverer(options);
        S2CellUnion s2cell_union = region_coverer.GetCovering(loop);

        assert (!s2cell_union.empty());

        // 3. get min and max cell ids of the cell union
        S2CellId min(s2cell_union[0].id()), max(s2cell_union[0].id());
        for (const auto &cell: s2cell_union) {
            if (min > cell.range_min()) {
                min = cell.range_min();
            }
            if (max < cell.range_max()) {
                max = cell.range_max();
            }
        }

#ifdef __DEBUG
        std::cout << "#cells in covering: " << std::to_string(s2cell_union.size()) << std::endl;
        std::cout << "min cell: " << std::to_string(min.id()) << std::endl;
        std::cout << "max cell: " << std::to_string(max.id()) << std::endl;

        for (const auto &entry: _user_location_ids) {
            std::cout << "\tuser cell: " << std::to_string(entry.first.id()) << std::endl;
        }
#endif

        // 4. binary search -> range from appropriate cells
        auto comparator = [](const PositionUserIdType &p1, const S2CellId v) {
            return p1.first.id() < v.id();
        };

        auto lower_it = std::lower_bound(_user_location_ids.begin(), _user_location_ids.end(), min, comparator);
        auto upper_it = std::lower_bound(_user_location_ids.begin(), _user_location_ids.end(), max, comparator);

        std::cout << "#elements found in binary search: " << std::to_string(std::distance(lower_it, upper_it)) << std::endl;

        for (; lower_it != upper_it; lower_it++) {
            // 5. exact check if point is in polygon, after 20-30 calls the loop builds its own shape index
            // for faster containment check
            if (loop.Contains(lower_it->first.ToPoint())) {
                UserLocation* user_location = response->add_userlocation();
                // set user id
                user_location->set_userid(lower_it->second);
                // set user location
                auto point = user_location->mutable_location();
                auto lat_long = lower_it->first.ToLatLng().Normalized();
                point->set_latitude(lat_long.lat().degrees());
                point->set_longitude(lat_long.lng().degrees());
            }
        }
        return Status::OK;
    };


    Status GetNeighborhoodsCount(ServerContext *context, const PointRequest *pointRequest,
                                 NeighborhoodCountsResponse *response) override {
        std::vector<S2Point> points;
        for (auto it = pointRequest->points().begin(); it != pointRequest->points().end(); it++) {
            auto point = S2LatLng::FromDegrees(it->latitude(), it->longitude()).Normalized().ToPoint();
            points.emplace_back(point);
        }

        S2ContainsPointQueryOptions options(S2VertexModel::CLOSED);
        auto query = MakeS2ContainsPointQuery(_index_ptr, options);

        uint32 hit_counter(0);
        std::vector<uint32> counts(_neighborhood_names.size(), 0);
        for (auto point : points) {
            for (S2Shape *shape : query.GetContainingShapes(point)) {
                counts[shape->id()]++;
                hit_counter++;
            }
        }

        std::cout << "queried points: " << std::to_string(points.size()) << std::endl;
        std::cout << "hits: " << std::to_string(hit_counter) << std::endl;

        // RESPONSE
        for (u_int16_t i = 0; i < counts.size(); i++) {
            auto neighborhood_count = response->add_neighborhoodcounts();
            neighborhood_count->set_name(_neighborhood_names[i]);
            neighborhood_count->set_count(counts[i]);
        }

        return Status::OK;
    }

    Status GetHeatmap(ServerContext* context, const GranularityLevel* request, HeatMap* response) override {
        auto level = request->level();
        CellMap cell_map;
        // 1. convert all latest location points to S2CellID and save them in a dedicated Hashtable.
        for (auto& cell_id : _user_location_ids) {
            auto parentCell = cell_id.first.parent(level);
            auto iterator = cell_map.find(parentCell);

            // 2. check if this cell already exists or if it must be newly inserted
            if (iterator != cell_map.end()) {
                // CellId already inserted
                iterator->second++;
            }
            else {
                cell_map.insert(std::make_pair(parentCell, 1));
            }
        }

        // 3. iterate through Hashtable
        for (auto it = cell_map.begin(); it != cell_map.end(); it++) {
            auto heat_map = response->add_heatmap();
            //add vertices of this cell
            for (uint8_t i = 0; i < 4; i++) {
                auto point = heat_map->add_vertices();
                S2Cell cell(it->first);
                S2LatLng lat_lng_point(cell.GetVertex(i)); //get latlong from S2Point directly
                point->set_latitude(static_cast<float>(lat_lng_point.lat().degrees()));
                point->set_longitude(static_cast<float>(lat_lng_point.lng().degrees()));
            }
            heat_map->set_count(it->second);
            heat_map->set_cellid(it->first.id());
        }

        return Status::OK;

    }
private:

    std::vector<std::string> _neighborhood_names;
    MutableS2ShapeIndex *_index_ptr;
    std::vector<std::pair<S2CellId, std::string>> _user_location_ids;
};

void RunServer() {
    std::vector<std::string> neighborhood_names;
    std::vector<std::unique_ptr<S2Loop>> loops;

    data::parse_neighborhoods("../data/muc.csv", &neighborhood_names, &loops);
    std::vector<u_int32_t> hit_counts(neighborhood_names.size());

#ifdef __DEBUG
    std::cout << "#polyongs names: " << std::to_string(neighborhood_names.size()) << std::endl;
    std::cout << "#polyongs loops: " << std::to_string(loops.size()) << std::endl;
#endif

    MutableS2ShapeIndex index;
    data::build_shape_index(&index, &loops);

    std::cout << "Shape Index: uses " << std::to_string(index.SpaceUsed() / 1000) << "kB." << std::endl;

    // loops is now invalidated.
    loops.clear();
    loops.shrink_to_fit();

    std::string server_address("0.0.0.0:50051");
    GreeterServiceImpl service(neighborhood_names, &index);

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char **argv) {
    RunServer();
    return 0;
}