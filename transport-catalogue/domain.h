#pragma once

#include <string>
#include <deque>
#include <set>
#include <vector>
#include <optional>
#include "geo.h"

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coords;
};

struct Bus {
    std::string name;
    std::vector<std::string> stop_names;
    bool is_roundtrip = true;
};

struct BusInfo {
    size_t all_stops = 0;
    size_t unique_stops = 0;
    int route_length = 0;
    double route_curvature = 1.0;
};

struct BusComp {
    bool operator()(const Bus* lhs, const Bus* rhs) const {
        return lhs->name < rhs->name;
    }
};

} // namespace domain