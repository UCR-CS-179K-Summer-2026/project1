#pragma once

#include "parser.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

vector<JSONValue> readFile(const string& path);

// Reads a .jsonl file (one JSON object per line) and returns the parsed
// records, in order, as an owned tree per line (JSONValue is move-only).
//
// - A blank/whitespace-only line is stored as a JSONNull.
// - A line that fails to parse prints a warning (with line number) and is
//   also stored as a JSONNull.
// Both cases keep records[i] aligned with line i in the file.
vector<JSONValue> readJsonlFile(const string& contents);
vector<JSONValue> readJsonFile(const string& contents);

// Result of looking up a path like ".student.name" or ".scores[0]" inside
// a JSONValue tree. Exactly one of {value, error} is meaningful, based on
// ok. `value` is a non-owning pointer into the tree passed to lookup() —
// it's only valid for as long as that tree is alive.
struct LookupResult {
    bool ok;
    const JSONValue* value;  // valid when ok == true
    string error;            // valid when ok == false, e.g. "field 'name' not found"
};

// Looks up a dotted/bracketed path inside a JSONValue tree.
// Supported path syntax (see PLAN.md JSON Examples):
//   .name
//   .student.name
//   .scores[0]
LookupResult lookup(const JSONValue& value, const string& path);

// Formats a LookupResult the way it should appear on the terminal,
// e.g. strings get quotes, numbers don't, errors are readable.
string formatResult(const LookupResult& result);
