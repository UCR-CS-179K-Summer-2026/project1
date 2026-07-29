#include "jsonl-reader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::vector<JsonValue> readJsonlFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(
            "Could not open '" + path + "'. Check that the path is spelled "
            "correctly, that the file exists, and that you're running the "
            "program from the directory you expect (try an absolute path "
            "if you're not sure).");
    }

    std::vector<JsonValue> records;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;

        // Blank line (or whitespace-only): store null rather than skipping,
        // so records[i] always corresponds to line i in the file.
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            records.push_back(JsonValue(nullptr));
            continue;
        }

        try {
            records.push_back(parseJson(line));
        } catch (const std::exception& e) {
            std::cerr << "Warning: line " << lineNumber
                      << ": failed to parse (" << e.what() << "), "
                      << "storing as null.\n";
            records.push_back(JsonValue(nullptr));
        }
    }

    return records;
}

LookupResult lookup(const JsonValue& value, const std::string& path) {
    // TODO: parse `path` into a sequence of steps, e.g.
    //   ".student.name"  -> [Field("student"), Field("name")]
    //   ".scores[0]"     -> [Field("scores"), Index(0)]
    //
    // A reasonable approach:
    //   1. Walk the string character by character.
    //   2. On '.', read until the next '.' or '[' -> that's a field name.
    //   3. On '[', read digits until ']' -> that's an array index.
    //   4. Apply each step against the current JsonValue, starting from `value`:
    //        - Field(name): current must be isObject(); look up `name` in asObject().
    //          Not found -> return {false, {}, "field 'name' not found"}.
    //        - Index(i): current must be isArray(); check i is in range.
    //          Out of range -> return {false, {}, "index i out of range"}.
    //   5. If a step doesn't match the current value's type (e.g. indexing
    //      into an object), that's also an error worth a clear message.

    (void)value;
    (void)path;
    return {false, JsonValue(), "lookup() not implemented yet"};
}

std::string formatResult(const LookupResult& result) {
    if (!result.ok) {
        // TODO: what should error output look like on the terminal?
        return "Error: " + result.error;
    }

    // TODO: format result.value based on its type:
    //   - String  -> wrap in quotes, e.g. "Ryan"
    //   - Number  -> plain number (watch out for trailing .0 on whole numbers)
    //   - Bool    -> "true" / "false"
    //   - Null    -> "null"
    //   - Object/Array -> probably print as JSON (nested formatting — can be
    //     minimal for Sprint 1, e.g. {"a":1,"b":2})
    std::ostringstream out;
    out << "TODO: format the value here";
    return out.str();
}
