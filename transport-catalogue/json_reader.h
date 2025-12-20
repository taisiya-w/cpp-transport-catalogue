#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"
#include "svg.h"
#include "json_builder.h"

namespace json_reader {

class JSONReader {
public:
    explicit JSONReader(std::istream& input);
    
    void Process(std::ostream& output);
    
    const renderer::RenderSettings& GetRenderSettings() const;
    
    transport_catalogue::TransportCatalogue& GetTransportCatalogue();
    const transport_catalogue::TransportCatalogue& GetTransportCatalogue() const;

private:
    void ParseBaseRequests();
    void ParseStatRequests();
    void ParseRenderSettings();
    
    void AddStopFromJSON(const json::Dict& stop_dict);
    void AddBusFromJSON(const json::Dict& bus_dict);
    void AddDistancesFromJSON(const json::Dict& stop_dict);
    
    json::Node MakeBusResponse(int id, std::optional<domain::BusInfo> info) const;
    json::Node MakeStopResponse(int id, const std::set<const domain::Bus*, domain::BusComp>* buses) const;
    json::Node MakeMapResponse(int id) const;
    
    json::Document json_doc_;
    const json::Dict& root_;
    
    transport_catalogue::TransportCatalogue db_;
    renderer::RenderSettings render_settings_;
    
    std::vector<const json::Dict*> stop_requests_;
    std::vector<const json::Dict*> bus_requests_;
    
    json::Array stat_responses_;
};

} // namespace json_reader