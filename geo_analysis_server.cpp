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
    Status GetNeighborhoodsCount(ServerContext* context, const PointRequest* pointRequest, NeighborhoodCountsResponse* response) override {
        // TODO implement logic




        auto neighborhoodcount1 = response->add_neighborhoodcounts();
        auto neighborhoodcount2 = response->add_neighborhoodcounts();

        neighborhoodcount1->set_name("Maxvorstadt");
        neighborhoodcount2->set_name("Ludwigsvorstadt");
        neighborhoodcount1->set_count(1234);
        neighborhoodcount2->set_count(321);

        return Status::OK;
    }
};

void RunServer() {
    auto polygon_map_ptr = data::parse_neighborhoods("../data/neighborhoods_munich.csv") ;
    std::cout << "#polyongs: " << std::to_string(polygon_map_ptr->size()) << std::endl;

    std::string server_address("0.0.0.0:50051");
    GreeterServiceImpl service;

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