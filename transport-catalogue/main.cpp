#include <iostream>
#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>

#include "input_reader.h"
#include "stat_reader.h"

using namespace transport_catalogue;

int main() {
    core::TransportCatalogue catalogue;
    io::ReadInput(std::cin, catalogue);
    io::ProcessRequests(std::cin, std::cout, catalogue);
}