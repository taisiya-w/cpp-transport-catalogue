#include "transport_catalogue.h"
#include "geo.h"

namespace transport_catalogue {
    namespace core {

        void TransportCatalogue::AddStop(std::string name, geo::Coordinates coords) {
            stops_storage_.push_back({std::move(name), coords});
            const Stop* ptr = &stops_storage_.back();
            stops_point_[ptr->name] = ptr;
        }

        void TransportCatalogue::AddBus(std::string name, std::deque<const Stop*> stops) {
            buses_storage_.push_back({std::move(name), stops});
            const Bus* ptr = &buses_storage_.back();
            buses_point_[ptr->name] = ptr;
            for (auto s : stops) {
                buses_at_stop_[s].insert(ptr);
            }
        }

        const Bus* TransportCatalogue::FindBus(std::string_view name) const {
            if (auto it = buses_point_.find(name); it != buses_point_.end()) {
                    return it->second;
                }
            return nullptr;
        }

        const Stop* TransportCatalogue::FindStop(std::string_view name) const {
            if (auto it = stops_point_.find(name); it != stops_point_.end()) {
                    return it->second;
                }
            return nullptr;
        }

        std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const {
            if (auto it = buses_point_.find(name); it != buses_point_.end()) {
                const Bus* ptr = it->second;
                BusInfo bus;
                bus.X = ptr->name;
                bus.R = (ptr->stops).size();
                std::unordered_set<const Stop*> unique;
                for (size_t i = 0; i < (ptr->stops).size() - 1; ++i) {
                    bus.L += ComputeDistance(ptr->stops[i]->coords, ptr->stops[i+1]->coords);
                    unique.insert(ptr->stops[i]);
                }
                unique.insert(ptr->stops.back());
                bus.U = unique.size();
                return bus;
            }
            return std::nullopt;
        }

        std::optional<std::unordered_set<const Bus*>> TransportCatalogue::GetStopInfo(std::string_view name) const {
            const Stop* ptr = FindStop(name);
            if (ptr == nullptr) {
                return std::nullopt;
            } else {
                auto it = buses_at_stop_.find(ptr);
                if (it == buses_at_stop_.end()) {
                    return std::unordered_set<const Bus*>{};
                }
                return it->second;
            }
        }
    }
}