#include "request_handler.h"
#include "transport_catalogue.h"
#include "geo.h"
#include <unordered_set>

namespace transport_catalogue {

std::optional<domain::BusInfo> RequestHandler::GetBusInfo(std::string_view name) const {
    const domain::Bus* bus = db_.FindBus(name);
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
        if (const domain::Stop* stop = db_.FindStop(stop_name)) {
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
            info.route_length += db_.GetDistance(stops[i], stops[next]);
            geo_length += geo::ComputeDistance(stops[i]->coords, stops[next]->coords);
        }
    } else {
        for (size_t i = 0; i + 1 < stops.size(); ++i) {
            info.route_length += db_.GetDistance(stops[i], stops[i + 1]);
            geo_length += geo::ComputeDistance(stops[i]->coords, stops[i + 1]->coords);
            
            info.route_length += db_.GetDistance(stops[i + 1], stops[i]);
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

const std::set<const domain::Bus*, domain::BusComp>* 
RequestHandler::GetStopInfo(std::string_view name) const {
    return db_.GetBusesForStop(name);
}

} // namespace transport_catalogue