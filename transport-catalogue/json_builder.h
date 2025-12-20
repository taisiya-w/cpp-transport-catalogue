#pragma once

#include "json.h"
#include <memory>
#include <stack>
#include <string>

namespace json {

class Builder;
class DictItemContext;
class ArrayItemContext;
class KeyContext;

class Builder {
private:
    struct BaseHolder {
        virtual ~BaseHolder() = default;
    };
    
    struct DictHolder : BaseHolder {
        Dict dict;
        std::string pending_key;
        bool has_key = false;
    };
    
    struct ArrayHolder : BaseHolder {
        Array array;
    };

public:
    Builder() = default;
    
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    
    KeyContext Key(std::string key);
    
    Builder& Value(Node::Value value);
    Builder& Value(int value);
    Builder& Value(double value);
    Builder& Value(bool value);
    Builder& Value(const char* value);
    Builder& Value(std::string value);
    Builder& Value(std::nullptr_t);
    Builder& Value(Array value);
    
    Builder& EndDict();
    Builder& EndArray();
    
    Node Build();

private:
    std::unique_ptr<Node> root_;
    std::stack<std::unique_ptr<BaseHolder>> holders_stack_;
    
    friend class DictItemContext;
    friend class ArrayItemContext;
    friend class KeyContext;
    
    bool IsInsideDict() const;
    bool IsInsideArray() const;
    DictHolder& GetTopDict();
    ArrayHolder& GetTopArray();
    bool IsReady() const { return root_ != nullptr; }
    void AddValue(Node value);
    void StartDictInternal();
    void StartArrayInternal();
    void KeyInternal(std::string key);
    void ValueInternal(Node::Value value);
    void EndDictInternal();
    void EndArrayInternal();
};

class DictItemContext {
public:
    explicit DictItemContext(Builder& builder) : builder_(builder) {}
    
    KeyContext Key(std::string key);
    Builder& EndDict();
    
    DictItemContext Value(int value) = delete;
    DictItemContext Value(double value) = delete;
    DictItemContext Value(bool value) = delete;
    DictItemContext Value(const char* value) = delete;
    DictItemContext Value(std::string value) = delete;
    DictItemContext Value(std::nullptr_t) = delete;
    DictItemContext Value(Array value) = delete;
    DictItemContext Value(Node::Value value) = delete;
    
    DictItemContext StartDict() = delete;
    ArrayItemContext StartArray() = delete;
    Builder& EndArray() = delete;
    
private:
    Builder& builder_;
};

class ArrayItemContext {
public:
    explicit ArrayItemContext(Builder& builder) : builder_(builder) {}
    
    ArrayItemContext Value(Node::Value value);
    ArrayItemContext Value(int value);
    ArrayItemContext Value(double value);
    ArrayItemContext Value(bool value);
    ArrayItemContext Value(const char* value);
    ArrayItemContext Value(std::string value);
    ArrayItemContext Value(std::nullptr_t);
    ArrayItemContext Value(Array value);
    
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndArray();
    
    KeyContext Key(std::string key) = delete;
    Builder& EndDict() = delete;
    
private:
    Builder& builder_;
};

class KeyContext {
public:
    explicit KeyContext(Builder& builder) : builder_(builder) {}
    
    DictItemContext Value(Node::Value value);
    DictItemContext Value(int value);
    DictItemContext Value(double value);
    DictItemContext Value(bool value);
    DictItemContext Value(const char* value);
    DictItemContext Value(std::string value);
    DictItemContext Value(std::nullptr_t);
    DictItemContext Value(Array value);
    
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    
    KeyContext Key(std::string key) = delete;
    Builder& EndDict() = delete;
    Builder& EndArray() = delete;
    
private:
    Builder& builder_;
};

} // namespace json