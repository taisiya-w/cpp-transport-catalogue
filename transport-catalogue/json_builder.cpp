#include "json_builder.h"
#include <stdexcept>

using namespace std::literals;

namespace json {

void Builder::StartDictInternal() {
    if (IsReady()) {
        throw std::logic_error("StartDict() called on ready builder"s);
    }
    
    holders_stack_.push(std::make_unique<DictHolder>());
}

void Builder::StartArrayInternal() {
    if (IsReady()) {
        throw std::logic_error("StartArray() called on ready builder"s);
    }
    
    holders_stack_.push(std::make_unique<ArrayHolder>());
}

void Builder::KeyInternal(std::string key) {
    if (!IsInsideDict()) {
        throw std::logic_error("Key() called outside of dict"s);
    }
    
    auto& dict_holder = GetTopDict();
    if (dict_holder.has_key) {
        throw std::logic_error("Key() called twice without Value()"s);
    }
    
    dict_holder.pending_key = std::move(key);
    dict_holder.has_key = true;
}

void Builder::ValueInternal(Node::Value value) {
    if (IsReady()) {
        throw std::logic_error("Value() called on ready builder"s);
    }
    
    Node node = std::visit([](auto&& arg) -> Node {
        return Node(std::forward<decltype(arg)>(arg));
    }, std::move(value));
    
    AddValue(std::move(node));
}

void Builder::EndDictInternal() {
    if (!IsInsideDict()) {
        throw std::logic_error("EndDict() called not inside dict"s);
    }
    
    auto& dict_holder = GetTopDict();
    if (dict_holder.has_key) {
        throw std::logic_error("EndDict() called with pending Key()"s);
    }
    
    Node dict_node(std::move(dict_holder.dict));
    holders_stack_.pop();
    AddValue(std::move(dict_node));
}

void Builder::EndArrayInternal() {
    if (!IsInsideArray()) {
        throw std::logic_error("EndArray() called not inside array"s);
    }
    
    auto& array_holder = GetTopArray();
    Node array_node(std::move(array_holder.array));
    holders_stack_.pop();
    AddValue(std::move(array_node));
}

DictItemContext Builder::StartDict() {
    StartDictInternal();
    return DictItemContext(*this);
}

ArrayItemContext Builder::StartArray() {
    StartArrayInternal();
    return ArrayItemContext(*this);
}

KeyContext Builder::Key(std::string key) {
    KeyInternal(std::move(key));
    return KeyContext(*this);
}

Builder& Builder::Value(Node::Value value) {
    ValueInternal(std::move(value));
    return *this;
}

Builder& Builder::Value(int value) {
    return Value(Node::Value(value));
}

Builder& Builder::Value(double value) {
    return Value(Node::Value(value));
}

Builder& Builder::Value(bool value) {
    return Value(Node::Value(value));
}

Builder& Builder::Value(const char* value) {
    return Value(std::string(value));
}

Builder& Builder::Value(std::string value) {
    return Value(Node::Value(std::move(value)));
}

Builder& Builder::Value(std::nullptr_t) {
    return Value(Node::Value(nullptr));
}

Builder& Builder::Value(Array value) {
    return Value(Node::Value(std::move(value)));
}

Builder& Builder::EndDict() {
    EndDictInternal();
    return *this;
}

Builder& Builder::EndArray() {
    EndArrayInternal();
    return *this;
}

Node Builder::Build() {
    if (!IsReady()) {
        throw std::logic_error("Build() called on incomplete JSON"s);
    }
    
    if (!holders_stack_.empty()) {
        throw std::logic_error("Build() called with unclosed containers"s);
    }
    
    return std::move(*root_);
}

bool Builder::IsInsideDict() const {
    if (holders_stack_.empty()) {
        return false;
    }
    return dynamic_cast<const DictHolder*>(holders_stack_.top().get()) != nullptr;
}

bool Builder::IsInsideArray() const {
    if (holders_stack_.empty()) {
        return false;
    }
    return dynamic_cast<const ArrayHolder*>(holders_stack_.top().get()) != nullptr;
}

Builder::DictHolder& Builder::GetTopDict() {
    return *static_cast<DictHolder*>(holders_stack_.top().get());
}

Builder::ArrayHolder& Builder::GetTopArray() {
    return *static_cast<ArrayHolder*>(holders_stack_.top().get());
}

void Builder::AddValue(Node value) {
    if (holders_stack_.empty()) {
        root_ = std::make_unique<Node>(std::move(value));
    } else if (IsInsideArray()) {
        GetTopArray().array.emplace_back(std::move(value));
    } else if (IsInsideDict()) {
        auto& dict_holder = GetTopDict();
        if (!dict_holder.has_key) {
            throw std::logic_error("Value added to dict without Key()"s);
        }
        dict_holder.dict[std::move(dict_holder.pending_key)] = std::move(value);
        dict_holder.has_key = false;
        dict_holder.pending_key.clear();
    }
}

KeyContext DictItemContext::Key(std::string key) {
    builder_.KeyInternal(std::move(key));
    return KeyContext(builder_);
}

Builder& DictItemContext::EndDict() {
    builder_.EndDictInternal();
    return builder_;
}

ArrayItemContext ArrayItemContext::Value(Node::Value value) {
    builder_.ValueInternal(std::move(value));
    return ArrayItemContext(builder_);
}

ArrayItemContext ArrayItemContext::Value(int value) {
    builder_.Value(value);
    return ArrayItemContext(builder_);
}

ArrayItemContext ArrayItemContext::Value(double value) {
    builder_.Value(value);
    return ArrayItemContext(builder_);
}

ArrayItemContext ArrayItemContext::Value(bool value) {
    builder_.Value(value);
    return ArrayItemContext(builder_);
}

ArrayItemContext ArrayItemContext::Value(const char* value) {
    builder_.Value(value);
    return ArrayItemContext(builder_);
}

ArrayItemContext ArrayItemContext::Value(std::string value) {
    builder_.Value(std::move(value));
    return ArrayItemContext(builder_);
}

ArrayItemContext ArrayItemContext::Value(std::nullptr_t) {
    builder_.Value(nullptr);
    return ArrayItemContext(builder_);
}

ArrayItemContext ArrayItemContext::Value(Array value) {
    builder_.Value(std::move(value));
    return ArrayItemContext(builder_);
}

DictItemContext ArrayItemContext::StartDict() {
    builder_.StartDictInternal();
    return DictItemContext(builder_);
}

ArrayItemContext ArrayItemContext::StartArray() {
    builder_.StartArrayInternal();
    return ArrayItemContext(builder_);
}

Builder& ArrayItemContext::EndArray() {
    builder_.EndArrayInternal();
    return builder_;
}

DictItemContext KeyContext::Value(Node::Value value) {
    builder_.ValueInternal(std::move(value));
    return DictItemContext(builder_);
}

DictItemContext KeyContext::Value(int value) {
    builder_.Value(value);
    return DictItemContext(builder_);
}

DictItemContext KeyContext::Value(double value) {
    builder_.Value(value);
    return DictItemContext(builder_);
}

DictItemContext KeyContext::Value(bool value) {
    builder_.Value(value);
    return DictItemContext(builder_);
}

DictItemContext KeyContext::Value(const char* value) {
    builder_.Value(value);
    return DictItemContext(builder_);
}

DictItemContext KeyContext::Value(std::string value) {
    builder_.Value(std::move(value));
    return DictItemContext(builder_);
}

DictItemContext KeyContext::Value(std::nullptr_t) {
    builder_.Value(nullptr);
    return DictItemContext(builder_);
}

DictItemContext KeyContext::Value(Array value) {
    builder_.Value(std::move(value));
    return DictItemContext(builder_);
}

DictItemContext KeyContext::StartDict() {
    builder_.StartDictInternal();
    return DictItemContext(builder_);
}

ArrayItemContext KeyContext::StartArray() {
    builder_.StartArrayInternal();
    return ArrayItemContext(builder_);
}

} // namespace json