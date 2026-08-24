// Session & execution engine (Ryan & Javier): reads a .json or .jsonl file
// into a JSONValue via FileParser, then implements path lookup/formatting
// and the FILTER/SORT/LIMIT/GROUPBY/AVERAGE pipeline against the resulting
// tree in executeQuery().
#include "query-engine.h"

#include <algorithm>
#include <charconv>
#include <thread>
#include <unordered_map>

namespace {

LookupResult queryError(const string& message) {
    return {false, nullptr, message};
}

// Compares an object key against a GET(...) path segment for equality.
//
// Technique: fast path / slow path optimization. The naive version called
// unescapeString(a) == unescapeString(b) unconditionally, which heap-allocates
// two new strings on every single key comparison during a linear object scan
// (see get() below) -- the dominant cost in the pipeline's hottest loop.
//
// Both a and b are stored in raw/escaped form (unescapeString is only run
// here, on demand, never when the tree is built). unescapeString is a pure,
// deterministic function, which licenses two shortcuts that preserve the
// exact same result as the naive version while skipping the allocation in
// the common case:
bool keysEqual(string_view a, string_view b) {
    // Fast path 1: raw byte comparison (~memcmp), zero allocation. Equal
    // inputs to a pure function always produce equal outputs, so if the raw
    // bytes already match, unescapeString(a) == unescapeString(b) is
    // guaranteed true without computing either side.
    if (a == b) {
        return true;
    }

    // Fast path 2: identity-transform check. unescapeString only changes a
    // string when it contains a '\' escape sequence -- with no backslash on
    // either side, unescapeString(x) == x for both, so the raw mismatch we
    // already observed above is a real mismatch. Still zero allocation.
    if (a.find('\\') == string_view::npos && b.find('\\') == string_view::npos) {
        return false;
    }

    // Slow path: only reached when the raw bytes differ AND at least one
    // side could plausibly decode into something the other side equals
    // (e.g. differently-escaped spellings of the same character). This is
    // the rare case, so paying for the two allocations here is fine.
    return unescapeString(a) == unescapeString(b);
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

string numberText(double value) {
    char buffer[64];
    auto result = to_chars(buffer, buffer + sizeof(buffer), value, chars_format::general, 6);
    if (result.ec != errc{}) {
        throw runtime_error("could not format number");
    }
    return string(buffer, result.ptr);
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
        case ValueType::Number:
            return numberText(get<double>(value.getValue()));
        default:
            throw runtime_error("can't group by an array or object field");
    }
}

// Per-chunk accumulator for the parallel reduction below. hasTypeError
// replaces the old inline `throw` -- a worker thread throwing across the
// thread boundary is undefined behavior (the exception would need to
// unwind into a stack frame that no longer exists once join() returns), so
// each chunk reports its own error status and the caller decides whether
// to throw, back on the main thread, after every worker has finished.
struct PartialSum {
    double sum = 0;
    size_t count = 0;
    bool hasTypeError = false;
};

// The original sequential body, unchanged, now scoped to a [begin, end)
// slice of arr instead of the whole array. This is the "map" step: each
// thread runs this independently over its own slice with no shared
// mutable state, so no locking is needed here.
PartialSum sumFieldRange(const ArrayValue& arr, size_t begin, size_t end, const vector<string_view>& path) {
    PartialSum result;
    for (size_t i = begin; i < end; i++) {
        LookupResult found = get(arr[i], path);
        if (!found.ok) {
            continue;
        }
        if (found.value->getType() != ValueType::Number) {
            result.hasTypeError = true;
            continue;
        }
        result.sum += get<double>(found.value->getValue());
        result.count++;
    }
    return result;
}

// Below this many records, thread creation/teardown overhead (each
// std::thread costs real time to spin up and join) would cost more than
// the parallel work saves -- this is the "+p" term in Brent's theorem
// (T_p = O(n/p + p)) actually mattering when n is small. Sequential
// fallback below the threshold sidesteps that overhead entirely.
constexpr size_t PARALLEL_THRESHOLD = 10000;

pair<double, size_t> sumField(const ArrayValue& arr, const vector<string_view>& path) {
    size_t n = arr.size();

    unsigned int hwThreads = thread::hardware_concurrency();
    size_t threadCount = hwThreads == 0 ? 1 : static_cast<size_t>(hwThreads);

    if (n < PARALLEL_THRESHOLD || threadCount <= 1) {
        PartialSum result = sumFieldRange(arr, 0, n, path);
        if (result.hasTypeError) {
            throw runtime_error("AVERAGE field is not a number on every record");
        }
        return {result.sum, result.count};
    }

    // Partition: split arr into threadCount contiguous, non-overlapping
    // slices (each ~n/threadCount records) instead of one record at a
    // time -- this keeps each thread's memory access sequential/cache-
    // friendly rather than interleaved, and keeps synchronization to just
    // the join() below (no per-record locking).
    size_t chunkSize = (n + threadCount - 1) / threadCount;
    vector<PartialSum> partials(threadCount);
    vector<thread> workers;
    workers.reserve(threadCount);

    for (size_t t = 0; t < threadCount; t++) {
        size_t begin = t * chunkSize;
        size_t end = min(begin + chunkSize, n);
        if (begin >= end) {
            break;
        }
        // Map: each thread reduces its own slice into partials[t]
        // independently -- this is the O(n/p) parallel work term.
        workers.emplace_back([&, begin, end, t]() {
            partials[t] = sumFieldRange(arr, begin, end, path);
        });
    }
    for (thread& worker : workers) {
        worker.join();
    }

    // Reduce: merge the (small, ~threadCount-sized) set of partial results
    // sequentially on the main thread. This is the O(p) term in Brent's
    // theorem -- negligible next to the O(n/p) map phase for realistic
    // core counts, so it isn't worth parallelizing further here.
    double sum = 0;
    size_t count = 0;
    bool hasTypeError = false;
    for (const PartialSum& partial : partials) {
        sum += partial.sum;
        count += partial.count;
        hasTypeError = hasTypeError || partial.hasTypeError;
    }

    if (hasTypeError) {
        throw runtime_error("AVERAGE field is not a number on every record");
    }
    return {sum, count};
}

}  // namespace

