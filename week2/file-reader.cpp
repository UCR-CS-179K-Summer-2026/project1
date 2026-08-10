// Person 2's contribution: reads a .json or .jsonl file into JSONValue
// records via Person 1's Parser, and implements path lookup/formatting
// against the resulting tree.
#include "file-reader.h"

void uploadFile(const string& path, Session& session) {
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

    JSONValue temp = JSONValue(nullptr);

    if(extension == ".jsonl") {
        //session.updateFile(contents, ParserType::JSONL);
        Parser p(contents, ParserType::JSONL);
        temp = p.parse();
    } else {
        //session.updateFile(contents, ParserType::JSON);
        Parser p(contents, ParserType::JSON);
        temp = p.parse();
    }

    session.initialize(temp);
}

string excecuteQuery(Session& session, string query) {
    cout << "Excecuting query..." << endl;

    QueryParser q(query);
    q.parse();

    const vector<QueryFunction>& pipeline = q.getFunctions();
    const vector<ExpressionNode>& nodes = q.getExpressionNodes();

    JSONValue current = session.file;

    for (const QueryFunction& function : pipeline) {
        if (holds_alternative<Get>(function)) {
            const Get& getFunc = get<Get>(function);
            const GetOperand& operand = get<GetOperand>(nodes[getFunc.target]);

            LookupResult result = get(current, operand.path);
            if (!result.ok) {
                return formatResult(result);
            }
            current = *result.value;

        } else if (holds_alternative<Filter>(function)) {
            // TODO: FILTER - keep only the records/entries in `current` for
            // which the condition expression (nodes[filter.condition])
            // evaluates to true.

        } else if (holds_alternative<Sort>(function)) {
            // TODO: SORT - reorder the entries in `current` by the value at
            // sort.target, ascending or descending per sort.direction.

        } else if (holds_alternative<Limit>(function)) {
            // TODO: LIMIT - truncate `current` down to at most limit.size
            // entries.

        } else if (holds_alternative<GroupBy>(function)) {
            // TODO: GROUPBY - bucket the entries in `current` by the value
            // at groupBy.target.

        } else if (holds_alternative<Average>(function)) {
            // TODO: AVERAGE - compute the mean of the numeric values at
            // average.target across the entries in `current`.
        }
    }

    return formatResult({true, &current, ""});
}

namespace {

LookupResult error(const string& message) {
    return {false, nullptr, message};
}

string formatValue(const JSONValue& value, int depth = 0) {
    string indent(depth + 1, ' ');
    string closeIndent(depth, ' ');

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
            if (obj.empty()) return "{}";
            ostringstream out;
            out << "{\n";
            for (size_t i = 0; i < obj.size(); ++i) {
                if (i > 0) out << ",\n";
                out << indent << "\"" << obj[i].first << "\": " << formatValue(obj[i].second, depth + 1);
            }
            out << "\n" << closeIndent << "}";
            return out.str();
        }
        case ValueType::Array: {
            const auto& arr = get<ArrayValue>(value.getValue());
            if (arr.empty()) return "[]";
            ostringstream out;
            out << "[\n";
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) out << ",\n";
                out << indent << formatValue(arr[i], depth + 1);
            }
            out << "\n" << closeIndent << "]";
            return out.str();
        }
    }
    return "null";
}

string formatValue(const vector<JSONValue>& values) {
    ostringstream out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) out << "\n";
        out << formatValue(values[i]);
    }
    return out.str();
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

string formatResult(const LookupResult& result) {
    if (!result.ok) {
        return "Error: " + result.error;
    }
    return formatValue(*result.value);
}
