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
                    output << "Bus " << info.X << ": " << info.R << " stops on route, " << info.U << " unique stops, " << std::fixed << std::setprecision(6) << info.L << " route length\n";
                } else {
                    output << "Bus " << id << ": not found\n";
                }
            }
            if (type == "Stop") {
                auto info_opt = transport_catalogue.GetStopInfo(id);
                if (info_opt) {
                    const auto& info = *info_opt;
                    if (info.empty()) {
                        output << "Stop " << id << ": no buses\n";
                    } else {
                        std::set<std::string_view> result;
                        for (auto bus : info) {
                            result.insert(bus->name);
                        }
                        output << "Stop " << id << ": buses";
                        for (auto bus : result) {
                            output << " " << bus;
                        }
                        output << "\n";
                    }
                } else {
                    output << "Stop " << id << ": not found\n";
                }
            }
            
        }
    }
}