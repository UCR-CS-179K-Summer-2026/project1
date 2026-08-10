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

inline string unescapeString(const string_view& str) {
    string result;

    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            i++;
            switch (str[i]) {
                case '\"': result += '\"'; break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'u':
                    if (i + 4 < str.size()) {
                        string hex = string(str.substr(i + 1, 4));
                        uint32_t codePoint = static_cast<uint32_t>(stoul(hex, nullptr, 16));
                        i += 4;

                        // High surrogate followed by a low surrogate: combine into one code point
                        if (codePoint >= 0xD800 && codePoint <= 0xDBFF &&
                            i + 6 < str.size() && str[i + 1] == '\\' && str[i + 2] == 'u') {
                            string lowHex = string(str.substr(i + 3, 4));
                            uint32_t low = static_cast<uint32_t>(stoul(lowHex, nullptr, 16));

                            if (low >= 0xDC00 && low <= 0xDFFF) {
                                codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00);
                                i += 6;
                            }
                        }

                        // Encode the code point as UTF-8
                        if (codePoint <= 0x7F) {
                            result += static_cast<char>(codePoint);
                        } else if (codePoint <= 0x7FF) {
                            result += static_cast<char>(0xC0 | (codePoint >> 6));
                            result += static_cast<char>(0x80 | (codePoint & 0x3F));
                        } else if (codePoint <= 0xFFFF) {
                            result += static_cast<char>(0xE0 | (codePoint >> 12));
                            result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (codePoint & 0x3F));
                        } else {
                            result += static_cast<char>(0xF0 | (codePoint >> 18));
                            result += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
                            result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (codePoint & 0x3F));
                        }
                    }

                    break;
                default:
                    continue;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}