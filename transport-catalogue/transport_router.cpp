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
        wait_edges_[wait_edge_id] = {stop_name, static_cast<double>(settings_.bus_wait_time)};
        
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
                bus_edges_[edge_id] = {bus->name, span_count, travel_time};
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
                bus_edges_[edge_id] = {bus->name, span_count, travel_time};
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
                bus_edges_[edge_id] = {bus->name, span_count, travel_time};
            }
        }
    }
}

double TransportRouter::ComputeBusTime(int distance) const {
    double distance_km = distance / 1000.0;
    double time_hours = distance_km / settings_.bus_velocity;
    return time_hours * 60.0;
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
    result.edges = std::move(route->edges);
    
    return result;
}

const graph::Edge<double>& TransportRouter::GetEdge(graph::EdgeId id) const {
    return graph_->GetEdge(id);
}

const WaitActivity& TransportRouter::GetWaitInfo(graph::EdgeId id) const {
    static WaitActivity empty{};
    auto it = wait_edges_.find(id);
    if (it != wait_edges_.end()) {
        return it->second;
    }
    return empty;
}

const BusActivity& TransportRouter::GetBusInfo(graph::EdgeId id) const {
    static BusActivity empty{};
    auto it = bus_edges_.find(id);
    if (it != bus_edges_.end()) {
        return it->second;
    }
    return empty;
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

std::string TransportRouter::GetStopNameByVertexId(size_t vertex_id) const {
    if (vertex_id % 2 == 0) {
        size_t stop_index = vertex_id / 2;
        if (stop_index < vertex_id_to_stop_.size()) {
            return vertex_id_to_stop_[stop_index];
        }
    } else {
        size_t stop_index = (vertex_id - 1) / 2;
        if (stop_index < vertex_id_to_stop_.size()) {
            return vertex_id_to_stop_[stop_index];
        }
    }
    return "";
}

} // namespace transport_router