#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <set>

#include "geo.h"

namespace transport_catalogue {
    namespace core {

		struct Stop {
			std::string name;
			geo::Coordinates coords;
		};

		struct Bus {
			std::string name;
			std::deque<const Stop*> stops;
		};

		struct BusInfo {
			size_t all_stops = 0;
			size_t unique_stops = 0;
			int route_length = 0;
			double route_curvature = 1.0;
		};

		struct BusComp {
			bool operator()(const Bus* lhs, const Bus* rhs) const {
				return lhs -> name < rhs -> name;
			}
		};

        struct PairHash {
            size_t operator()(const std::pair<const Stop*, const Stop*>& p) const {
                auto h1 = std::hash<const void*>{}(p.first);
                auto h2 = std::hash<const void*>{}(p.second);
                return h1 ^ (h2 << 1);
            }
        };

		class TransportCatalogue {
		public:
			void AddStop(std::string name, const geo::Coordinates coords);
			void AddBus(std::string name, const std::deque<const Stop*> stops);
			const Bus* FindBus(std::string_view name) const;
			const Stop* FindStop(std::string_view name) const;
			std::optional<BusInfo> GetBusInfo(std::string_view name) const;
			const std::set<const Bus*, BusComp>* GetStopInfo(std::string_view name) const;
			int GetDistance(const Stop* from, const Stop* to) const;
			void AddDistance(std::string_view from, std::string_view to, int distance);
		private:
			std::deque<Stop> stops_storage_;
			std::deque<Bus> buses_storage_;
			std::unordered_map<std::string_view, const Stop*> stops_point_;
			std::unordered_map<std::string_view, const Bus*> buses_point_;
			std::unordered_map<const Stop*, std::set<const Bus*, BusComp>> buses_at_stop_;
            		std::unordered_map<std::pair<const Stop*, const Stop*>, int, PairHash> road_distances_;
		};
	}
}