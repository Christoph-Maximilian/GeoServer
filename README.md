

# Dependencies
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

## protoc compilation
- `protoc -I protos/ --grpc_out=. --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin protos/test.proto`
- IF error occured that grpc plugin could not be found: `protoc-gen-grpc=../grpc/bins/opt/grpc_cpp_plugin`
- `protoc -I protos/ --cpp_out=. protos/test.proto`