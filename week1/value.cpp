#include <cstdint>
#include <iostream>
#include <regex>
#include <string_view>
#include <variant>
#include <vector>

using namespace std;

/*Note: Valid JSONs require keys to be in "double quotes" ('single quotes' are invalid)
    'Single quotes' are also invalid for strings
    Trailing commas are invalid- this means the last element in an array/object cannot be followed by a comma
    Blackslash\ precedes a literal*/

enum class ValueType : uint8_t {array, boolean, null, number, object, string};

using ArrayValue = vector<JSONValue>;
using ObjectValue = vector<pair<string_view, JSONValue>>;

class JSONValue {
    private:
    ValueType type;
    variant<ArrayValue, bool, nullptr_t, double, ObjectValue, string_view> value;

    public:
    JSONValue(ValueType t, variant<ArrayValue, bool, nullptr_t, double, ObjectValue, string_view> v) : type(t), value(v) {}

    ValueType getType() {
        return type;
    };

    variant<ArrayValue, bool, nullptr_t, double, ObjectValue, string_view> getValue() {
        return value;
    };
};