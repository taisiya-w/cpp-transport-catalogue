#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

#include "domain.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace renderer {

struct RenderSettings {
    double width = 0.0;
    double height = 0.0;
    double padding = 0.0;
    double stop_radius = 0.0;
    double line_width = 0.0;
    int bus_label_font_size = 0;
    svg::Point bus_label_offset = {0.0, 0.0};
    int stop_label_font_size = 0;
    svg::Point stop_label_offset = {0.0, 0.0};
    svg::Color underlayer_color;
    double underlayer_width = 0.0;
    std::vector<svg::Color> color_palette;
};

class MapRenderer {
public:
    explicit MapRenderer(const RenderSettings& settings);
    
    void SetBuses(const std::vector<const domain::Bus*>& buses, const transport_catalogue::TransportCatalogue& db);
    
    svg::Document Render(const transport_catalogue::TransportCatalogue& db) const;

private:
    class SphereProjector;
    
    std::vector<const domain::Stop*> GetUniqueStops(const transport_catalogue::TransportCatalogue& db) const;
    std::vector<const domain::Bus*> GetSortedBuses() const;
    std::vector<svg::Point> GetStopPoints(const std::vector<const domain::Stop*>& stops, const SphereProjector& proj) const;
    
    void RenderBusLines(svg::Document& doc, const std::vector<const domain::Bus*>& sorted_buses, const SphereProjector& proj, const transport_catalogue::TransportCatalogue& db) const;
    
    void RenderBusLabels(svg::Document& doc, const std::vector<const domain::Bus*>& sorted_buses, const SphereProjector& proj, const transport_catalogue::TransportCatalogue& db) const;
    
    void RenderStopCircles(svg::Document& doc, const std::vector<const domain::Stop*>& unique_stops, const SphereProjector& proj) const;
    
    void RenderStopLabels(svg::Document& doc, const std::vector<const domain::Stop*>& unique_stops, const SphereProjector& proj) const;
    
    svg::Text CreateUnderlayerText(const std::string& text, const svg::Point& pos) const;
    svg::Text CreateFillText(const std::string& text, const svg::Point& pos, const svg::Color& fill_color) const;
    
    RenderSettings settings_;
    std::vector<const domain::Bus*> buses_;
    std::vector<geo::Coordinates> all_coords_;
};

} // namespace renderer