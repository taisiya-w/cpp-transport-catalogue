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

struct WaitActivity {
    std::string stop_name;
    double time = 0.0;
};

struct BusActivity {
    std::string bus;
    int span_count = 0;
    double time = 0.0;
};

struct RouteInfo {
    double total_time = 0.0;
    std::vector<graph::EdgeId> edges;
};

class TransportRouter {
public:
    TransportRouter(const transport_catalogue::TransportCatalogue& catalogue, 
                    const RoutingSettings& settings);
    
    std::optional<RouteInfo> BuildRoute(const std::string& from, 
                                        const std::string& to) const;
    
    WaitActivity GetWaitInfo(graph::EdgeId id) const;
    BusActivity GetBusInfo(graph::EdgeId id) const;
    
private:
    void BuildGraph();
    void AddEdgesForBus(const domain::Bus* bus);
    
    double ComputeBusTime(int distance) const;
    
    const transport_catalogue::TransportCatalogue& catalogue_;
    RoutingSettings settings_;
    
    std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    
    std::unordered_map<std::string, size_t> stop_to_vertex_id_;
    std::vector<std::string> vertex_id_to_stop_;
    
    std::unordered_map<graph::EdgeId, WaitActivity> wait_edges_;
    std::unordered_map<graph::EdgeId, BusActivity> bus_edges_;
    
    size_t GetWaitVertexId(const std::string& stop_name) const;
    size_t GetBusVertexId(const std::string& stop_name) const;
};

} // namespace transport_router