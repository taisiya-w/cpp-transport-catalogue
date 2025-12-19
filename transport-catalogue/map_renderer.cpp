#include "map_renderer.h"
#include "geo.h"
#include <algorithm>
#include <set>
#include <unordered_set>
#include <functional>

using namespace std;

namespace renderer {

class MapRenderer::SphereProjector {
public:
    template <typename Iterator>
    SphereProjector(Iterator begin, Iterator end, double width, double height, double padding)
        : padding_(padding) {
        if (begin == end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            begin, end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            begin, end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        } else {
            zoom_coeff_ = 0;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
    
    static bool IsZero(double value) {
        return std::abs(value) < 1e-9;
    }
};

MapRenderer::MapRenderer(const RenderSettings& settings) 
    : settings_(settings) {
}

void MapRenderer::SetBuses(const std::vector<const domain::Bus*>& buses, const transport_catalogue::TransportCatalogue& db) {
    buses_ = buses;
    all_coords_.clear();

    if (buses.empty()) {
        return;
    }

    std::unordered_set<const domain::Stop*> unique_stops;
    for (const domain::Bus* bus : buses) {
        for (const string& name : bus->stop_names) {
            if (const domain::Stop* stop = db.FindStop(name)) {
                unique_stops.insert(stop);
            }
        }
    }
    
    for (const domain::Stop* stop : unique_stops) {
        all_coords_.push_back(stop->coords);
    }
}

std::vector<const domain::Bus*> MapRenderer::GetSortedBuses() const {
    std::vector<const domain::Bus*> sorted_buses = buses_;
    sort(sorted_buses.begin(), sorted_buses.end(),
        [](const domain::Bus* lhs, const domain::Bus* rhs) {
            return lhs->name < rhs->name;
        });
    return sorted_buses;
}

std::vector<const domain::Stop*> MapRenderer::GetUniqueStops(const transport_catalogue::TransportCatalogue& db) const {
    std::vector<const domain::Stop*> unique_stops;
    for (const domain::Bus* bus : buses_) {
        for (const string& name : bus->stop_names) {
            if (const domain::Stop* stop = db.FindStop(name)) {
                unique_stops.push_back(stop);
            }
        }
    }

    sort(unique_stops.begin(), unique_stops.end(),
        [](const domain::Stop* a, const domain::Stop* b) {
            return a->name < b->name;
        });
    auto last = unique(unique_stops.begin(), unique_stops.end(),
        [](const domain::Stop* a, const domain::Stop* b) {
            return a->name == b->name;
        });
    unique_stops.erase(last, unique_stops.end());
    
    return unique_stops;
}

svg::Text MapRenderer::CreateUnderlayerText(const std::string& text, const svg::Point& pos) const {
    return svg::Text{}
        .SetPosition(pos)
        .SetOffset(settings_.stop_label_offset)
        .SetFontSize(static_cast<uint32_t>(settings_.stop_label_font_size))
        .SetFontFamily("Verdana")
        .SetData(text)
        .SetFillColor(settings_.underlayer_color)
        .SetStrokeColor(settings_.underlayer_color)
        .SetStrokeWidth(settings_.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

svg::Text MapRenderer::CreateFillText(const std::string& text, const svg::Point& pos, const svg::Color& fill_color) const {
    return svg::Text{}
        .SetPosition(pos)
        .SetOffset(settings_.stop_label_offset)
        .SetFontSize(static_cast<uint32_t>(settings_.stop_label_font_size))
        .SetFontFamily("Verdana")
        .SetData(text)
        .SetFillColor(fill_color);
}

void MapRenderer::RenderBusLines(svg::Document& doc, const std::vector<const domain::Bus*>& sorted_buses, const SphereProjector& proj, const transport_catalogue::TransportCatalogue& db) const {
    size_t color_index = 0;
    for (const domain::Bus* bus : sorted_buses) {
        if (bus->stop_names.size() < 2) {
            continue;
        }
        
        std::vector<const domain::Stop*> stops;
        for (const string& name : bus->stop_names) {
            if (const domain::Stop* s = db.FindStop(name)) {
                stops.push_back(s);
            } else {
                stops.clear();
                break;
            }
        }
        if (stops.size() < 2) continue;

        svg::Polyline polyline;
        polyline.SetFillColor(svg::NoneColor)
                .SetStrokeColor(settings_.color_palette[color_index % settings_.color_palette.size()])
                .SetStrokeWidth(settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        if (bus->is_roundtrip) {
            for (const domain::Stop* stop : stops) {
                polyline.AddPoint(proj(stop->coords));
            }
        } else {
            for (const domain::Stop* stop : stops) {
                polyline.AddPoint(proj(stop->coords));
            }
            if (stops.size() > 1) {
                for (size_t i = stops.size() - 1; i > 0; --i) {
                    polyline.AddPoint(proj(stops[i - 1]->coords));
                }
            }
        }
        
        doc.Add(polyline);
        ++color_index;
    }
}

void MapRenderer::RenderBusLabels(svg::Document& doc, const std::vector<const domain::Bus*>& sorted_buses, const SphereProjector& proj, const transport_catalogue::TransportCatalogue& db) const {
    size_t color_index = 0;
    for (const domain::Bus* bus : sorted_buses) {
        if (bus->stop_names.empty()) continue;
        
        std::vector<const domain::Stop*> stops;
        for (const string& name : bus->stop_names) {
            if (const domain::Stop* s = db.FindStop(name)) {
                stops.push_back(s);
            } else {
                stops.clear();
                break;
            }
        }
        if (stops.empty()) continue;

        svg::Color route_color = settings_.color_palette[color_index % settings_.color_palette.size()];

        auto renderBusLabel = [&](const svg::Point& pos, const std::string& bus_name) {
            doc.Add(svg::Text{}
                .SetPosition(pos)
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(static_cast<uint32_t>(settings_.bus_label_font_size))
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus_name)
                .SetFillColor(settings_.underlayer_color)
                .SetStrokeColor(settings_.underlayer_color)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
            );
            
            doc.Add(svg::Text{}
                .SetPosition(pos)
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(static_cast<uint32_t>(settings_.bus_label_font_size))
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus_name)
                .SetFillColor(route_color)
            );
        };

        if (bus->is_roundtrip) {
            svg::Point pos = proj(stops.front()->coords);
            renderBusLabel(pos, bus->name);
        } else {
            if (!stops.empty()) {
                svg::Point first_pos = proj(stops.front()->coords);
                renderBusLabel(first_pos, bus->name);
                
                if (stops.size() > 1 && stops.front() != stops.back()) {
                    svg::Point last_pos = proj(stops.back()->coords);
                    renderBusLabel(last_pos, bus->name);
                }
            }
        }
        
        if (bus->stop_names.size() >= 2) {
            ++color_index;
        }
    }
}

void MapRenderer::RenderStopCircles(svg::Document& doc, const std::vector<const domain::Stop*>& unique_stops, const SphereProjector& proj) const {
    for (const domain::Stop* stop : unique_stops) {
        svg::Point pos = proj(stop->coords);
        doc.Add(svg::Circle{}
            .SetCenter(pos)
            .SetRadius(settings_.stop_radius)
            .SetFillColor("white")
        );
    }
}

void MapRenderer::RenderStopLabels(svg::Document& doc, const std::vector<const domain::Stop*>& unique_stops, const SphereProjector& proj) const {
    for (const domain::Stop* stop : unique_stops) {
        svg::Point pos = proj(stop->coords);
        
        doc.Add(svg::Text{}
            .SetPosition(pos)
            .SetOffset(settings_.stop_label_offset)
            .SetFontSize(static_cast<uint32_t>(settings_.stop_label_font_size))
            .SetFontFamily("Verdana")
            .SetData(stop->name)
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
        );
        
        doc.Add(svg::Text{}
            .SetPosition(pos)
            .SetOffset(settings_.stop_label_offset)
            .SetFontSize(static_cast<uint32_t>(settings_.stop_label_font_size))
            .SetFontFamily("Verdana")
            .SetData(stop->name)
            .SetFillColor("black")
        );
    }
}

svg::Document MapRenderer::Render(const transport_catalogue::TransportCatalogue& db) const {
    if (all_coords_.empty()) {
        return svg::Document{};
    }

    SphereProjector proj(all_coords_.begin(), all_coords_.end(),
                         settings_.width, settings_.height, settings_.padding);

    svg::Document doc;

    std::vector<const domain::Bus*> sorted_buses = GetSortedBuses();
    std::vector<const domain::Stop*> unique_stops = GetUniqueStops(db);

    RenderBusLines(doc, sorted_buses, proj, db);
    
    RenderBusLabels(doc, sorted_buses, proj, db);
    
    RenderStopCircles(doc, unique_stops, proj);
    
    RenderStopLabels(doc, unique_stops, proj);

    return doc;
}

} // namespace renderer