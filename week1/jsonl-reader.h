#pragma once

#include <string>
#include <vector>

#include "json_value.h"

// Reads a .jsonl file (one JSON object per line) and returns the parsed
// records, in order.
//
// TODO: decide + document what happens on:
//   - a blank line (skip it? error?)
//   - a line that fails to parse (skip + warn? abort the whole read?)
std::vector<JsonValue> readJsonlFile(const std::string& path);

// Result of looking up a path like ".student.name" or ".scores[0]" inside
// a JsonValue. Exactly one of {value, error} is meaningful, based on ok.
struct LookupResult {
    bool ok;
    JsonValue value;    // valid when ok == true
    std::string error;  // valid when ok == false, e.g. "field 'name' not found"
};

// Looks up a dotted/bracketed path inside a JsonValue.
// Supported path syntax (see PLAN.md JSON Examples):
//   .name
//   .student.name
//   .scores[0]
LookupResult lookup(const JsonValue& value, const std::string& path);

// Formats a LookupResult the way it should appear on the terminal,
// e.g. strings get quotes, numbers don't, errors are readable.
std::string formatResult(const LookupResult& result);
