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

//Remember order of these matters & corresponds to the order in the variant
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