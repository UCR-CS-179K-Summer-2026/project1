#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "file-reader.h"
#include "version.h"

const string DATASET = "json/students.json";
const string QUERY = ".courses[0].grade";
const size_t EXPECTED_RECORDS = 85032;
const int RUNS = 5;

struct BenchmarkResult {
    size_t records;
    size_t lookups;
    double loadMs;
    double lookupMs;
    double totalMs;
};

BenchmarkResult runBenchmark() {
    auto start = chrono::steady_clock::now();
    vector<JSONValue> records = readFile(DATASET);
    auto loaded = chrono::steady_clock::now();

    size_t lookups = 0;
    for (const auto& record : records) {
        LookupResult result = get(record, QUERY);
        if (result.ok) {
            lookups++;
        }
    }

    auto finished = chrono::steady_clock::now();

    double loadMs = chrono::duration<double, milli>(loaded - start).count();
    double lookupMs = chrono::duration<double, milli>(finished - loaded).count();
    double totalMs = chrono::duration<double, milli>(finished - start).count();

    return {records.size(), lookups, loadMs, lookupMs, totalMs};
}

bool validResult(const BenchmarkResult& result) {
    return result.records == EXPECTED_RECORDS && result.lookups == EXPECTED_RECORDS;
}

void printResult(const string& run, const BenchmarkResult& result) {
    cout << getVersionId() << ","
         << DATASET << ","
         << QUERY << ","
         << run << ","
         << result.records << ","
         << fixed << setprecision(3)
         << result.loadMs << ","
         << result.lookupMs << ","
         << result.totalMs << "\n";
}

int main() {
    try {
        BenchmarkResult warmup = runBenchmark();
        if (!validResult(warmup)) {
            cerr << "benchmark: expected " << EXPECTED_RECORDS
                 << " records and successful lookups\n";
            return 1;
        }

        vector<BenchmarkResult> results;

        for (int i = 0; i < RUNS; i++) {
            BenchmarkResult result = runBenchmark();
            if (!validResult(result)) {
                cerr << "benchmark: expected " << EXPECTED_RECORDS
                     << " records and successful lookups\n";
                return 1;
            }

            results.push_back(result);
        }

        cout << "version,dataset,query,run,records,load_ms,lookup_ms,total_ms\n";
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
