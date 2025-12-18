#pragma once
#include <vector>
#include <string>
#include "svg.h"
#include "domain.h"
#include "geo.h"
#include "transport_catalogue.h"

namespace renderer {

struct RenderSettings {
    double width = 0.0;
    double height = 0.0;
    double padding = 0.0;
    double line_width = 0.0;
    double stop_radius = 0.0;
    int bus_label_font_size = 0;
    svg::Point bus_label_offset = {0.0, 0.0};
    int stop_label_font_size = 0;
    svg::Point stop_label_offset = {0.0, 0.0};
    svg::Color underlayer_color = svg::NoneColor;
    double underlayer_width = 0.0;
    std::vector<svg::Color> color_palette;
};

class MapRenderer {
public:
    explicit MapRenderer(const RenderSettings& settings) : settings_(settings) {}

    void SetBuses(const std::vector<const domain::Bus*>& buses, const transport_catalogue::TransportCatalogue& db);
    svg::Document Render(const transport_catalogue::TransportCatalogue& db) const;

private:
    RenderSettings settings_;
    std::vector<const domain::Bus*> buses_;
    std::vector<geo::Coordinates> all_coords_;
};

} // namespace renderer