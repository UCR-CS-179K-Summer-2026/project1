#include "jsonl-reader.h"

#include <fstream>
#include <sstream>

std::vector<JsonValue> readJsonlFile(const std::string& path) {
    std::ifstream file(path);
    // TODO: handle the file-not-found / can't-open case. Throw? Return empty
    // and let the caller check? Decide and document it here.

    std::vector<JsonValue> records;
    std::string line;

    while (std::getline(file, line)) {
        // TODO: skip blank lines here, or is an empty line a malformed record?

        // TODO: send `line` into Person 1's parser:
        //   JsonValue record = parseJson(line);
        //   records.push_back(std::move(record));
        //
        // TODO: what happens if parseJson() fails on one line? Options:
        //   - let it throw and crash the whole read
        //   - catch it, print a warning, and skip that line
        //   - catch it and store some kind of "error record"
        // Pick one and make it consistent with lookup()'s error handling below.
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
