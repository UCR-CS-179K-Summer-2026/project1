// STUB interface for Person 1's JsonValue type.
//
// Person 1 owns the real implementation of JsonValue and parseJson().
// This file exists so Person 2 (JSONL reader / lookup engine) can compile
// and test against a real type *now*, instead of waiting on Person 1's branch.
//
// When Person 1's version lands, drop this file and #include theirs instead —
// as long as the public shape (type names + method names below) matches,
// nothing in jsonl-reader.cpp should need to change.
#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

class JsonValue {
public:
    using Object = std::map<std::string, JsonValue>;
    using Array = std::vector<JsonValue>;

    enum class Type { Null, Bool, Number, String, Object, Array };

    JsonValue() : data_(nullptr) {}
    JsonValue(std::nullptr_t) : data_(nullptr) {}
    JsonValue(bool b) : data_(b) {}
    JsonValue(double n) : data_(n) {}
    JsonValue(const std::string& s) : data_(s) {}
    JsonValue(const char* s) : data_(std::string(s)) {}
    JsonValue(Object o) : data_(std::move(o)) {}
    JsonValue(Array a) : data_(std::move(a)) {}

    Type type() const { return static_cast<Type>(data_.index()); }

    bool isNull() const   { return type() == Type::Null; }
    bool isBool() const   { return type() == Type::Bool; }
    bool isNumber() const { return type() == Type::Number; }
    bool isString() const { return type() == Type::String; }
    bool isObject() const { return type() == Type::Object; }
    bool isArray() const  { return type() == Type::Array; }

    bool asBool() const              { return std::get<bool>(data_); }
    double asNumber() const          { return std::get<double>(data_); }
    const std::string& asString() const { return std::get<std::string>(data_); }
    const Object& asObject() const   { return std::get<Object>(data_); }
    const Array& asArray() const     { return std::get<Array>(data_); }

private:
    std::variant<std::nullptr_t, bool, double, std::string, Object, Array> data_;
};

// Person 1 will provide the real definition of this in their .cpp.
// Declared here so readJsonlFile() has something to call against; until
// Person 1's branch is merged in, anything that actually calls parseJson()
// will fail to *link* (not compile) — that's expected.
JsonValue parseJson(const std::string& text);
