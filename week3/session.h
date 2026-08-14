#pragma once

#include "parser.h"
#include "query-parser.h"
#include "value.h"

#include <memory>
#include <string_view>

using namespace std;

struct Session {
    JSONValue file = JSONValue(nullptr);
    bool isInitialized = false;
    string name = "";
    int records = 0;

    void initialize(JSONValue v, string n) {
        file = v;
        name = n;
        
        if(v.getType() == ValueType::Array) {
            records = get<ArrayValue>(v.getValue()).size();
        } else if(v.getType() == ValueType::Null) {
            records = 0;
        } else {
            records = 1;
        }
        
        isInitialized = true;
    }
};