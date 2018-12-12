sudo docker image build  --rm -t executable:0.1  -f Dockerfile_4 .

# you can run this image:
# docker run -it --rm -p 50051:50051 --name GeoService executable:0.1