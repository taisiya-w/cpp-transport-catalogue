#pragma once

#include <iosfwd>
#include <string_view>
#include <optional>

#include "transport_catalogue.h"

namespace transport_catalogue {
    namespace io {
        void ParseAndPrintStat(const core::TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output);
        void ProcesRequests(std::istream& in, std::ostream& out, const core::TransportCatalogue& catalogue);
    }
}