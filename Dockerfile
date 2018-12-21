FROM gcc:7.3

# gRPC Dependencies
RUN set -ex; \
	apt-get update; \
	apt-get install -y --no-install-recommends build-essential autoconf libtool pkg-config\ 
	libgflags-dev libgtest-dev clang libc++-dev git;

# S2 Dependencies
RUN set -ex; \
	apt-get update; \
	apt-get install -y --no-install-recommends cmake libgoogle-glog-dev libssl-dev;


#clone gRPC
WORKDIR /usr/src/
RUN git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
WORKDIR /usr/src/grpc
RUN git submodule update --init

#make gRPC
RUN make
RUN make install

#install protoc
WORKDIR /usr/src/grpc/third_party/protobuf
RUN make install

#clone S2
WORKDIR /usr/src/
RUN git clone https://github.com/google/s2geometry.git

WORKDIR /usr/src/ServerGRPCTest
COPY . /usr/src/ServerGRPCTest

# Generate gRPC protobuf files
WORKDIR /usr/src/ServerGRPCTest
RUN ./grpc_file_generation.sh

# Make the application - build type Release
RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Run the executable
WORKDIR /usr/src/ServerGRPCTest/build
ENTRYPOINT ["./ServerGRPCTest"]
