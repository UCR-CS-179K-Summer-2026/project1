#include "integration.h"

#include <algorithm>
#include <utility>

#include "file-reader.h"

namespace {

LookupResult pathError(const string& message) {
    return {false, nullptr, message};
}

string fileExtension(const string& path) {
    string extension = filesystem::path(path).extension().string();
    transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return tolower(c);
    });
    return extension;
}

string fileContents(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        throw runtime_error("Could not open '" + path + "'");
    }

    ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

}

vector<JSONValue> readFile(const string& path) {
    string contents = fileContents(path);
    if (fileExtension(path) == ".jsonl") {
        return readJsonlFile(contents);
    }
    return readJsonFile(contents);
}

vector<JSONValue> readJsonlFile(const string& contents) {
    vector<JSONValue> records;
    istringstream stream(contents);
    string line;
    int lineNumber = 0;

    try {
        while (getline(stream, line)) {
            lineNumber++;
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
        JSONValue value = parser.parse();
        if (value.getType() == ValueType::Array) {
            records = get<ArrayValue>(value.getValue());
        } else {
            records.push_back(std::move(value));
        }
    } catch (const exception& e) {
        cerr << "Warning: failed to parse JSON file (" << e.what() << ")\n";
    }

    return records;
}

void loadSessionFile(const string& path, Session& session, string& name, size_t& recordCount) {
    uploadFile(path, session);
    name = filesystem::path(path).filename().string();
    if (session.file.getType() == ValueType::Array) {
        recordCount = get<ArrayValue>(session.file.getValue()).size();
    } else {
        recordCount = 1;
    }
}

LookupResult get(const JSONValue& value, const string& path) {
    vector<pair<bool, string>> parts;
    size_t i = 0;

    while (i < path.size()) {
        if (path[i] == '.') {
            i++;
            size_t start = i;
            while (i < path.size() && path[i] != '.' && path[i] != '[') {
                i++;
            }
            parts.push_back({false, path.substr(start, i - start)});
        } else if (path[i] == '[') {
            i++;
            size_t start = i;
            while (i < path.size() && path[i] != ']') {
                i++;
            }
            if (i >= path.size()) {
                return pathError("missing closing ']' in path");
            }
            parts.push_back({true, path.substr(start, i - start)});
            i++;
        } else {
            return pathError("invalid path syntax at position " + to_string(i));
        }
    }

    const JSONValue* current = &value;

    for (const auto& part : parts) {
        const string& text = part.second;

        if (!part.first) {
            if (current->getType() != ValueType::Object) {
                return pathError("expected an object to look up field '" + text + "'");
            }

            const auto& object = get<ObjectValue>(current->getValue());
            const JSONValue* found = nullptr;
            for (const auto& entry : object) {
                if (entry.first == text) {
                    found = &entry.second;
                    break;
                }
            }
            if (!found) {
                return pathError("field '" + text + "' not found");
            }
            current = found;
        } else {
            size_t parsedLength = 0;
            int index;
            try {
                index = stoi(text, &parsedLength);
            } catch (const exception&) {
                return pathError("invalid array index '" + text + "'");
            }
            if (parsedLength != text.size()) {
                return pathError("invalid array index '" + text + "'");
            }
            if (current->getType() != ValueType::Array) {
                return pathError("expected an array to index with [" + text + "]");
            }

            const auto& array = get<ArrayValue>(current->getValue());
            if (index < 0 || static_cast<size_t>(index) >= array.size()) {
                return pathError("index " + text + " out of range");
            }
            current = &array[static_cast<size_t>(index)];
        }
    }

    return {true, current, ""};
}
