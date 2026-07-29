#include "jsonl-reader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::vector<JSONValuePtr> readJsonlFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(
            "Could not open '" + path + "'. Check that the path is spelled "
            "correctly, that the file exists, and that you're running the "
            "program from the directory you expect (try an absolute path "
            "if you're not sure).");
    }

    std::vector<JSONValuePtr> records;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;

        // Blank line (or whitespace-only): store null rather than skipping,
        // so records[i] always corresponds to line i in the file.
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            records.push_back(std::make_unique<JSONNull>());
            continue;
        }

        // Parser copies string data into each JSONValue node's own
        // std::string as it builds the tree, so the tree doesn't depend on
        // `line` surviving past this iteration (unlike the old string_view
        // design, this needs no extra buffer bookkeeping).
        Parser parser(line);

        try {
            records.push_back(parser.parse());
        } catch (const std::exception& e) {
            std::cerr << "Warning: line " << lineNumber
                      << ": failed to parse (" << e.what() << "), "
                      << "storing as null.\n";
            records.push_back(std::make_unique<JSONNull>());
        }
    }

    return records;
}

namespace {

LookupResult error(const std::string& message) {
    return {false, nullptr, message};
}

std::string formatValue(const JSONValue& value) {
    switch (value.getType()) {
        case ValueType::Null:
            return "null";
        case ValueType::Boolean:
            return static_cast<const JSONBoolean&>(value).value() ? "true" : "false";
        case ValueType::Number: {
            std::ostringstream out;
            out << static_cast<const JSONNumber&>(value).value();
            return out.str();
        }
        case ValueType::String:
            return "\"" + static_cast<const JSONString&>(value).value() + "\"";
        case ValueType::Object: {
            const auto& obj = static_cast<const JSONObject&>(value).entries();
            std::ostringstream out;
            out << "{";
            for (size_t i = 0; i < obj.size(); ++i) {
                if (i > 0) out << ",";
                out << "\"" << obj[i].first << "\":" << formatValue(*obj[i].second);
            }
            out << "}";
            return out.str();
        }
        case ValueType::Array: {
            const auto& arr = static_cast<const JSONArray&>(value).items();
            std::ostringstream out;
            out << "[";
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) out << ",";
                out << formatValue(*arr[i]);
            }
            out << "]";
            return out.str();
        }
    }
    return "null";
}

}  // namespace

LookupResult lookup(const JSONValue& value, const std::string& path) {
    const JSONValue* current = &value;
    size_t i = 0;

    while (i < path.size()) {
        if (path[i] == '.') {
            ++i;
            size_t start = i;
            while (i < path.size() && path[i] != '.' && path[i] != '[') ++i;
            std::string field = path.substr(start, i - start);

            if (!current->isObject()) {
                return error("expected an object to look up field '" + field + "'");
            }
            const auto& obj = static_cast<const JSONObject&>(*current).entries();

            const JSONValue* found = nullptr;
            for (const auto& entry : obj) {
                if (entry.first == field) {
                    found = entry.second.get();
                    break;
                }
            }
            if (!found) {
                return error("field '" + field + "' not found");
            }
            current = found;

        } else if (path[i] == '[') {
            ++i;
            size_t start = i;
            while (i < path.size() && path[i] != ']') ++i;
            if (i >= path.size()) {
                return error("missing closing ']' in path");
            }
            std::string indexStr = path.substr(start, i - start);
            ++i;  // skip ']'

            int index;
            try {
                index = std::stoi(indexStr);
            } catch (const std::exception&) {
                return error("invalid array index '" + indexStr + "'");
            }

            if (!current->isArray()) {
                return error("expected an array to index with [" + indexStr + "]");
            }
            const auto& arr = static_cast<const JSONArray&>(*current).items();

            if (index < 0 || static_cast<size_t>(index) >= arr.size()) {
                return error("index " + indexStr + " out of range");
            }
            current = arr[static_cast<size_t>(index)].get();

        } else {
            return error("invalid path syntax at position " + std::to_string(i));
        }
    }

    return {true, current, ""};
}

std::string formatResult(const LookupResult& result) {
    if (!result.ok) {
        return "Error: " + result.error;
    }
    return formatValue(*result.value);
}
