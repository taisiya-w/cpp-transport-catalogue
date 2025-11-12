#include "stat_reader.h"

namespace transport_catalogue {
    namespace io {

        void ParseAndPrintStat(const core::TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
            auto pos = request.find(" ");
            std::string_view type = request.substr(0, pos);
            std::string_view id = request.substr(pos+1);
            if (type == "Bus") {
                auto info_opt = transport_catalogue.GetBusInfo(id);
                if (info_opt) {
                    transport_catalogue::core::BusInfo info = *info_opt;
                    output << "Bus " << id << ": " << info.all_stops << " stops on route, " << info.unique_stops << " unique stops, " << info.route_length << " route length"<< ", " << std::fixed << std::setprecision(6) << info.route_curvature << " curvature\n";
                } else {
                    output << "Bus " << id << ": not found\n";
                }
            }
            if (type == "Stop") {
                const auto& info = transport_catalogue.GetStopInfo(id);
                if (info != nullptr) {
                    if (info->empty()) {
                        output << "Stop " << id << ": no buses\n";
                    } else {
                        output << "Stop " << id << ": buses";
                        for (const auto* bus : *info) {
                            output << " " << bus->name;
                        }
                        output << "\n";
                    }
                } else {
                    output << "Stop " << id << ": not found\n";
                }
            }
            
        }
        void ProcessRequests(std::istream& in, std::ostream& out, const core::TransportCatalogue& catalogue) {
            int stat_request_count;
            in >> stat_request_count >> std::ws;
            for (int i = 0; i < stat_request_count; ++i) {
                std::string line;
                std::getline(in, line);
                io::ParseAndPrintStat(catalogue, line, out);
            }
        }
    }
}