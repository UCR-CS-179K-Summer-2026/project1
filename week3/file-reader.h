#pragma once

#include "parser.h"
#include "query-parser.h"
#include "session.h"
#include "value.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace std;

void uploadFile(const string& path, Session& session);
void uploadQuery(const string& query, Session& session);

vector<JSONValue> readFile(const string& path);

// Reads a .jsonl file (one JSON object per line) and returns the parsed
// records, in order, as an owned tree per line (JSONValue is move-only).
//
// - A blank/whitespace-only line is skipped entirely (no record is added
//   for it), so records[i] does NOT correspond to line i in the file.
// - A line that fails to parse prints a warning (with line number) to
//   stderr and aborts the read early: the records parsed so far are
//   discarded and an empty vector is returned, rather than skipping just
//   that line.
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

string excecuteQuery(Session& session, string query);

// Looks up a property (or nested path of properties/indices) inside a
// JSONValue tree, per the arguments to a GET(...) call, e.g.:
//   GET()                    -> path == {}                (returns value itself)
//   GET("name")              -> path == {"name"}
//   GET("student", "name")   -> path == {"student", "name"}
//   GET("scores", "0")       -> path == {"scores", "0"}
// Each element of path is looked up as an object field name when the
// current value is an object, or as a numeric array index when the
// current value is an array.
LookupResult get(const JSONValue& value, const vector<string_view>& path);
LookupResult get(const JSONValue& value, const string& path);

// Formats a LookupResult the way it should appear on the terminal,
// e.g. strings get quotes, numbers don't, errors are readable.
string formatResult(const LookupResult& result);
