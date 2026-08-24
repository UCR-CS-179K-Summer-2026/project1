#pragma once

#include "file-parser.h"
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

// Result of looking up a path like ".student.name" or ".scores[0]" inside
// a JSONValue tree. Exactly one of {value, error} is meaningful, based on
// ok. `value` is a non-owning pointer into the tree passed to lookup() —
// it's only valid for as long as that tree is alive.
struct LookupResult {
    bool ok;
    const JSONValue* value;  // valid when ok == true
    string error;            // valid when ok == false, e.g. "field 'name' not found"
};

void uploadFile(const string& path, Session& session);
void uploadQuery(const string& query, Session& session);

string executeQuery(Session& session, const string& query);

// Looks up a property (or nested path of properties/indices) inside a
// JSONValue tree, per the arguments to a GET(...) call, e.g.:
//   GET()                    -> path == {}                (returns value itself)
//   GET("name")              -> path == {"name"}
//   GET("student", "name")   -> path == {"student", "name"}
//   GET("scores", "0")       -> path == {"scores", "0"}
// Each element of path is looked up as an object field name when the
// current value is an object, or as a numeric array index when the
// current value is an array.
//LookupResult get(const JSONValue& value, const string& path);
LookupResult get(const JSONValue& value, const vector<string_view>& path);

// Formats a LookupResult the way it should appear on the terminal,
// e.g. strings get quotes, numbers don't, errors are readable.
string formatResult(const LookupResult& result);
