

# Dependencies
## gRPC
- Before installing, make sure all previous _Protobuf_ versions are removed
- $ `git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc`
- $ `cd grpc`
- $ `git submodule update --init` // takes some time
- Build it from the grpc repository root
    - $ `make`
## Google S2 Geolocation Software

## protoc compilation
- `protoc -I protos/ --grpc_out=. --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin protos/test.proto`
- `protoc -I protos/ --cpp_out=. protos/test.proto`