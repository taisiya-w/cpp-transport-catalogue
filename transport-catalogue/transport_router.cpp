#include "transport_router.h"

#include <algorithm>
#include <unordered_set>

namespace transport_router {

TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& catalogue,
                                 const RoutingSettings& settings)
    : catalogue_(catalogue)
    , settings_(settings) {
    BuildGraph();
}

void TransportRouter::BuildGraph() {
    const auto& all_stops = catalogue_.GetAllStops();
    
    size_t vertex_count = all_stops.size() * 2;
    graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(vertex_count);
    
    size_t vertex_id = 0;
    for (const auto& [stop_name, stop] : all_stops) {
        stop_to_vertex_id_[stop_name] = vertex_id;
        vertex_id_to_stop_.push_back(stop_name);
        
        size_t wait_vertex = vertex_id * 2;
        size_t bus_vertex = vertex_id * 2 + 1;
        
        graph::Edge<double> wait_edge{
            wait_vertex,
            bus_vertex,
            static_cast<double>(settings_.bus_wait_time)
        };
        
        auto wait_edge_id = graph_->AddEdge(wait_edge);
        wait_edges_[wait_edge_id] = stop_name;
        
        vertex_id++;
    }
    
    const auto& all_buses = catalogue_.GetAllBuses();
    for (const auto& [name, bus] : all_buses) {
        AddEdgesForBus(bus);
    }
    
    router_ = std::make_unique<graph::Router<double>>(*graph_);
}

void TransportRouter::AddEdgesForBus(const domain::Bus* bus) {
    if (!bus || bus->stop_names.size() < 2) {
        return;
    }
    
    const auto& stops = bus->stop_names;
    
    if (bus->is_roundtrip) {
        for (size_t start = 0; start < stops.size(); ++start) {
            int accumulated_distance = 0;
            int span_count = 0;
            
            for (size_t end = start + 1; end < stops.size(); ++end) {
                const auto* prev_stop = catalogue_.FindStop(stops[end-1]);
                const auto* curr_stop = catalogue_.FindStop(stops[end]);
                
                if (!prev_stop || !curr_stop) break;
                
                accumulated_distance += catalogue_.GetDistance(prev_stop, curr_stop);
                span_count++;
                
                size_t from_vertex = GetBusVertexId(stops[start]);
                size_t to_vertex = GetWaitVertexId(stops[end]);
                
                double travel_time = ComputeBusTime(accumulated_distance);
                
                graph::Edge<double> edge{
                    from_vertex,
                    to_vertex,
                    travel_time
                };
                
                auto edge_id = graph_->AddEdge(edge);
                bus_edges_[edge_id] = {bus->name, span_count};
            }
        }
    } else {
        for (size_t start = 0; start < stops.size() - 1; ++start) {
            int accumulated_distance = 0;
            int span_count = 0;
            
            for (size_t end = start + 1; end < stops.size(); ++end) {
                const auto* prev_stop = catalogue_.FindStop(stops[end-1]);
                const auto* curr_stop = catalogue_.FindStop(stops[end]);
                
                if (!prev_stop || !curr_stop) break;
                
                accumulated_distance += catalogue_.GetDistance(prev_stop, curr_stop);
                span_count++;
                
                size_t from_vertex = GetBusVertexId(stops[start]);
                size_t to_vertex = GetWaitVertexId(stops[end]);
                
                double travel_time = ComputeBusTime(accumulated_distance);
                
                graph::Edge<double> edge{
                    from_vertex,
                    to_vertex,
                    travel_time
                };
                
                auto edge_id = graph_->AddEdge(edge);
                bus_edges_[edge_id] = {bus->name, span_count};
            }
        }
        
        for (size_t start = stops.size() - 1; start > 0; --start) {
            int accumulated_distance = 0;
            int span_count = 0;
            
            for (size_t end = start; end > 0; --end) {
                const auto* curr_stop = catalogue_.FindStop(stops[end]);
                const auto* prev_stop = catalogue_.FindStop(stops[end-1]);
                
                if (!curr_stop || !prev_stop) break;
                
                accumulated_distance += catalogue_.GetDistance(curr_stop, prev_stop);
                span_count++;
                
                size_t from_vertex = GetBusVertexId(stops[start]);
                size_t to_vertex = GetWaitVertexId(stops[end-1]);
                
                double travel_time = ComputeBusTime(accumulated_distance);
                
                graph::Edge<double> edge{
                    from_vertex,
                    to_vertex,
                    travel_time
                };
                
                auto edge_id = graph_->AddEdge(edge);
                bus_edges_[edge_id] = {bus->name, span_count};
            }
        }
    }
}

double TransportRouter::ComputeBusTime(int distance) const {
    constexpr double METERS_IN_KILOMETER = 1000.0;
    constexpr double MINUTES_IN_HOUR = 60.0;
    
    double distance_km = distance / METERS_IN_KILOMETER;
    double time_hours = distance_km / settings_.bus_velocity;
    return time_hours * MINUTES_IN_HOUR;
}

std::optional<RouteInfo> TransportRouter::BuildRoute(const std::string& from, 
                                                     const std::string& to) const {
    auto from_it = stop_to_vertex_id_.find(from);
    auto to_it = stop_to_vertex_id_.find(to);
    
    if (from_it == stop_to_vertex_id_.end() || to_it == stop_to_vertex_id_.end()) {
        return std::nullopt;
    }
    
    size_t from_vertex = GetWaitVertexId(from);
    size_t to_vertex = GetWaitVertexId(to);
    
    auto route = router_->BuildRoute(from_vertex, to_vertex);
    
    if (!route) {
        if (from == to) {
            RouteInfo result;
            result.total_time = 0.0;
            return result;
        }
        return std::nullopt;
    }
    
    RouteInfo result;
    result.total_time = route->weight;
    
    for (auto edge_id : route->edges) {
        const auto& edge = graph_->GetEdge(edge_id);
        
        if (std::abs(edge.weight - static_cast<double>(settings_.bus_wait_time)) < 1e-6) {
            result.activities.push_back(CreateWaitActivity(edge_id));
        } else {
            result.activities.push_back(CreateBusActivity(edge_id));
        }
    }
    
    return result;
}

RouteActivity TransportRouter::CreateWaitActivity(graph::EdgeId id) const {
    RouteActivity activity;
    activity.type = ActivityType::WAIT;
    
    auto it = wait_edges_.find(id);
    if (it != wait_edges_.end()) {
        activity.name = it->second;
        activity.time = static_cast<double>(settings_.bus_wait_time);
    }
    
    return activity;
}

RouteActivity TransportRouter::CreateBusActivity(graph::EdgeId id) const {
    RouteActivity activity;
    activity.type = ActivityType::BUS;
    
    auto it = bus_edges_.find(id);
    if (it != bus_edges_.end()) {
        activity.name = it->second.first;
        activity.span_count = it->second.second;
        const auto& edge = graph_->GetEdge(id);
        activity.time = edge.weight;
    }
    
    return activity;
}

size_t TransportRouter::GetWaitVertexId(const std::string& stop_name) const {
    auto it = stop_to_vertex_id_.find(stop_name);
    if (it != stop_to_vertex_id_.end()) {
        return it->second * 2;
    }
    return 0;
}

size_t TransportRouter::GetBusVertexId(const std::string& stop_name) const {
    auto it = stop_to_vertex_id_.find(stop_name);
    if (it != stop_to_vertex_id_.end()) {
        return it->second * 2 + 1;
    }
    return 0;
}

} // namespace transport_router