#include "json_reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "json.h"
#include "map_renderer.h"
#include "svg.h"

#include <cstdint>
#include <sstream>
#include <vector>
#include <algorithm>
#include <string_view>

using namespace std;
using namespace transport_catalogue;

namespace {

void AddStop(TransportCatalogue& db, const json::Dict& stop) {
    string name = stop.at("name").AsString();
    double lat = stop.at("latitude").AsDouble();
    double lng = stop.at("longitude").AsDouble();
    db.AddStop(move(name), {lat, lng});
}

void AddDistances(TransportCatalogue& db, const json::Dict& stop) {
    string from = stop.at("name").AsString();
    auto it = stop.find("road_distances");
    if (it != stop.end()) {
        const auto& dist_map = it->second.AsMap();
        for (const auto& [to, node] : dist_map) {
            db.AddDistance(from, to, node.AsInt());
        }
    }
}

void AddBus(TransportCatalogue& db, const json::Dict& bus) {
    string name = bus.at("name").AsString();
    bool is_roundtrip = bus.at("is_roundtrip").AsBool();
    const auto& stops_array = bus.at("stops").AsArray();
    
    vector<string> stop_names;
    for (const auto& node : stops_array) {
        string stop_name = node.AsString();
        stop_names.push_back(stop_name);
    }
    
    if (!stop_names.empty()) {
        db.AddBus(move(name), move(stop_names), is_roundtrip);
    }
}

json::Node MakeBusResponse(int id, optional<domain::BusInfo> info) {
    if (!info) {
        return json::Dict{
            {"request_id", json::Node(id)},
            {"error_message", json::Node(string("not found"))}
        };
    }
    
    return json::Dict{
        {"request_id", json::Node(id)},
        {"stop_count", json::Node(static_cast<int>(info->all_stops))},
        {"unique_stop_count", json::Node(static_cast<int>(info->unique_stops))},
        {"route_length", json::Node(info->route_length)},
        {"curvature", json::Node(info->route_curvature)}
    };
}

json::Node MakeStopResponse(int id, const set<const domain::Bus*, domain::BusComp>* buses) {

    if (buses == nullptr) {
        return json::Dict{
            {"request_id", json::Node(id)},
            {"error_message", json::Node("not found"s)}
        };
    }
    
    vector<string> bus_names;
    for (const domain::Bus* bus : *buses) {
        bus_names.push_back(bus->name);
    }
    sort(bus_names.begin(), bus_names.end());

    json::Array bus_array;
    for (const auto& name : bus_names) {
        bus_array.push_back(json::Node(name));
    }
    
    return json::Dict{
        {"request_id", json::Node(id)},
        {"buses", json::Node(bus_array)}
    };
}

} // namespace

