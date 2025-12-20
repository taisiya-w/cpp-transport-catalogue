#include "json_reader.h"

int main() {
    json_reader::JSONReader reader(std::cin);
    reader.Process(std::cout);
    return 0;
}