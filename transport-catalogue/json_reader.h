#pragma once

#include <iosfwd>

namespace json_reader {
void Process(std::istream& in, std::ostream& out);
}