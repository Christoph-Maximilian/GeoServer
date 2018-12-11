//
// Created by Christoph Anneser on 11.12.18.
//

#include "data.h"
#include <fstream>
#include <iostream>
#include <google/protobuf/map.h>
#include "s2/s2loop.h"

/* This function parses the neighborhoods and returns a unique pointer to the map that comprises the results. */
std::unique_ptr<regionMapType> data::parse_neighborhoods(std::string file){
    auto map_ptr = std::make_unique<regionMapType>();
    std::string delimiter(";");
    std::string point_delimiter(",");
    std::ifstream infile(file);
    std::string line;

    while (std::getline(infile, line))
    {
        size_t pos = 0;
        std::string neighborhood_name;
        pos = line.find(delimiter);
        neighborhood_name = line.substr(0, pos);
        line.erase(0, pos + delimiter.length());

        // process now the polygon itself
        pos = 0;
        std::string point;
        std::vector<S2Point> points;
        while ((pos = line.find(point_delimiter)) != std::string::npos) {
            point = line.substr(0, pos);
            line.erase(0, pos + delimiter.length());
            points.emplace_back(parse_point(point));
        }
        // TODO: remove last point since it is the same es the first one ?
        // points.pop_back();
        auto loop = std::make_unique<S2Loop>(points);
        map_ptr->insert(std::pair<std::string, std::unique_ptr<S2Loop>>(neighborhood_name, std::move(loop)));
    }
    return std::move(map_ptr);
}


S2Point data::parse_point(std::string& point){
    std::string lng, lat;
    size_t pos = 0;
    std::string neighborhood_name;
    pos = point.find(' ');
    lng = point.substr(0, pos);
    lat = point.erase(0, pos + 1);
    // point contains only the latitude now
    S2LatLng lat_lng = S2LatLng::FromDegrees(std::stod(lat), std::stod(lng)).Normalized();
    S2Point s2_point = lat_lng.ToPoint();
    return s2_point;
}
