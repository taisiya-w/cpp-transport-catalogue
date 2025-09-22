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
			std::string X;
			size_t R = 0;
			size_t U = 0;
			double L = 0.0;
		};

		class TransportCatalogue {
		public:
			void AddStop(std::string name, geo::Coordinates coords);
			void AddBus(std::string name, std::deque<const Stop*> stops);
			const Bus* FindBus(std::string_view name) const;
			const Stop* FindStop(std::string_view name) const;
			std::optional<BusInfo> GetBusInfo(std::string_view name) const;
			std::optional<std::unordered_set<const Bus*>> GetStopInfo(std::string_view name) const;
		private:
			std::deque<Stop> stops_storage_;
			std::deque<Bus> buses_storage_;
			std::unordered_map<std::string_view, const Stop*> stops_point_;
			std::unordered_map<std::string_view, const Bus*> buses_point_;
			std::unordered_map<const Stop*, std::unordered_set<const Bus*>> buses_at_stop_;
		};
	}
}