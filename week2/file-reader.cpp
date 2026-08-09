// Person 2's contribution: reads a .json or .jsonl file into JSONValue
// records via Person 1's Parser, and implements path lookup/formatting
// against the resulting tree.
#include "file-reader.h"

vector<JSONValue> readFile(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        throw runtime_error(
            "Could not open '" + path + "'. Check that the path is spelled "
            "correctly, that the file exists, and that you're running the "
            "program from the directory you expect (try an absolute path "
            "if you're not sure).");
    }

    filesystem::path filepath(path);
    string extension = filepath.extension().string();

    transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return tolower(c);
    });

    ostringstream buffer;
    buffer << file.rdbuf();
    string contents = buffer.str();

    if(extension == ".jsonl") {
        return readJsonlFile(contents);
    } else {
        return readJsonFile(contents);
    }
}

vector<JSONValue> readJsonlFile(const string& contents) {
    vector<JSONValue> records;
    string line;
    int lineNumber = 0;

    istringstream stream(contents);

    try {
        while (getline(stream, line)) {
            ++lineNumber;

            if (line.find_first_not_of(" \t\r\n") == string::npos) {
                continue;
            }

            Parser parser(line, ParserType::JSONL);
            records.push_back(parser.parse());
        }
    } catch (const exception& e) {
        cerr << "Warning: failed to parse (" << e.what() << " at line " << lineNumber << ")\n";
        return {};
    }

    return records;
}

vector<JSONValue> readJsonFile(const string& contents) {
    vector<JSONValue> records;

    try {
        Parser parser(contents, ParserType::JSON);
        JSONValue doc = parser.parse();
        if (doc.getType() == ValueType::Array) {
            records = get<ArrayValue>(doc.getValue());
        } else {
            records.push_back(move(doc));
        }
    } catch (const exception& e) {
        cerr << "Warning: failed to parse JSON file (" << e.what() << ")\n";
    }

    return records;
}

namespace {

LookupResult error(const string& message) {
    return {false, nullptr, message};
}

string formatValue(const JSONValue& value) {
    switch (value.getType()) {
        case ValueType::Null:
            return "null";
        case ValueType::Boolean:
            return get<bool>(value.getValue()) ? "true" : "false";
        case ValueType::Number: {
            ostringstream out;
            out << get<double>(value.getValue());
            return out.str();
        }
        case ValueType::String:
            return "\"" + get<string>(value.getValue()) + "\"";
        case ValueType::Object: {
            const auto& obj = get<ObjectValue>(value.getValue());
            ostringstream out;
            out << "{";
            for (size_t i = 0; i < obj.size(); ++i) {
                if (i > 0) out << ",";
                out << "\"" << obj[i].first << "\":" << formatValue(obj[i].second);
            }
            out << "}";
            return out.str();
        }
        case ValueType::Array: {
            const auto& arr = get<ArrayValue>(value.getValue());
            ostringstream out;
            out << "[";
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) out << ",";
                out << formatValue(arr[i]);
            }
            out << "]";
            return out.str();
        }
    }
    return "null";
}

}  // namespace

//Roughly rewritten to support GET("propName") syntax.
LookupResult get(const JSONValue& value, const vector<string_view>& path) {
    const JSONValue* current = &value;

    for (const string_view& prop : path) {
        if (current->getType() == ValueType::Object) {
            const auto& obj = get<ObjectValue>(current->getValue());

            const JSONValue* found = nullptr;
            for (const auto& entry : obj) {
                if (unescapeString(entry.first) == unescapeString(prop)) {
                    found = &entry.second;
                    break;
                }
            }
            if (!found) {
                return error("field '" + string(prop) + "' not found");
            }
            current = found;

        } else if (current->getType() == ValueType::Array) {
            string indexStr(prop);
            size_t parsedLen = 0;
            int index;
            try {
                index = stoi(indexStr, &parsedLen);
            } catch (const exception&) {
                return error("expected a numeric index to access an array, got '" + indexStr + "'");
            }
            if (parsedLen != indexStr.size()) {
                return error("expected a numeric index to access an array, got '" + indexStr + "'");
            }

            const auto& arr = get<ArrayValue>(current->getValue());
            if (index < 0 || static_cast<size_t>(index) >= arr.size()) {
                return error("index " + indexStr + " out of range");
            }
            current = &arr[static_cast<size_t>(index)];

        } else {
            return error("cannot look up '" + string(prop) + "' on a non-object, non-array value");
        }
    }

    return {true, current, ""};
}

LookupResult get(const JSONValue& value, const string& path) {
    vector<pair<bool, string>> parts;
    size_t i = 0;

    while (i < path.size()) {
        if (path[i] == '.') {
            ++i;
            size_t start = i;
            while (i < path.size() && path[i] != '.' && path[i] != '[') ++i;
            parts.push_back({false, path.substr(start, i - start)});
        } else if (path[i] == '[') {
            ++i;
            size_t start = i;
            while (i < path.size() && path[i] != ']') ++i;
            if (i >= path.size()) {
                return error("missing closing ']' in path");
            }
            parts.push_back({true, path.substr(start, i - start)});
            ++i;
        } else {
            return error("invalid path syntax at position " + to_string(i));
        }
    }

    const JSONValue* current = &value;

    for (const auto& part : parts) {
        const string& text = part.second;

        if (!part.first) {
            if (current->getType() != ValueType::Object) {
                return error("expected an object to look up field '" + text + "'");
            }
            const auto& obj = get<ObjectValue>(current->getValue());

            const JSONValue* found = nullptr;
            for (const auto& entry : obj) {
                if (entry.first == text) {
                    found = &entry.second;
                    break;
                }
            }
            if (!found) {
                return error("field '" + text + "' not found");
            }
            current = found;
        } else {
            size_t parsedLen = 0;

            int index;
            try {
                index = stoi(text, &parsedLen);
            } catch (const exception&) {
                return error("invalid array index '" + text + "'");
            }
            if (parsedLen != text.size()) {
                return error("invalid array index '" + text + "'");
            }

            if (current->getType() != ValueType::Array) {
                return error("expected an array to index with [" + text + "]");
            }
            const auto& arr = get<ArrayValue>(current->getValue());

            if (index < 0 || static_cast<size_t>(index) >= arr.size()) {
                return error("index " + text + " out of range");
            }
            current = &arr[static_cast<size_t>(index)];
        }
    }

    return {true, current, ""};
}
string formatResult(const LookupResult& result) {
    if (!result.ok) {
        return "Error: " + result.error;
    }
    return formatValue(*result.value);
}
