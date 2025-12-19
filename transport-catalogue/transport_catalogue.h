#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <deque>
#include <optional>
#include <algorithm>

#include "domain.h"

namespace transport_catalogue {

class TransportCatalogue {
public:
    void AddStop(const std::string name, geo::Coordinates coords);
    void AddBus(const std::string name, const std::vector<std::string> stop_names, bool is_roundtrip);
    const domain::Stop* FindStop(std::string_view name) const;
    const domain::Bus* FindBus(std::string_view name) const;
    void AddDistance(std::string_view from, std::string_view to, int distance);
    int GetDistance(const domain::Stop* from, const domain::Stop* to) const;
    const std::set<const domain::Bus*, domain::BusComp>* 
    GetStopInfo(std::string_view stop_name) const;
    const std::unordered_map<std::string, const domain::Bus*>& GetAllBuses() const;
    std::optional<domain::BusInfo> GetBusInfo(std::string_view name) const;
    

private:
    std::deque<domain::Stop> stops_storage_;
    std::deque<domain::Bus> buses_storage_;
    std::unordered_map<std::string, const domain::Stop*> stops_point_;
    std::unordered_map<std::string, const domain::Bus*> buses_point_;
    mutable std::unordered_map<const domain::Stop*, std::set<const domain::Bus*, domain::BusComp>> buses_at_stop_;

    struct PairHash {
        size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& p) const {
            return std::hash<const void*>{}(p.first) ^ (std::hash<const void*>{}(p.second) << 1);
        }
    };
    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, PairHash> road_distances_;
};

} // namespace transport_catalogue