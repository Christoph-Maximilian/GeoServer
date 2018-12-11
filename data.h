//
// Created by Christoph Anneser on 11.12.18.
//

#include <string>
#include <sstream>
#include <s2/s2point.h>
#include <s2/s2loop.h>

using regionMapType = std::map<std::string, std::unique_ptr<S2Loop>>;

class data {
public:
    static std::unique_ptr<regionMapType> parse_neighborhoods(std::string file);

private:
    static S2Point parse_point(std::string& point);
};

