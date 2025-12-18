#include "json.h"
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace std;

namespace json {

namespace {

string ParseString(istream& input) {
    string result;
    char c;
    while (input.get(c)) {
        if (c == '"') {
            return result;
        }
        if (c == '\\') {
            if (!input.get(c)) {
                throw ParsingError("Invalid escape sequence");
            }
            switch (c) {
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case '"': result += '"'; break;
                case 't': result += '\t'; break;
                case '\\': result += '\\'; break;
                default: throw ParsingError("Invalid escape sequence");
            }
        } else {
            result += c;
        }
    }
    throw ParsingError("Unterminated string");
}

void PrintString(const string& str, ostream& out) {
    out << '"';
    for (char c : str) {
        switch (c) {
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '"': out << "\\\""; break;
            case '\t': out << "\\t"; break;
            case '\\': out << "\\\\"; break;
            default: out << c; break;
        }
    }
    out << '"';
}

void SkipSpaces(istream& input) {
    char c;
    while (input.get(c)) {
        if (!isspace(static_cast<unsigned char>(c))) {
            input.putback(c);
            break;
        }
    }
}

Node ParseNumber(istream& input) {
    string num_str;
    char c;
    
    if (input.peek() == '-' || input.peek() == '+') {
        input.get(c);
        num_str += c;
    }
    
    while (isdigit(input.peek())) {
        input.get(c);
        num_str += c;
    }
    
    if (input.peek() == '.') {
        input.get(c);
        num_str += c;
        while (isdigit(input.peek())) {
            input.get(c);
            num_str += c;
        }
    }
    
    if (input.peek() == 'e' || input.peek() == 'E') {
        input.get(c);
        num_str += c;
        if (input.peek() == '+' || input.peek() == '-') {
            input.get(c);
            num_str += c;
        }
        while (isdigit(input.peek())) {
            input.get(c);
            num_str += c;
        }
    }
    
    bool has_decimal = (num_str.find('.') != string::npos);
    bool has_exponent = (num_str.find('e') != string::npos || num_str.find('E') != string::npos);
    
    if (has_decimal || has_exponent) {
        double val = stod(num_str);
        return Node(val);
    } else {
        try {
            int val = stoi(num_str);
            return Node(val);
        } catch (...) {
            double val = stod(num_str);
            return Node(val);
        }
    }
}

Node ParseBool(istream& input) {
    char buffer[6] = {0};
    
    if (input.peek() == 't') {
        input.read(buffer, 4);
        if (input.gcount() == 4 && string(buffer, 4) == "true") {
            if (input.eof() || !isalpha(input.peek())) {
                return Node(true);
            }
        }
        for (int i = 3; i >= 0; --i) {
            input.putback(buffer[i]);
        }
    }
    
    if (input.peek() == 'f') {
        input.read(buffer, 5);
        if (input.gcount() == 5 && string(buffer, 5) == "false") {
            if (input.eof() || !isalpha(input.peek())) {
                return Node(false);
            }
        }
        for (int i = 4; i >= 0; --i) {
            input.putback(buffer[i]);
        }
    }
    
    throw ParsingError("Invalid boolean value");
}

Node ParseNull(istream& input) {
    char buffer[5] = {0};
    
    if (input.peek() == 'n') {
        input.read(buffer, 4);
        if (input.gcount() == 4 && string(buffer, 4) == "null") {
            if (input.eof() || !isalpha(input.peek())) {
                return Node(nullptr);
            }
        }
        for (int i = 3; i >= 0; --i) {
            input.putback(buffer[i]);
        }
    }
    
    throw ParsingError("Invalid null value");
}

Node ParseArray(istream& input);
Node ParseDict(istream& input);

Node ParseNode(istream& input) {
    SkipSpaces(input);
    if (input.eof()) {
        throw ParsingError("Unexpected end of input");
    }
    char c = input.peek();
    
    if (c == '"') {
        input.get();
        return Node(ParseString(input));
    } else if (c == '[') {
        input.get();
        return ParseArray(input);
    } else if (c == '{') {
        input.get();
        return ParseDict(input);
    } else if (c == 't' || c == 'f') {
        return ParseBool(input);
    } else if (c == 'n') {
        return ParseNull(input);
    } else if (isdigit(c) || c == '-' || c == '+') {
        return ParseNumber(input);
    } else {
        throw ParsingError("Unexpected character");
    }
}

Node ParseArray(istream& input) {
    Array arr;
    SkipSpaces(input);
    if (input.peek() == ']') {
        input.get();
        return Node(arr);
    }
    
    while (true) {
        arr.push_back(ParseNode(input));
        SkipSpaces(input);
        char next = input.peek();
        if (next == ']') {
            input.get();
            break;
        } else if (next == ',') {
            input.get();
            SkipSpaces(input);
        } else {
            throw ParsingError("Expected ',' or ']'");
        }
    }
    return Node(arr);
}

Node ParseDict(istream& input) {
    Dict dict;
    SkipSpaces(input);
    if (input.peek() == '}') {
        input.get();
        return Node(dict);
    }
    
    while (true) {
        SkipSpaces(input);
        if (input.peek() != '"') {
            throw ParsingError("Expected string key");
        }
        input.get();
        string key = ParseString(input);
        SkipSpaces(input);
        if (input.peek() != ':') {
            throw ParsingError("Expected ':'");
        }
        input.get();
        dict[key] = ParseNode(input);
        SkipSpaces(input);
        char next = input.peek();
        if (next == '}') {
            input.get();
            break;
        } else if (next == ',') {
            input.get();
            SkipSpaces(input);
        } else {
            throw ParsingError("Expected ',' or '}'");
        }
    }
    return Node(dict);
}

void PrintNode(const Node& node, ostream& out) {
    if (node.IsInt()) {
        out << node.AsInt();
    } else if (node.IsPureDouble()) {
        out << node.AsDouble();
    } else if (node.IsBool()) {
        out << (node.AsBool() ? "true" : "false");
    } else if (node.IsNull()) {
        out << "null";
    } else if (node.IsString()) {
        PrintString(node.AsString(), out);
    } else if (node.IsArray()) {
        out << "[";
        const auto& arr = node.AsArray();
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i > 0) out << ", ";
            PrintNode(arr[i], out);
        }
        out << "]";
    } else if (node.IsMap()) {
        out << "{";
        const auto& dict = node.AsMap();
        bool first = true;
        for (const auto& [key, value] : dict) {
            if (!first) out << ", ";
            PrintString(key, out);
            out << ": ";
            PrintNode(value, out);
            first = false;
        }
        out << "}";
    }
}

}  // namespace

