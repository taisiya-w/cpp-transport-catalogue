#include "transport_catalogue.h"
#include "geo.h"

namespace transport_catalogue {

void TransportCatalogue::AddStop(const std::string name, geo::Coordinates coords) {
    auto it = stops_point_.find(name);
    if (it != stops_point_.end()) {
        return;
    }
    
    stops_storage_.push_back({name, coords});
    const domain::Stop* ptr = &stops_storage_.back();
    stops_point_[ptr->name] = ptr;
}

void TransportCatalogue::AddBus(const std::string name, const std::vector<std::string> stop_names, bool is_roundtrip) {
    if (buses_point_.find(name) != buses_point_.end()) {
        return;
    }
    
    buses_storage_.push_back({name, stop_names, is_roundtrip});
    const domain::Bus* bus_ptr = &buses_storage_.back();
    buses_point_[bus_ptr->name] = bus_ptr;
    
    std::unordered_set<const domain::Stop*> unique_stops;
    for (const auto& stop_name : stop_names) {
        const domain::Stop* stop_ptr = FindStop(stop_name);
        if (stop_ptr && unique_stops.insert(stop_ptr).second) {
            buses_at_stop_[stop_ptr].insert(bus_ptr);
        }
    }
}

const domain::Stop* TransportCatalogue::FindStop(std::string_view name) const {
    auto it = stops_point_.find(std::string(name));
    return (it != stops_point_.end()) ? it->second : nullptr;
}

const domain::Bus* TransportCatalogue::FindBus(std::string_view name) const {
    auto it = buses_point_.find(std::string(name));
    return (it != buses_point_.end()) ? it->second : nullptr;
}

void TransportCatalogue::AddDistance(std::string_view from, std::string_view to, int distance) {
    const domain::Stop* f = FindStop(from);
    const domain::Stop* t = FindStop(to);
    if (!f || !t) return;

    road_distances_[{f, t}] = distance;
}

int TransportCatalogue::GetDistance(const domain::Stop* from, const domain::Stop* to) const {
    
    auto it = road_distances_.find({from, to});
    if (it != road_distances_.end()) {
        return it->second;
    }
    
    it = road_distances_.find({to, from});
    if (it != road_distances_.end()) {
        return it->second;
    }
    
    return 0;
}

const std::set<const domain::Bus*, domain::BusComp>* 
TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    const domain::Stop* stop = FindStop(stop_name);
    if (!stop) {
        return nullptr;
    }
    
    auto it = buses_at_stop_.find(stop);
    if (it != buses_at_stop_.end()) {
        return &it->second;
    }
    
    static const std::set<const domain::Bus*, domain::BusComp> empty;
    return &empty;
}

const std::unordered_map<std::string, const domain::Bus*>& TransportCatalogue::GetAllBuses() const {
        return buses_point_;
}

std::optional<domain::BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const {
    const domain::Bus* bus = FindBus(name);
    if (!bus || bus->stop_names.empty()) return std::nullopt;

    domain::BusInfo info;
    
    std::unordered_set<std::string_view> unique_stops;
    for (const auto& stop_name : bus->stop_names) {
        unique_stops.insert(stop_name);
    }
    info.unique_stops = static_cast<int>(unique_stops.size());
    
    if (bus->is_roundtrip) {
        info.all_stops = static_cast<int>(bus->stop_names.size());
    } else {
        info.all_stops = static_cast<int>(bus->stop_names.size() * 2 - 1);
    }
    
    std::vector<const domain::Stop*> stops;
    for (const auto& stop_name : bus->stop_names) {
        if (const domain::Stop* stop = FindStop(stop_name)) {
            stops.push_back(stop);
        } else {
            return std::nullopt;
        }
    }
    
    if (stops.size() < 2) {
        info.route_length = 0;
        info.route_curvature = 0;
        return info;
    }
    
    double geo_length = 0.0;
    info.route_length = 0;
    
    if (bus->is_roundtrip) {
        for (size_t i = 0; i < stops.size(); ++i) {
            size_t next = (i + 1) % stops.size();
            info.route_length += GetDistance(stops[i], stops[next]);
            geo_length += geo::ComputeDistance(stops[i]->coords, stops[next]->coords);
        }
    } else {
        for (size_t i = 0; i + 1 < stops.size(); ++i) {
            info.route_length += GetDistance(stops[i], stops[i + 1]);
            geo_length += geo::ComputeDistance(stops[i]->coords, stops[i + 1]->coords);
            
            info.route_length += GetDistance(stops[i + 1], stops[i]);
        }
        geo_length *= 2.0;
    }
    
    if (geo_length > 0) {
        info.route_curvature = static_cast<double>(info.route_length) / geo_length;
    } else {
        info.route_curvature = 1.0;
    }
    
    return info;
}

} // namespace transport_catalogue