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

    void initialize(JSONValue v) {
        file = v;
        isInitialized = true;
    }
};