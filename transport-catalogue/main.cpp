#include "json_reader.h"
#include <iostream>

int main() {
    json_reader::Process(std::cin, std::cout);
    return 0;
}