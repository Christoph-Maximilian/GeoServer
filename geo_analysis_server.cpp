//
// Created by Christoph Anneser on 10.12.18.
//

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "geoanalysis.grpc.pb.h"
#include "geoanalysis.pb.h"
#include <s2/s2contains_point_query.h>
#include <s2/s2latlng.h>
#include "data.h"
#include "s2/s2edge_crosser.h"
#include "s2/s2shape_index.h"
#include "s2/s2shapeutil_shape_edge.h"

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

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public geoanalyser::Service {
    using PositionUserIdType = std::pair<S2CellId, std::string>;

public:
    GreeterServiceImpl(std::vector<std::string> neighborhood_names, MutableS2ShapeIndex *index_ptr) {
        _neighborhood_names = std::move(neighborhood_names);
        _index_ptr = index_ptr;
    }

    Status SetLastUserLocations(::grpc::ServerContext *context, const ::geo::UserLocationsRequest *request,
                                ::geo::StatusResponse *response) override {
        assert(request->location().size() == request->userid().size());
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

        return Status::OK;
    }

    Status GetUsersInPolygon(ServerContext *context, const PolgonRequest *request, UsersResponse *response) override {
        /*
         * 1. build a loop of the request
         * 2. create region covering of this loop -> cell union
         * 3. get min and max cell ids of the cell union
         * 4. binary search -> range from appropriate cells
         * 5. exact check necessary?
         */

        // IDEA: store all users in a b-tree, where the key is their point.
        /* Could be useful:

        CellIDsType::const_iterator i

        if (i != cell_ids.end() && i->first.range_min() <= cell_id) {
            return i->second;
        }
        if (i != cell_ids.begin() && (--i)->first.range_max() >= cell_id) {
            return i->second;
        }


         region coverer:
          S2RegionCoverer::Options options;
          options.set_max_cells(max_cells);
          options.set_max_level(max_level);
          S2RegionCoverer region_coverer(options);
          S2CellUnion union = region_coverer.GetCovering(loop);
        */
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

    std::cout << "#polyongs names: " << std::to_string(neighborhood_names.size()) << std::endl;
    std::cout << "#polyongs loops: " << std::to_string(loops.size()) << std::endl;

    MutableS2ShapeIndex index;
    data::build_shape_index(&index, &loops);

    std::cout << "Shape Index: uses " << std::to_string(index.SpaceUsed() / 1000) << "kB.";

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