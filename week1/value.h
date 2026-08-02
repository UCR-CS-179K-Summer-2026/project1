// This file is meant to define the JSONValue class, which is a variant type that can hold any of the six JSON value types: array, boolean, null, number, object, or string. 
// It also defines the ValueType enum to represent the type of a JSON value.
// The JSONValue class uses std::variant to hold the actual value, and provides constructors for each type, as well as methods to get the type and value.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace std;

enum class ValueType : uint8_t {Array, Boolean, Null, Number, Object, String};

class JSONValue;

using ArrayValue = vector<JSONValue>;
using ObjectValue = vector<pair<string, JSONValue>>;

class JSONValue {
    private:
    variant<ArrayValue, bool, nullptr_t, double, ObjectValue, string> value;

    public:
    explicit JSONValue(ArrayValue a) : value(move(a)) {}
    explicit JSONValue(bool b) : value(b) {}
    explicit JSONValue(nullptr_t n) : value(nullptr) {}
    explicit JSONValue(double d) : value(d) {}
    explicit JSONValue(ObjectValue o) : value(move(o)) {}
    explicit JSONValue(string s) : value(move(s)) {}

    ValueType getType() const { return static_cast<ValueType>(value.index()); }
    const variant<ArrayValue, bool, nullptr_t, double, ObjectValue, string>& getValue() const { return value; }
};