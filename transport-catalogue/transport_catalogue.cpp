#include "transport_catalogue.h"
#include "geo.h"

namespace transport_catalogue {

void TransportCatalogue::AddStop(std::string name, geo::Coordinates coords) {
    auto it = stops_point_.find(name);
    if (it != stops_point_.end()) {
        return;
    }
    
    stops_storage_.push_back({std::move(name), coords});
    const domain::Stop* ptr = &stops_storage_.back();
    stops_point_[ptr->name] = ptr;
}

void TransportCatalogue::AddBus(std::string name, std::vector<std::string> stop_names, bool is_roundtrip) {
    buses_storage_.push_back({std::move(name), std::move(stop_names), is_roundtrip});
    const domain::Bus* ptr = &buses_storage_.back();
    buses_point_[ptr->name] = ptr;
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
    if (from == to) return 0;
    
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
TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
    const domain::Stop* stop = FindStop(stop_name);
    if (!stop) {
        return nullptr;
    }
    
    if (buses_at_stop_.empty()) {
        for (const auto& bus : buses_storage_) {
            std::unordered_set<const domain::Stop*> unique_stops;
            for (const auto& name : bus.stop_names) {
                if (const domain::Stop* s = FindStop(name)) {
                    if (unique_stops.insert(s).second) {
                        buses_at_stop_[s].insert(&bus);
                    }
                }
            }
        }
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

} // namespace transport_catalogue