//
// Created by Christoph Anneser on 10.12.18.
//

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "geoanalysis.grpc.pb.h"
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

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public geoanalyser::Service {

public:
    GreeterServiceImpl(std::vector<std::string> neighborhood_names, MutableS2ShapeIndex* index_ptr ){
        this->_neighborhood_names = std::move(neighborhood_names);
        this->_index_ptr = index_ptr;
    }

    Status GetNeighborhoodsCount(ServerContext* context, const PointRequest* pointRequest, NeighborhoodCountsResponse* response) override {
        std::vector<S2Point > points;
        for (auto it = pointRequest->points().begin(); it != pointRequest->points().end(); it++) {
            auto point = S2LatLng::FromDegrees(it->latitude(), it->longitude()).Normalized().ToPoint();
            points.emplace_back(point);
        }

        S2ContainsPointQueryOptions options(S2VertexModel::CLOSED);
        auto query = MakeS2ContainsPointQuery(_index_ptr, options);

        uint32 hit_counter(0);
        std::vector<uint32 > counts(_neighborhood_names.size(), 0);
        for (auto point : points) {
            for (S2Shape* shape : query.GetContainingShapes(point)) {
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
    MutableS2ShapeIndex* _index_ptr;
};

void RunServer() {
    std::vector<std::string> neighborhood_names;
    std::vector<std::unique_ptr<S2Loop>> loops;

    data::parse_neighborhoods("../data/muc.csv", &neighborhood_names, &loops);
    std::vector<u_int32_t > hit_counts(neighborhood_names.size());

    std::cout << "#polyongs names: " << std::to_string(neighborhood_names.size()) << std::endl;
    std::cout << "#polyongs loops: " << std::to_string(loops.size()) << std::endl;

    MutableS2ShapeIndex index;
    data::build_shape_index(&index, &loops);

    std::cout << "Shape Index: uses " << std::to_string(index.SpaceUsed()/1000) << "kB.";

    // loops is now invalidated.
    loops.clear(); loops.shrink_to_fit();

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

int main(int argc, char** argv) {
    RunServer();
    return 0;
}