// IMPLEMENTATION

Node::Node() : value_(nullptr) {}

Node::Node(Array array) : value_(move(array)) {}
Node::Node(Dict map) : value_(move(map)) {}
Node::Node(int value) : value_(value) {}
Node::Node(double value) : value_(value) {}
Node::Node(bool value) : value_(value) {}
Node::Node(std::string value) : value_(move(value)) {}
Node::Node(std::nullptr_t) : value_(nullptr) {}

bool Node::IsInt() const { return holds_alternative<int>(value_); }
bool Node::IsDouble() const { return IsInt() || IsPureDouble(); }
bool Node::IsPureDouble() const { return holds_alternative<double>(value_); }
bool Node::IsBool() const { return holds_alternative<bool>(value_); }
bool Node::IsString() const { return holds_alternative<string>(value_); }
bool Node::IsNull() const { return holds_alternative<nullptr_t>(value_); }
bool Node::IsArray() const { return holds_alternative<Array>(value_); }
bool Node::IsMap() const { return holds_alternative<Dict>(value_); }

int Node::AsInt() const {
    if (auto p = get_if<int>(&value_)) {
        return *p;
    }
    throw logic_error("Node is not an int");
}

bool Node::AsBool() const {
    if (auto p = get_if<bool>(&value_)) {
        return *p;
    }
    throw logic_error("Node is not a bool");
}

double Node::AsDouble() const {
    if (auto p = get_if<double>(&value_)) {
        return *p;
    }
    if (auto p = get_if<int>(&value_)) {
        return static_cast<double>(*p);
    }
    throw logic_error("Node is not a number");
}

const string& Node::AsString() const {
    if (auto p = get_if<string>(&value_)) {
        return *p;
    }
    throw logic_error("Node is not a string");
}

const Array& Node::AsArray() const {
    if (auto p = get_if<Array>(&value_)) {
        return *p;
    }
    throw logic_error("Node is not an array");
}

const Dict& Node::AsMap() const {
    if (auto p = get_if<Dict>(&value_)) {
        return *p;
    }
    throw logic_error("Node is not a dict");
}

bool Node::operator==(const Node& other) const {
    return value_ == other.value_;
}

bool Node::operator!=(const Node& other) const {
    return !(*this == other);
}

Document::Document(Node root) : root_(move(root)) {}
const Node& Document::GetRoot() const { return root_; }

bool Document::operator==(const Document& other) const {
    return root_ == other.root_;
}

bool Document::operator!=(const Document& other) const {
    return !(*this == other);
}

Document Load(istream& input) {
    return Document(ParseNode(input));
}

void Print(const Document& doc, ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

}  // namespace json