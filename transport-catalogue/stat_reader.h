#pragma once

#include <iosfwd>
#include <string_view>
#include <optional>
#include <iomanip>

#include "transport_catalogue.h"

namespace transport_catalogue {
    namespace io {
        void ParseAndPrintStat(const core::TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output);
        void ProcessRequests(std::istream& in, std::ostream& out, const core::TransportCatalogue& catalogue);
    }
}