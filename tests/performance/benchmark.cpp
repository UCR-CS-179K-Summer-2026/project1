#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "file-reader.h"
#include "version.h"

const string DATASET = "json/students.json";
const string QUERY = R"(AVERAGE(GET("gpa")))";
const string QUERY_NAME = "average_gpa";
const string EXPECTED_RESULT = "3.00217";
const size_t EXPECTED_RECORDS = 85032;
const int RUNS = 5;

struct BenchmarkResult {
    size_t records;
    string result;
    double loadMs;
    double queryMs;
    double totalMs;
};

BenchmarkResult runBenchmark() {
    Session session;

    auto start = chrono::steady_clock::now();
    uploadFile(DATASET, session);
    auto loaded = chrono::steady_clock::now();

    ostringstream ignored;
    streambuf* normalOutput = cout.rdbuf(ignored.rdbuf());
    string result;
    auto queryStarted = chrono::steady_clock::now();
    try {
        result = excecuteQuery(session, QUERY);
    } catch (...) {
        cout.rdbuf(normalOutput);
        throw;
    }
    auto finished = chrono::steady_clock::now();
    cout.rdbuf(normalOutput);

    size_t records = 0;
    if (session.isInitialized && session.file.getType() == ValueType::Array) {
        records = get<ArrayValue>(session.file.getValue()).size();
    }

    double loadMs = chrono::duration<double, milli>(loaded - start).count();
    double queryMs = chrono::duration<double, milli>(finished - queryStarted).count();
    double totalMs = loadMs + queryMs;

    return {records, result, loadMs, queryMs, totalMs};
}

bool validResult(const BenchmarkResult& result) {
    return result.records == EXPECTED_RECORDS && result.result == EXPECTED_RESULT;
}

void printResult(const string& run, const BenchmarkResult& result) {
    cout << getVersionId() << ","
         << DATASET << ","
         << QUERY_NAME << ","
         << run << ","
         << result.records << ","
         << fixed << setprecision(3)
         << result.loadMs << ","
         << result.queryMs << ","
         << result.totalMs << "\n";
}

int main() {
    try {
        BenchmarkResult warmup = runBenchmark();
        if (!validResult(warmup)) {
            cerr << "benchmark: unexpected current query result\n";
            return 1;
        }

        vector<BenchmarkResult> results;

        for (int i = 0; i < RUNS; i++) {
            BenchmarkResult result = runBenchmark();
            if (!validResult(result)) {
                cerr << "benchmark: unexpected current query result\n";
                return 1;
            }
            results.push_back(result);
        }

        cout << "version,dataset,query,run,records,load_ms,query_ms,total_ms\n";
        for (size_t i = 0; i < results.size(); i++) {
            printResult(to_string(i + 1), results[i]);
        }

        vector<BenchmarkResult> sortedResults = results;
        sort(sortedResults.begin(), sortedResults.end(), [](const BenchmarkResult& a,
                                                            const BenchmarkResult& b) {
            return a.totalMs < b.totalMs;
        });
        printResult("median", sortedResults[sortedResults.size() / 2]);
    } catch (const exception& e) {
        cerr << "benchmark: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
