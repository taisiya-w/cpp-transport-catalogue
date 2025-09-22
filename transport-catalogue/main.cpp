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

    int base_request_count;
    std::cin >> base_request_count >> std::ws;

    {
        io::InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            std::string line;
            std::getline(std::cin, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

    int stat_request_count;
    std::cin >> stat_request_count >> std::ws;
    for (int i = 0; i < stat_request_count; ++i) {
        std::string line;
        std::getline(std::cin, line);
        io::ParseAndPrintStat(catalogue, line, std::cout);
    }
}