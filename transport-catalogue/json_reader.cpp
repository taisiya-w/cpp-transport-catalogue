#include "json_reader.h"
#include <sstream>
#include <algorithm>

using namespace std;

namespace json_reader {

namespace {

svg::Color ParseColor(const json::Node& color_node) {
    if (color_node.IsString()) {
        return color_node.AsString();
    } else if (color_node.IsArray()) {
        const auto& arr = color_node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb(
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt())
            );
        } else if (arr.size() == 4) {
            return svg::Rgba(
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt()),
                arr[3].AsDouble()
            );
        }
    }
    return svg::Rgb(0, 0, 0);
}

} // namespace

JSONReader::JSONReader(istream& input)
    : json_doc_(json::Load(input))
    , root_(json_doc_.GetRoot().AsMap()) {
}

void JSONReader::Process(ostream& output) {
    ParseRenderSettings();
    
    ParseBaseRequests();
    
    ParseStatRequests();
    
    json::Print(json::Document{stat_responses_}, output);
}

void JSONReader::ParseRenderSettings() {
    auto it = root_.find("render_settings");
    if (it == root_.end()) {
        return;
    }
    
    const auto& rs = it->second.AsMap();
    render_settings_.width = rs.at("width").AsDouble();
    render_settings_.height = rs.at("height").AsDouble();
    render_settings_.padding = rs.at("padding").AsDouble();
    render_settings_.stop_radius = rs.at("stop_radius").AsDouble();
    render_settings_.line_width = rs.at("line_width").AsDouble();
    render_settings_.bus_label_font_size = rs.at("bus_label_font_size").AsInt();
    render_settings_.stop_label_font_size = rs.at("stop_label_font_size").AsInt();
    render_settings_.underlayer_width = rs.at("underlayer_width").AsDouble();
    
    const auto& bl_offset = rs.at("bus_label_offset").AsArray();
    render_settings_.bus_label_offset = {bl_offset[0].AsDouble(), bl_offset[1].AsDouble()};
    
    const auto& sl_offset = rs.at("stop_label_offset").AsArray();
    render_settings_.stop_label_offset = {sl_offset[0].AsDouble(), sl_offset[1].AsDouble()};
    
    render_settings_.underlayer_color = ParseColor(rs.at("underlayer_color"));
    
    const auto& palette = rs.at("color_palette").AsArray();
    for (const auto& color_node : palette) {
        render_settings_.color_palette.push_back(ParseColor(color_node));
    }
}

void JSONReader::ParseBaseRequests() {
    auto it = root_.find("base_requests");
    if (it == root_.end()) {
        return;
    }
    
    for (const auto& node : it->second.AsArray()) {
        const auto& dict = node.AsMap();
        if (dict.at("type").AsString() == "Stop") {
            stop_requests_.push_back(&dict);
        } else {
            bus_requests_.push_back(&dict);
        }
    }
    
    for (const auto* stop_dict : stop_requests_) {
        AddStopFromJSON(*stop_dict);
    }
    
    for (const auto* stop_dict : stop_requests_) {
        AddDistancesFromJSON(*stop_dict);
    }
    
    for (const auto* bus_dict : bus_requests_) {
        AddBusFromJSON(*bus_dict);
    }
}

void JSONReader::AddStopFromJSON(const json::Dict& stop_dict) {
    string name = stop_dict.at("name").AsString();
    double lat = stop_dict.at("latitude").AsDouble();
    double lng = stop_dict.at("longitude").AsDouble();
    db_.AddStop(move(name), {lat, lng});
}

void JSONReader::AddDistancesFromJSON(const json::Dict& stop_dict) {
    string from = stop_dict.at("name").AsString();
    auto it = stop_dict.find("road_distances");
    if (it != stop_dict.end()) {
        const auto& dist_map = it->second.AsMap();
        for (const auto& [to, node] : dist_map) {
            db_.AddDistance(from, to, node.AsInt());
        }
    }
}

void JSONReader::AddBusFromJSON(const json::Dict& bus_dict) {
    string name = bus_dict.at("name").AsString();
    bool is_roundtrip = bus_dict.at("is_roundtrip").AsBool();
    const auto& stops_array = bus_dict.at("stops").AsArray();
    
    vector<string> stop_names;
    for (const auto& node : stops_array) {
        stop_names.push_back(node.AsString());
    }
    
    if (!stop_names.empty()) {
        db_.AddBus(move(name), move(stop_names), is_roundtrip);
    }
}

void JSONReader::ParseStatRequests() {
    auto it = root_.find("stat_requests");
    if (it == root_.end() || !it->second.IsArray()) {
        return;
    }
    
    const auto& stat_requests = it->second.AsArray();
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
                stat_responses_.push_back(MakeBusResponse(id, db_.GetBusInfo(name_it->second.AsString())));
            }
        } else if (type == "Stop") {
            auto name_it = r.find("name");
            if (name_it != r.end() && name_it->second.IsString()) {
                stat_responses_.push_back(MakeStopResponse(id, db_.GetStopInfo(name_it->second.AsString())));
            }
        } else if (type == "Map") {
            stat_responses_.push_back(MakeMapResponse(id));
        }
    }
}

json::Node JSONReader::MakeBusResponse(int id, optional<domain::BusInfo> info) const {
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

json::Node JSONReader::MakeStopResponse(int id, const set<const domain::Bus*, domain::BusComp>* buses) const {
    if (buses == nullptr) {
        return json::Dict{
            {"request_id", json::Node(id)},
            {"error_message", json::Node(string("not found"))}
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

json::Node JSONReader::MakeMapResponse(int id) const {
    renderer::MapRenderer map_renderer(render_settings_);
    
    const auto& all_buses = db_.GetAllBuses();
    vector<const domain::Bus*> buses_for_render;
    buses_for_render.reserve(all_buses.size());
    for (const auto& [name, bus] : all_buses) {
        buses_for_render.push_back(bus);
    }
    
    map_renderer.SetBuses(buses_for_render, db_);
    
    svg::Document svg_doc = map_renderer.Render(db_);
    stringstream svg_stream;
    svg_doc.Render(svg_stream);
    string svg_str = svg_stream.str();
    
    return json::Dict{
        {"request_id", json::Node(id)},
        {"map", json::Node(svg_str)}
    };
}

const renderer::RenderSettings& JSONReader::GetRenderSettings() const {
    return render_settings_;
}

transport_catalogue::TransportCatalogue& JSONReader::GetTransportCatalogue() {
    return db_;
}

const transport_catalogue::TransportCatalogue& JSONReader::GetTransportCatalogue() const {
    return db_;
}

void Process(istream& in, ostream& out) {
    JSONReader reader(in);
    reader.Process(out);
}

} // namespace json_reader