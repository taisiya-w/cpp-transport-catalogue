#pragma once

#include <string_view>
#include <optional>
#include <set>
#include <vector>
#include "domain.h"

namespace transport_catalogue {

class TransportCatalogue;

class RequestHandler {
public:
    explicit RequestHandler(const TransportCatalogue& db) : db_(db) {}

    std::optional<domain::BusInfo> GetBusInfo(std::string_view name) const;
    const std::set<const domain::Bus*, domain::BusComp>* GetStopInfo(std::string_view name) const;

private:
    const TransportCatalogue& db_;
};

} // namespace transport_catalogue