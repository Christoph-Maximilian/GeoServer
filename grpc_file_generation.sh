protoc -I protos/ --grpc_out=. --plugin=protoc-gen-grpc=../grpc/bins/opt/grpc_cpp_plugin protos/geoanalysis.proto
protoc -I protos/ --cpp_out=. protos/geoanalysis.proto