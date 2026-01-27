#pragma once

#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace transport_router {

struct RoutingSettings {
    int bus_wait_time = 0;
    double bus_velocity = 0.0;
};

enum class ActivityType {
    WAIT,
    BUS
};

struct RouteActivity {
    ActivityType type;
    std::string name;
    double time = 0.0;
    int span_count = 0;
};

struct RouteInfo {
    double total_time = 0.0;
    std::vector<RouteActivity> activities;
};

class TransportRouter {
public:
    TransportRouter(const transport_catalogue::TransportCatalogue& catalogue, 
                    const RoutingSettings& settings);
    
    std::optional<RouteInfo> BuildRoute(const std::string& from, 
                                        const std::string& to) const;
    
private:
    void BuildGraph();
    void AddEdgesForBus(const domain::Bus* bus);
    
    double ComputeBusTime(int distance) const;
    
    RouteActivity CreateWaitActivity(graph::EdgeId id) const;
    RouteActivity CreateBusActivity(graph::EdgeId id) const;
    
    const transport_catalogue::TransportCatalogue& catalogue_;
    RoutingSettings settings_;
    
    std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    
    std::unordered_map<std::string, size_t> stop_to_vertex_id_;
    std::vector<std::string> vertex_id_to_stop_;
    
    std::unordered_map<graph::EdgeId, std::string> wait_edges_;
    std::unordered_map<graph::EdgeId, std::pair<std::string, int>> bus_edges_;
    
    size_t GetWaitVertexId(const std::string& stop_name) const;
    size_t GetBusVertexId(const std::string& stop_name) const;
};

} // namespace transport_router