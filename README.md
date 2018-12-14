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
You have the choice - build the docker image on your own machine or download an existing image. Last is preferred since 
it is way faster than building it on your own machine.
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


## Get existing Docker image [preferred!]
Instead of building all stuff on your local machine, you can download a docker image
- Download `GeoService.zip` - link is provided in our **Slack Channel**.
- Unzip the file to `GeoService.tar`
- Load the image that is saved in `GeoService.tar` into your own, local Docker registry:
    - `docker load --input GeoService.tar`
- You can now mount the loaded Docker image in a fresh container:
    - `sudo docker run -p 50051:50051 executable:0.1 `
    

### Create Docker tar file:
- `docker save --output GeoService.tar dockerregistry/executable:0.1`



# Security
if this should be necessary one day (e.g. host the service online), then refer to 
    - https://github.com/grpc/grpc/issues/9593