void uploadFile(const string& path, Session& session) {
    ifstream file(path, ios::binary);
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

    file.seekg(0, ios::end);
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    string contents(static_cast<size_t>(size), '\0');
    if (size > 0 && !file.read(contents.data(), size)) {
        throw runtime_error("Could not read '" + path + "'.");
    }

    JSONValue temp = JSONValue(nullptr);

    if(extension == ".jsonl") {
        FileParser p(contents, ParserType::JSONL);
        temp = p.parse();
    } else {
        FileParser p(contents, ParserType::JSON);
        temp = p.parse();
    }

    session.initialize(move(temp), name);
}

string executeQuery(Session& session, const string& query) {
    QueryParser q(query);
    q.parse();

    const vector<QueryFunction>& pipeline = q.getFunctions();
    const vector<ExpressionNode>& nodes = q.getExpressionNodes();

    JSONValue owned = JSONValue(nullptr);
    const JSONValue* currentPtr = &session.file;

    // True once currentPtr points somewhere inside `owned`'s own tree --
    // i.e. once some earlier stage has already produced a value nobody
    // else needs, so records under it can be moved instead of copied.
    // False while currentPtr still points into session.file's tree, which
    // must come out of this query unmutated (the next query needs it
    // intact). GET narrows currentPtr within whichever tree it's already
    // in, so it never changes which one that is -- only FILTER/SORT/
    // LIMIT/GROUPBY/AVERAGE do, by building a genuinely new `owned` value.
    bool owns = false;

    // Takes record i out of arr as an owned JSONValue: a move (steals the
    // record's internal buffers -- strings, nested arrays -- rather than
    // recursively duplicating them) if arr really is owned's storage, a
    // real copy if arr is still borrowed from session.file. Only ever
    // called for a record a stage is actually keeping, never for one it
    // skips -- that's what stops a FILTER/GROUPBY that keeps almost
    // nothing from paying for records it's about to throw away, and what
    // lets LIMIT(n) touch exactly n records regardless of how large the
    // array feeding it is.
    auto take = [](const ArrayValue& arr, size_t i, bool owns) -> JSONValue {
        if (owns) {
            // Safe: `owns` being true is exactly the guarantee that arr is
            // owned's own (genuinely mutable) storage, not session.file's --
            // this const_cast undoes the const view we deliberately kept
            // uniform with the borrowed-from-session.file case, it doesn't
            // discard any real constness.
            return move(const_cast<JSONValue&>(arr[i]));
        }
        return arr[i];
    };

    for (const QueryFunction& function : pipeline) {
        if (holds_alternative<Get>(function)) {
            const Get& getFunc = get<Get>(function);
            const GetOperand& operand = get<GetOperand>(nodes[getFunc.target]);

            LookupResult result = get(*currentPtr, operand.path);
            if (!result.ok) {
                return formatResult(result);
            }
            currentPtr = result.value;  // just repoint -- no copy needed to read further into the same tree

        } else if (holds_alternative<Filter>(function)) {
            const Filter& filterFunc = get<Filter>(function);

            if (currentPtr->getType() != ValueType::Array) {
                return formatResult(queryError("FILTER expects an array of records"));
            }
            const ArrayValue& arr = get<ArrayValue>(currentPtr->getValue());

            // Evaluating one record's predicate never touches another
            // record, so this is the same map/partition/merge shape as
            // sumField's reduction: each thread filters its own [begin,
            // end) slice into its own local ArrayValue (using the same
            // take() rule -- move if owned, copy if borrowed -- since
            // indices are disjoint across threads, no two threads ever
            // touch the same element). No exception-across-threads
            // workaround is needed here the way sumField needed
            // hasTypeError: a record whose condition can't be evaluated
            // is already caught and skipped locally, per record, same as
            // the sequential version -- nothing ever needs to escape a
            // worker thread.
            auto filterRange = [&](size_t begin, size_t end) -> ArrayValue {
                ArrayValue localMatches;
                for (size_t i = begin; i < end; i++) {
                    bool keep;
                    try {
                        keep = requireBoolean(evaluateNode(filterFunc.condition, nodes, arr[i]));
                    } catch (const exception&) {
                        continue;  // condition couldn't be evaluated for this record: doesn't match
                    }
                    if (keep) {
                        localMatches.push_back(take(arr, i, owns));
                    }
                }
                return localMatches;
            };

            size_t n = arr.size();
            unsigned int hwThreads = thread::hardware_concurrency();
            size_t threadCount = hwThreads == 0 ? 1 : static_cast<size_t>(hwThreads);

            ArrayValue matches;
            if (n < PARALLEL_THRESHOLD || threadCount <= 1) {
                matches = filterRange(0, n);
            } else {
                size_t chunkSize = (n + threadCount - 1) / threadCount;
                vector<ArrayValue> partials(threadCount);
                vector<thread> workers;
                workers.reserve(threadCount);
                for (size_t t = 0; t < threadCount; t++) {
                    size_t begin = t * chunkSize;
                    size_t end = min(begin + chunkSize, n);
                    if (begin >= end) {
                        break;
                    }
                    workers.emplace_back([&, begin, end, t]() {
                        partials[t] = filterRange(begin, end);
                    });
                }
                for (thread& worker : workers) {
                    worker.join();
                }

                // Merge: partitions cover strictly increasing, disjoint
                // index ranges, so concatenating their matches in
                // partition order reproduces the exact same relative
                // order the sequential version would have produced.
                size_t total = 0;
                for (const ArrayValue& partial : partials) {
                    total += partial.size();
                }
                matches.reserve(total);
                for (ArrayValue& partial : partials) {
                    for (JSONValue& record : partial) {
                        matches.push_back(move(record));
                    }
                }
            }

            owned = JSONValue(move(matches));
            currentPtr = &owned;
            owns = true;

        } else if (holds_alternative<Sort>(function)) {
            const Sort& sortFunc = get<Sort>(function);

            if (currentPtr->getType() != ValueType::Array) {
                return formatResult(queryError("SORT expects an array of records"));
            }
            const ArrayValue& arr = get<ArrayValue>(currentPtr->getValue());
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

            // Ascending wants "a < b"; descending wants "a > b" -- both are
            // a single compareValues call. The previous version computed
            // "a < b" and, for descending, *also* "a == b" to build "not
            // less and not equal" as a roundabout way of saying "greater" --
            // two comparisons per call instead of one, for every one of the
            // O(n log n) comparisons stable_sort makes (~1.4M of them at
            // 85k records) whenever the sort was descending.
            QueryTokenType comparisonOp = sortFunc.direction == Direction::Desc ? QueryTokenType::Gt : QueryTokenType::Lt;
            try {
                stable_sort(keyed.begin(), keyed.end(), [&](const auto& a, const auto& b) {
                    return compareValues(*a.first, comparisonOp, *b.first);
                });
            } catch (const exception& e) {
                return formatResult(queryError(e.what()));
            }

            // SORT keeps every record (just reordered) -- unlike
            // FILTER/GROUPBY/LIMIT there's no work to skip, so take()'s
            // per-record laziness buys nothing here and only adds a
            // function call per element. Since every record is wanted,
            // take the whole array in one bulk operation instead: a move
            // (O(1) -- steals the vector's buffer wholesale) if it's
            // already owned, a copy (the vector's own bulk copy
            // constructor, sequential and cache-friendly) if it's still
            // borrowed from session.file. Either way, permuting into sort
            // order afterward is just moves -- a few pointer-swaps per
            // record -- so doing that scattered-order pass on already-owned
            // records is cheap in a way scattering the copy itself would not be.
            ArrayValue records = owns ? move(const_cast<ArrayValue&>(arr)) : arr;
            ArrayValue sorted;
            sorted.reserve(records.size());
            for (const auto& [keyPtr, index] : keyed) {
                sorted.push_back(move(records[index]));
            }
            owned = JSONValue(move(sorted));
            currentPtr = &owned;
            owns = true;

        } else if (holds_alternative<Limit>(function)) {
            const Limit& limitFunc = get<Limit>(function);

            if (currentPtr->getType() != ValueType::Array) {
                return formatResult(queryError("LIMIT expects an array of records"));
            }
            const ArrayValue& arr = get<ArrayValue>(currentPtr->getValue());
            size_t count = min(static_cast<size_t>(limitFunc.size), arr.size());

            // Only the surviving `count` records are ever touched -- the
            // rest of arr (which may be most of an 85k-record array) is
            // never read, copied, or moved at all.
            ArrayValue limited;
            limited.reserve(count);
            for (size_t i = 0; i < count; i++) {
                limited.push_back(take(arr, i, owns));
            }
            owned = JSONValue(move(limited));
            currentPtr = &owned;
            owns = true;

        } else if (holds_alternative<GroupBy>(function)) {
            const GroupBy& groupFunc = get<GroupBy>(function);

            if (currentPtr->getType() != ValueType::Array) {
                return formatResult(queryError("GROUPBY expects an array of records"));
            }
            const ArrayValue& arr = get<ArrayValue>(currentPtr->getValue());
            const GetOperand& target = get<GetOperand>(nodes[groupFunc.target]);

            // Same map/partition/merge shape as FILTER above, one level
            // richer: each thread builds its own key -> bucket map (hash
            // map from group key to bucket index, same O(1)-average
            // reasoning as the sequential version) over its own slice,
            // then the "reduce" step below combines the (small number of)
            // local maps into one.
            struct LocalGroups {
                unordered_map<string, size_t> index;
                vector<pair<string, ArrayValue>> groups;
            };

            auto groupRange = [&](size_t begin, size_t end) -> LocalGroups {
                LocalGroups local;
                for (size_t i = begin; i < end; i++) {
                    LookupResult found = get(arr[i], target.path);
                    if (!found.ok) {
                        continue;  // record missing the field: leave it out of every group
                    }
                    string key;
                    try {
                        key = groupKeyText(*found.value);
                    } catch (const exception&) {
                        continue;  // field isn't a valid group key (array/object): leave it out
                    }

                    auto it = local.index.find(key);
                    size_t bucketIndex;
                    if (it == local.index.end()) {
                        bucketIndex = local.groups.size();
                        local.index.emplace(key, bucketIndex);
                        local.groups.emplace_back(key, ArrayValue{});
                    } else {
                        bucketIndex = it->second;
                    }
                    local.groups[bucketIndex].second.push_back(take(arr, i, owns));
                }
                return local;
            };

            size_t n = arr.size();
            unsigned int hwThreads = thread::hardware_concurrency();
            size_t threadCount = hwThreads == 0 ? 1 : static_cast<size_t>(hwThreads);

            unordered_map<string, size_t> groupIndex;
            vector<pair<string, ArrayValue>> groups;

            if (n < PARALLEL_THRESHOLD || threadCount <= 1) {
                LocalGroups local = groupRange(0, n);
                groupIndex = move(local.index);
                groups = move(local.groups);
            } else {
                size_t chunkSize = (n + threadCount - 1) / threadCount;
                vector<LocalGroups> partials(threadCount);
                vector<thread> workers;
                workers.reserve(threadCount);
                for (size_t t = 0; t < threadCount; t++) {
                    size_t begin = t * chunkSize;
                    size_t end = min(begin + chunkSize, n);
                    if (begin >= end) {
                        break;
                    }
                    workers.emplace_back([&, begin, end, t]() {
                        partials[t] = groupRange(begin, end);
                    });
                }
                for (thread& worker : workers) {
                    worker.join();
                }

                // Reduce: merge the local group maps in partition order.
                // Partitions cover strictly increasing, disjoint index
                // ranges, so a key's first appearance always surfaces in
                // the partition that actually contains its true first
                // occurrence -- merging partition-by-partition, and
                // within each partition in its own first-seen order,
                // reconstructs the exact group order (and, by appending
                // each partition's bucket for a key in the same order,
                // the exact within-group record order) the sequential
                // version would have produced.
                for (LocalGroups& local : partials) {
                    for (auto& [key, bucket] : local.groups) {
                        auto it = groupIndex.find(key);
                        size_t index;
                        if (it == groupIndex.end()) {
                            index = groups.size();
                            groupIndex.emplace(key, index);
                            groups.emplace_back(key, ArrayValue{});
                        } else {
                            index = it->second;
                        }
                        ArrayValue& combined = groups[index].second;
                        combined.reserve(combined.size() + bucket.size());
                        for (JSONValue& record : bucket) {
                            combined.push_back(move(record));
                        }
                    }
                }
            }

            ObjectValue result;
            result.reserve(groups.size());
            for (auto& [key, bucket] : groups) {
                result.emplace_back(move(key), JSONValue(move(bucket)));
            }
            owned = JSONValue(move(result));
            currentPtr = &owned;
            owns = true;

        } else if (holds_alternative<Average>(function)) {
            const Average& avgFunc = get<Average>(function);
            const AverageOperand& target = get<AverageOperand>(nodes[avgFunc.target]);

            try {
                if (currentPtr->getType() == ValueType::Array) {
                    auto [sum, count] = sumField(get<ArrayValue>(currentPtr->getValue()), target.path);
                    if (count == 0) {
                        return formatResult(queryError("no numeric values found for AVERAGE"));
                    }
                    owned = JSONValue(sum / static_cast<double>(count));
                    currentPtr = &owned;
                    owns = true;

                } else if (currentPtr->getType() == ValueType::Object) {
                    const auto& groups = get<ObjectValue>(currentPtr->getValue());
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
                    owned = JSONValue(move(result));
                    currentPtr = &owned;
                    owns = true;

                } else {
                    return formatResult(queryError("AVERAGE expects an array of records, or a GROUPBY result"));
                }
            } catch (const exception& e) {
                return formatResult(queryError(e.what()));
            }
        }
    }

    return formatResult({true, currentPtr, ""});
}

