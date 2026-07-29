#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace std;

/*Note: Valid JSONs require keys to be in "double quotes" ('single quotes' are invalid)
    'Single quotes' are also invalid for strings
    Trailing commas are invalid- this means the last element in an array/object cannot be followed by a comma
    Blackslash\ precedes a literal*/

enum class ValueType : uint8_t {Array, Boolean, Null, Number, Object, String};

// Polymorphic JSON value: one abstract base plus one concrete leaf/container
// class per ValueType. Each node is heap-allocated (via JSONValuePtr) and
// only holds the data it actually needs — no unused fields, no variant.
// Tree ownership is via unique_ptr, so a JSONValue tree is move-only; code
// that needs to look inside a tree it doesn't own should hold a
// `const JSONValue*`/`const JSONValue&`, not a copy.
class JSONValue {
    public:
    virtual ~JSONValue() = default;
    virtual ValueType getType() const = 0;

    bool isNull() const   { return getType() == ValueType::Null; }
    bool isBool() const   { return getType() == ValueType::Boolean; }
    bool isNumber() const { return getType() == ValueType::Number; }
    bool isString() const { return getType() == ValueType::String; }
    bool isObject() const { return getType() == ValueType::Object; }
    bool isArray() const  { return getType() == ValueType::Array; }
};

using JSONValuePtr = unique_ptr<JSONValue>;

class JSONNull : public JSONValue {
    public:
    ValueType getType() const override { return ValueType::Null; }
};

class JSONBoolean : public JSONValue {
    private:
    bool value_;

    public:
    explicit JSONBoolean(bool v) : value_(v) {}
    ValueType getType() const override { return ValueType::Boolean; }
    bool value() const { return value_; }
};

class JSONNumber : public JSONValue {
    private:
    double value_;

    public:
    explicit JSONNumber(double v) : value_(v) {}
    ValueType getType() const override { return ValueType::Number; }
    double value() const { return value_; }
};

class JSONString : public JSONValue {
    private:
    string value_;

    public:
    explicit JSONString(string v) : value_(std::move(v)) {}
    ValueType getType() const override { return ValueType::String; }
    const string& value() const { return value_; }
};

class JSONArray : public JSONValue {
    private:
    vector<JSONValuePtr> items_;

    public:
    explicit JSONArray(vector<JSONValuePtr> items) : items_(std::move(items)) {}
    ValueType getType() const override { return ValueType::Array; }
    const vector<JSONValuePtr>& items() const { return items_; }
};

class JSONObject : public JSONValue {
    private:
    vector<pair<string, JSONValuePtr>> entries_;

    public:
    explicit JSONObject(vector<pair<string, JSONValuePtr>> entries) : entries_(std::move(entries)) {}
    ValueType getType() const override { return ValueType::Object; }
    const vector<pair<string, JSONValuePtr>>& entries() const { return entries_; }
};
