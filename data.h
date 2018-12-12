//
// Created by Christoph Anneser on 11.12.18.
//

#include <string>
#include <sstream>
#include <s2/s2point.h>
#include <s2/s2loop.h>
#include <s2/s2contains_point_query.h>

using regionMapType = std::map<std::string, std::unique_ptr<S2Loop>>;

class data {
public:
    //parses the csv file that contains the neighborhoods into a map of string (name) and S2Loop (the 2D area/shape)
    static void parse_neighborhoods(std::string file,std::vector<std::string>* neighborhoods, std::vector<std::unique_ptr<S2Loop>>* loops);

    //given the loops, build and return a shape index
    static void build_shape_index(MutableS2ShapeIndex* index, std::vector<std::unique_ptr<S2Loop>> loops);

private:
    static S2Point parse_point(std::string& point);
};