namespace {

string formatValue(const JSONValue& value, int depth = 0) {
    string indent(depth + 1, ' ');
    string closeIndent(depth, ' ');

    switch (value.getType()) {
        case ValueType::Null:
            return "null";
        case ValueType::Boolean:
            return get<bool>(value.getValue()) ? "true" : "false";
        case ValueType::Number:
            return numberText(get<double>(value.getValue()));
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

LookupResult get(const JSONValue& value, const vector<string_view>& path) {
    const JSONValue* current = &value;

    for (const string_view& prop : path) {
        if (current->getType() == ValueType::Object) {
            const auto& obj = get<ObjectValue>(current->getValue());

            const JSONValue* found = nullptr;
            for (const auto& entry : obj) {
                if (keysEqual(entry.first, prop)) {
                    found = &entry.second;
                    break;
                }
            }
            if (!found) {
                return queryError("field '" + string(prop) + "' not found");
            }
            current = found;

        } else if (current->getType() == ValueType::Array) {
            string indexStr(prop);
            size_t parsedLen = 0;
            int index;
            try {
                index = stoi(indexStr, &parsedLen);
            } catch (const exception&) {
                return queryError("expected a numeric index to access an array, got '" + indexStr + "'");
            }
            if (parsedLen != indexStr.size()) {
                return queryError("expected a numeric index to access an array, got '" + indexStr + "'");
            }

            const auto& arr = get<ArrayValue>(current->getValue());
            if (index < 0 || static_cast<size_t>(index) >= arr.size()) {
                return queryError("index " + indexStr + " out of range");
            }
            current = &arr[static_cast<size_t>(index)];

        } else {
            return queryError("cannot look up '" + string(prop) + "' on a non-object, non-array value");
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
