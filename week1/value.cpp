#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace std;

/*Note: Valid JSONs require keys to be in "double quotes" ('single quotes' are invalid)
    'Single quotes' are also invalid for strings
    Trailing commas are invalid- this means the last element in an array/object cannot be followed by a comma
    Blackslash\ precedes a literal*/

enum class ValueType : uint8_t {Array, Boolean, Null, Number, Object, String};

class JSONValue;

using ArrayValue = vector<JSONValue>;
using ObjectValue = vector<pair<string, JSONValue>>;

class JSONValue {
    private:
    ValueType type;
    variant<ArrayValue, bool, nullptr_t, double, ObjectValue, string> value;

    public:
    explicit JSONValue(ArrayValue a) : type(ValueType::Array), value(move(a)) {}
    explicit JSONValue(bool b) : type(ValueType::Boolean), value(b) {}
    explicit JSONValue(nullptr_t n) : type(ValueType::Null), value(nullptr) {}
    explicit JSONValue(double d) : type(ValueType::Number), value(d) {}
    explicit JSONValue(ObjectValue o) : type(ValueType::Object), value(move(o)) {}
    explicit JSONValue(string s) : type(ValueType::String), value(move(s)) {}

    ValueType getType() const { return type; }
    const variant<ArrayValue, bool, nullptr_t, double, ObjectValue, string>& getValue() const { return value; }
};