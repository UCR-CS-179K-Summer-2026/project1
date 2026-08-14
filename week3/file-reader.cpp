// Person 2's contribution: reads a .json or .jsonl file into JSONValue
// records via Person 1's Parser, and implements path lookup/formatting
// against the resulting tree.
#include "file-reader.h"

#include <algorithm>
#include <unordered_map>

namespace {

LookupResult queryError(const string& message) {
    return {false, nullptr, message};
}

bool valuesEqual(const JSONValue& lhs, const JSONValue& rhs) {
    if (lhs.getType() != rhs.getType()) {
        return false;
    }
    switch (lhs.getType()) {
        case ValueType::Null:
            return true;
        case ValueType::Boolean:
            return get<bool>(lhs.getValue()) == get<bool>(rhs.getValue());
        case ValueType::Number:
            return get<double>(lhs.getValue()) == get<double>(rhs.getValue());
        case ValueType::String:
            return unescapeString(get<string>(lhs.getValue())) == unescapeString(get<string>(rhs.getValue()));
        default:
            return false;  // arrays/objects: not a supported comparison
    }
}

// Applies a QueryScanner comparison token to two JSONValues. Ordering
// operators only make sense between two numbers or two strings.
bool compareValues(const JSONValue& lhs, QueryTokenType op, const JSONValue& rhs) {
    if (op == QueryTokenType::Eq) return valuesEqual(lhs, rhs);
    if (op == QueryTokenType::Neq) return !valuesEqual(lhs, rhs);

    if (lhs.getType() == ValueType::Number && rhs.getType() == ValueType::Number) {
        double a = get<double>(lhs.getValue());
        double b = get<double>(rhs.getValue());
        if (op == QueryTokenType::Gt) return a > b;
        if (op == QueryTokenType::Lt) return a < b;
        if (op == QueryTokenType::Geq) return a >= b;
        if (op == QueryTokenType::Leq) return a <= b;
    }

    if (lhs.getType() == ValueType::String && rhs.getType() == ValueType::String) {
        const string& a = get<string>(lhs.getValue());
        const string& b = get<string>(rhs.getValue());
        if (op == QueryTokenType::Gt) return a > b;
        if (op == QueryTokenType::Lt) return a < b;
        if (op == QueryTokenType::Geq) return a >= b;
        if (op == QueryTokenType::Leq) return a <= b;
    }

    throw runtime_error("cannot compare with that operator unless both sides are numbers or both are strings");
}

bool requireBoolean(const JSONValue& value) {
    if (value.getType() != ValueType::Boolean) {
        throw runtime_error("expected a boolean expression in FILTER");
    }
    return get<bool>(value.getValue());
}

// Sums a numeric field across `arr`, skipping records missing the field.
// Returns {sum, count}; count == 0 means no usable values were found.
pair<double, size_t> sumField(const ArrayValue& arr, const vector<string_view>& path);

// Evaluates an expression node (from QueryParser's expression tree) against
// one record. GetOperand nodes look up their path on `record`; AverageOperand
// averages a numeric path across `record` (which must be an array);
// StringOperand/NumberOperand/BooleanOperand are literals; BinaryOperand
// recurses into its two sides and applies the comparison or AND/OR.
JSONValue evaluateNode(NodeId id, const vector<ExpressionNode>& nodes, const JSONValue& record) {
    const ExpressionNode& node = nodes[id];

    if (holds_alternative<StringOperand>(node)) {
        return JSONValue(unescapeString(get<StringOperand>(node).val));
    }
    if (holds_alternative<NumberOperand>(node)) {
        return JSONValue(get<NumberOperand>(node).val);
    }
    if (holds_alternative<BooleanOperand>(node)) {
        return JSONValue(get<BooleanOperand>(node).val);
    }
    if (holds_alternative<GetOperand>(node)) {
        LookupResult result = get(record, get<GetOperand>(node).path);
        if (!result.ok) {
            throw runtime_error(result.error);
        }
        return *result.value;
    }
    if (holds_alternative<AverageOperand>(node)) {
        const AverageOperand& avg = get<AverageOperand>(node);
        if (record.getType() != ValueType::Array) {
            throw runtime_error("AVERAGE() in an expression expects the current value to be an array");
        }
        auto [sum, count] = sumField(get<ArrayValue>(record.getValue()), avg.path);
        if (count == 0) {
            throw runtime_error("no numeric values found for AVERAGE");
        }
        return JSONValue(sum / static_cast<double>(count));
    }

    const BinaryOperand& bin = get<BinaryOperand>(node);

    if (bin.op == QueryTokenType::And || bin.op == QueryTokenType::Or) {
        bool left = requireBoolean(evaluateNode(bin.left, nodes, record));
        if (bin.op == QueryTokenType::And && !left) return JSONValue(false);
        if (bin.op == QueryTokenType::Or && left) return JSONValue(true);
        return JSONValue(requireBoolean(evaluateNode(bin.right, nodes, record)));
    }

    JSONValue left = evaluateNode(bin.left, nodes, record);
    JSONValue right = evaluateNode(bin.right, nodes, record);
    return JSONValue(compareValues(left, bin.op, right));
}

// Turns a scalar field value into the text used as a GROUPBY key.
string groupKeyText(const JSONValue& value) {
    switch (value.getType()) {
        case ValueType::String:
            return get<string>(value.getValue());
        case ValueType::Boolean:
            return get<bool>(value.getValue()) ? "true" : "false";
        case ValueType::Null:
            return "null";
        case ValueType::Number: {
            ostringstream out;
            out << get<double>(value.getValue());
            return out.str();
        }
        default:
            throw runtime_error("can't group by an array or object field");
    }
}

pair<double, size_t> sumField(const ArrayValue& arr, const vector<string_view>& path) {
    double sum = 0;
    size_t count = 0;
    for (const auto& record : arr) {
        LookupResult found = get(record, path);
        if (!found.ok) {
            continue;
        }
        if (found.value->getType() != ValueType::Number) {
            throw runtime_error("AVERAGE field is not a number on every record");
        }
        sum += get<double>(found.value->getValue());
        count++;
    }
    return {sum, count};
}

}  // namespace

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
    string name = filepath.filename().string();

    transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return tolower(c);
    });

    ostringstream buffer;
    buffer << file.rdbuf();
    string contents = buffer.str();

    JSONValue temp = JSONValue(nullptr);

    if(extension == ".jsonl") {
        Parser p(contents, ParserType::JSONL);
        temp = p.parse();
    } else {
        Parser p(contents, ParserType::JSON);
        temp = p.parse();
    }

    session.initialize(temp, name);
}