void json_reader::Process(istream& in, ostream& out) {
    auto doc = json::Load(in);
    auto& root = doc.GetRoot().AsMap();
    
    renderer::RenderSettings settings;
    auto render_settings_it = root.find("render_settings");
    if (render_settings_it != root.end()) {
        const auto& rs = render_settings_it->second.AsMap();
        settings.width = rs.at("width").AsDouble();
        settings.height = rs.at("height").AsDouble();
        settings.padding = rs.at("padding").AsDouble();
        settings.stop_radius = rs.at("stop_radius").AsDouble();
        settings.line_width = rs.at("line_width").AsDouble();
        settings.bus_label_font_size = rs.at("bus_label_font_size").AsInt();
        settings.stop_label_font_size = rs.at("stop_label_font_size").AsInt();
        settings.underlayer_width = rs.at("underlayer_width").AsDouble();
        
        const auto& bl_offset = rs.at("bus_label_offset").AsArray();
        settings.bus_label_offset = {bl_offset[0].AsDouble(), bl_offset[1].AsDouble()};
        
        const auto& sl_offset = rs.at("stop_label_offset").AsArray();
        settings.stop_label_offset = {sl_offset[0].AsDouble(), sl_offset[1].AsDouble()};
        
        const auto& ul_color = rs.at("underlayer_color");
        if (ul_color.IsString()) {
            settings.underlayer_color = ul_color.AsString();
        } else if (ul_color.IsArray()) {
            const auto& arr = ul_color.AsArray();
            if (arr.size() == 4) {
                settings.underlayer_color = svg::Rgba(
                    static_cast<uint8_t>(arr[0].AsInt()),
                    static_cast<uint8_t>(arr[1].AsInt()),
                    static_cast<uint8_t>(arr[2].AsInt()),
                    arr[3].AsDouble()
                );
            } else if (arr.size() == 3) {
                settings.underlayer_color = svg::Rgb(
                    static_cast<uint8_t>(arr[0].AsInt()),
                    static_cast<uint8_t>(arr[1].AsInt()),
                    static_cast<uint8_t>(arr[2].AsInt())
                );
            }
        }
        
        const auto& palette = rs.at("color_palette").AsArray();
        for (const auto& color_node : palette) {
            if (color_node.IsString()) {
                settings.color_palette.push_back(color_node.AsString());
            } else if (color_node.IsArray()) {
                const auto& arr = color_node.AsArray();
                if (arr.size() == 3) {
                    settings.color_palette.emplace_back(svg::Rgb(
                        static_cast<uint8_t>(arr[0].AsInt()),
                        static_cast<uint8_t>(arr[1].AsInt()),
                        static_cast<uint8_t>(arr[2].AsInt())
                    ));
                } else if (arr.size() == 4) {
                    settings.color_palette.emplace_back(svg::Rgba(
                        static_cast<uint8_t>(arr[0].AsInt()),
                        static_cast<uint8_t>(arr[1].AsInt()),
                        static_cast<uint8_t>(arr[2].AsInt()),
                        arr[3].AsDouble()
                    ));
                }
            }
        }
    }

    TransportCatalogue db;
    
    vector<const json::Dict*> stops, buses;
    auto base_requests_it = root.find("base_requests");
    if (base_requests_it != root.end()) {
        for (const auto& node : base_requests_it->second.AsArray()) {
            const auto& dict = node.AsMap();
            if (dict.at("type").AsString() == "Stop") {
                stops.push_back(&dict);
            } else {
                buses.push_back(&dict);
            }
        }
    }

    for (const auto* s : stops) {
        AddStop(db, *s);
    }
    
    for (const auto* s : stops) {
        AddDistances(db, *s);
    }
    
    for (const auto* b : buses) {
        AddBus(db, *b);
    }
    
    RequestHandler handler(db);
    json::Array responses;
    
    auto stat_requests_it = root.find("stat_requests");
    if (stat_requests_it != root.end() && stat_requests_it->second.IsArray()) {
        const auto& stat_requests = stat_requests_it->second.AsArray();
        
        for (const auto& req_node : stat_requests) {
            if (!req_node.IsMap()) continue;
            const auto& r = req_node.AsMap();
            
            auto id_it = r.find("id");
            auto type_it = r.find("type");
            if (id_it == r.end() || !id_it->second.IsInt() ||
                type_it == r.end() || !type_it->second.IsString()) {
                continue;
            }
            
            int id = id_it->second.AsInt();
            string type = type_it->second.AsString();
            
            if (type == "Bus") {
                auto name_it = r.find("name");
                if (name_it != r.end() && name_it->second.IsString()) {
                    responses.push_back(MakeBusResponse(id, handler.GetBusInfo(name_it->second.AsString())));
                }
            } else if (type == "Stop") {
                auto name_it = r.find("name");
                if (name_it != r.end() && name_it->second.IsString()) {
                    responses.push_back(MakeStopResponse(id, handler.GetStopInfo(name_it->second.AsString())));
                }
            } else if (type == "Map") {
                renderer::MapRenderer map_renderer(settings);
                
                const auto& all_buses = db.GetAllBuses();
                std::vector<const domain::Bus*> buses_for_render;
                buses_for_render.reserve(all_buses.size());
                for (const auto& [name, bus] : all_buses) {
                    buses_for_render.push_back(bus);
                }
                
                map_renderer.SetBuses(buses_for_render, db);
                
                svg::Document svg_doc = map_renderer.Render(db);
                stringstream svg_stream;
                svg_doc.Render(svg_stream);
                string svg_str = svg_stream.str();
                
                responses.push_back(json::Dict{
                    {"request_id", json::Node(id)},
                    {"map", json::Node(svg_str)} 
                });
            }
        }
    }

    json::Print(json::Document{responses}, out);
}