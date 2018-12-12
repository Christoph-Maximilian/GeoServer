# Installation: Dependencies
## gRPC
- Before installing, make sure all previous _Protobuf_ versions are removed
- $ `git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc`
- $ `cd grpc`
- $ `git submodule update --init` // takes some time
- Build it from the grpc repository root
    - $ `make`
- Install protobuf:
    - `cd third-party/protobuf`
    - `sudo make install`
## Google S2 Geolocation Software
- Download S2 from github:
    - $ `git clone git@github.com:google/s2geometry.git`

## manual protoc compilation
- `protoc -I protos/ --grpc_out=. --plugin=protoc-gen-grpc=../grpc/bins/opt/grpc_cpp_plugin protos/geoanalysis.proto`
- IF error occured that grpc plugin could not be found: `protoc-gen-grpc=../grpc/bins/opt/grpc_cpp_plugin` or `/usr/local/bin/grpc_cpp_plugin`
- `protoc -I protos/ --cpp_out=. protos/geoanalysis.proto`

# Docker
For now, build the docker image on your own machine.
## Build the docker image locally
### Prerequisite
- Install `docker` on your machine:
    - Debian: https://docs.docker.com/install/linux/docker-ce/ubuntu/#install-docker-ce
    - MAC: https://docs.docker.com/docker-for-mac/install/
    
- Go to the root folder of this project (`{PATH}/GeoService`)and execute
    - $ `./create_docker_container.sh`
- This downloads a fresh linux image containing the GNU compilers.
- Then the gRPC and S2 dependencies are downloaded.
- gRPC and `protoc` are installed.
- Finally, this project is copied into the container and gets compiled.
- The resulting container can be executed as follows:
    - `sudo docker run -p 50051:50051 executable:0.1 `
    - This should produce the following output:
    _Shape Index: uses 722kB.Server listening on 0.0.0.0:50051_


## _Get existing Docker image (tba)_ 
Instead of building all stuff on your local machine, you can download a docker image
    - tba.
## Transferring Docker Images without registry:
### Create Docker tar file:
- `docker save --output latestversion-1.0.0.tar dockerregistry/latestversion:1.0.0`
### Load Docker tar file:
- `docker load --input latestversion-1.0.0.tar`