string excecuteQuery(Session& session, const string& query) {
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
            const Filter& filterFunc = get<Filter>(function);

            if (current.getType() != ValueType::Array) {
                return formatResult(queryError("FILTER expects an array of records"));
            }
            const ArrayValue& arr = get<ArrayValue>(current.getValue());

            ArrayValue matches;
            for (const auto& record : arr) {
                bool keep;
                try {
                    keep = requireBoolean(evaluateNode(filterFunc.condition, nodes, record));
                } catch (const exception&) {
                    continue;  // condition couldn't be evaluated for this record: doesn't match
                }
                if (keep) {
                    matches.push_back(record);
                }
            }
            current = JSONValue(move(matches));

        } else if (holds_alternative<Sort>(function)) {
            const Sort& sortFunc = get<Sort>(function);

            if (current.getType() != ValueType::Array) {
                return formatResult(queryError("SORT expects an array of records"));
            }
            const ArrayValue& arr = get<ArrayValue>(current.getValue());
            const GetOperand& target = get<GetOperand>(nodes[sortFunc.target]);

            // Look up each record's sort key once up front (a "Schwartzian
            // transform") instead of re-deriving it inside the comparator
            // on every comparison.
            vector<pair<const JSONValue*, size_t>> keyed;
            keyed.reserve(arr.size());
            for (size_t i = 0; i < arr.size(); i++) {
                LookupResult found = get(arr[i], target.path);
                if (!found.ok) {
                    return formatResult(found);
                }
                keyed.emplace_back(found.value, i);
            }

            bool descending = sortFunc.direction == Direction::Desc;
            try {
                stable_sort(keyed.begin(), keyed.end(), [&](const auto& a, const auto& b) {
                    bool less = compareValues(*a.first, QueryTokenType::Lt, *b.first);
                    return descending ? (!less && !valuesEqual(*a.first, *b.first)) : less;
                });
            } catch (const exception& e) {
                return formatResult(queryError(e.what()));
            }

            ArrayValue sorted;
            sorted.reserve(arr.size());
            for (const auto& [keyPtr, index] : keyed) {
                sorted.push_back(arr[index]);
            }
            current = JSONValue(move(sorted));

        } else if (holds_alternative<Limit>(function)) {
            const Limit& limitFunc = get<Limit>(function);

            if (current.getType() != ValueType::Array) {
                return formatResult(queryError("LIMIT expects an array of records"));
            }
            const ArrayValue& arr = get<ArrayValue>(current.getValue());
            size_t count = min(static_cast<size_t>(limitFunc.size), arr.size());
            current = JSONValue(ArrayValue(arr.begin(), arr.begin() + static_cast<long>(count)));

        } else if (holds_alternative<GroupBy>(function)) {
            const GroupBy& groupFunc = get<GroupBy>(function);

            if (current.getType() != ValueType::Array) {
                return formatResult(queryError("GROUPBY expects an array of records"));
            }
            const ArrayValue& arr = get<ArrayValue>(current.getValue());
            const GetOperand& target = get<GetOperand>(nodes[groupFunc.target]);

            // Hash map from group key to the bucket's index (O(1) average)
            // so finding-or-creating a record's group doesn't mean scanning
            // every group seen so far.
            unordered_map<string, size_t> groupIndex;
            vector<pair<string, ArrayValue>> groups;

            for (const auto& record : arr) {
                LookupResult found = get(record, target.path);
                if (!found.ok) {
                    continue;  // record missing the field: leave it out of every group
                }
                string key;
                try {
                    key = groupKeyText(*found.value);
                } catch (const exception&) {
                    continue;  // field isn't a valid group key (array/object): leave it out
                }

                auto it = groupIndex.find(key);
                size_t index;
                if (it == groupIndex.end()) {
                    index = groups.size();
                    groupIndex.emplace(key, index);
                    groups.emplace_back(key, ArrayValue{});
                } else {
                    index = it->second;
                }
                groups[index].second.push_back(record);
            }

            ObjectValue result;
            result.reserve(groups.size());
            for (auto& [key, bucket] : groups) {
                result.emplace_back(move(key), JSONValue(move(bucket)));
            }
            current = JSONValue(move(result));

        } else if (holds_alternative<Average>(function)) {
            const Average& avgFunc = get<Average>(function);
            const AverageOperand& target = get<AverageOperand>(nodes[avgFunc.target]);

            try {
                if (current.getType() == ValueType::Array) {
                    auto [sum, count] = sumField(get<ArrayValue>(current.getValue()), target.path);
                    if (count == 0) {
                        return formatResult(queryError("no numeric values found for AVERAGE"));
                    }
                    current = JSONValue(sum / static_cast<double>(count));

                } else if (current.getType() == ValueType::Object) {
                    const auto& groups = get<ObjectValue>(current.getValue());
                    ObjectValue result;
                    for (const auto& [key, bucket] : groups) {
                        if (bucket.getType() != ValueType::Array) {
                            return formatResult(queryError("AVERAGE expects each group to be an array of records"));
                        }
                        auto [sum, count] = sumField(get<ArrayValue>(bucket.getValue()), target.path);
                        if (count == 0) {
                            continue;  // no usable values in this group: omit it
                        }
                        result.emplace_back(key, JSONValue(sum / static_cast<double>(count)));
                    }
                    current = JSONValue(move(result));

                } else {
                    return formatResult(queryError("AVERAGE expects an array of records, or a GROUPBY result"));
                }
            } catch (const exception& e) {
                return formatResult(queryError(e.what()));
            }
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
