#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "query-engine.h"
#include "session.h"
#include "version.h"

#ifndef STREAMLINE_PROJECT_ROOT
#define STREAMLINE_PROJECT_ROOT "."
#endif

#ifndef STREAMLINE_SYSTEM_NAME
#define STREAMLINE_SYSTEM_NAME "unknown"
#endif

#ifndef STREAMLINE_SYSTEM_PROCESSOR
#define STREAMLINE_SYSTEM_PROCESSOR "unknown"
#endif

#ifndef STREAMLINE_COMPILER
#define STREAMLINE_COMPILER "unknown"
#endif

#ifndef STREAMLINE_BUILD_TYPE
#define STREAMLINE_BUILD_TYPE "unknown"
#endif

const filesystem::path PROJECT_ROOT = STREAMLINE_PROJECT_ROOT;
const filesystem::path DATASET_PATH = PROJECT_ROOT / "json/students.json";
const string DATASET_NAME = "json/students.json";
const size_t EXPECTED_RECORDS = 85032;
const int RUNS = 5;

struct BenchmarkCase {
    string name;
    string query;
    string expectedResult;
};

const vector<BenchmarkCase> BENCHMARK_CASES = {
    {"nested_get", R"(GET(42516, "address", "city"))", "\"Riverside\""},
    {"average_gpa", R"(AVERAGE(GET("gpa")))", "3.00217"},
    {"filter_none", R"(FILTER(GET("gpa") > 4))", "[]"},
    {"filter_all", R"(FILTER(GET("gpa") >= 2) | GET(85031, "student_id"))", "85032"},
    {"sort_gpa_desc", R"(SORT(GET("gpa"), DESC) | GET(0, "student_id"))", "229"},
    {"group_by_major", R"(GROUPBY(GET("major")) | GET("Computer Science", 0, "student_id"))", "4"}
};

struct BenchmarkResult {
    size_t records;
    string result;
    double loadMs;
    double queryMs;
    double totalMs;
};

struct BenchmarkRun {
    BenchmarkCase benchmark;
    vector<BenchmarkResult> results;
    BenchmarkResult median;
};

struct BenchmarkOptions {
    string machine;
    filesystem::path output;
    bool outputWasSet = false;
    bool showHelp = false;
};

string detectMachine() {
    const char* configured = getenv("STREAMLINE_BENCHMARK_MACHINE");
    if (configured != nullptr && configured[0] != '\0') {
        return configured;
    }

    char name[256] = {};
#if defined(_WIN32)
    DWORD size = static_cast<DWORD>(sizeof(name));
    if (GetComputerNameA(name, &size) != 0 && size > 0) {
        return string(name, size);
    }
#else
    if (gethostname(name, sizeof(name) - 1) == 0 && name[0] != '\0') {
        return name;
    }
#endif
    return "unknown-machine";
}

string safeFileName(const string& value) {
    string result;
    bool previousUnderscore = false;

    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_') {
            result += static_cast<char>(tolower(c));
            previousUnderscore = false;
        } else if (!previousUnderscore) {
            result += '_';
            previousUnderscore = true;
        }
    }

    while (!result.empty() && result.back() == '_') {
        result.pop_back();
    }
    return result.empty() ? "unknown-machine" : result;
}

BenchmarkOptions parseOptions(int argc, char** argv) {
    BenchmarkOptions options;
    options.machine = detectMachine();

    for (int i = 1; i < argc; i++) {
        string argument = argv[i];
        if (argument == "--machine") {
            if (i + 1 >= argc) {
                throw invalid_argument("--machine requires a value");
            }
            options.machine = argv[++i];
        } else if (argument == "--output") {
            if (i + 1 >= argc) {
                throw invalid_argument("--output requires a path");
            }
            options.output = argv[++i];
            options.outputWasSet = true;
        } else if (argument == "--help" || argument == "-h") {
            options.showHelp = true;
        } else {
            throw invalid_argument("unknown option '" + argument + "'");
        }
    }

    if (!options.outputWasSet) {
        options.output = PROJECT_ROOT / "benchmarks/results" / (safeFileName(options.machine) + ".csv");
    }
    return options;
}

string utcTimestamp() {
    auto now = chrono::system_clock::now();
    time_t currentTime = chrono::system_clock::to_time_t(now);
    tm utc = {};
#if defined(_WIN32)
    gmtime_s(&utc, &currentTime);
#else
    gmtime_r(&currentTime, &utc);
#endif
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    ostringstream timestamp;
    timestamp << put_time(&utc, "%Y-%m-%dT%H:%M:%S")
              << "." << setfill('0') << setw(3) << milliseconds << "Z";
    return timestamp.str();
}

string csvField(const string& value) {
    if (value.find_first_of(",\"\n\r") == string::npos) {
        return value;
    }

    string escaped = "\"";
    for (char c : value) {
        if (c == '\"') {
            escaped += "\"\"";
        } else {
            escaped += c;
        }
    }
    escaped += '\"';
    return escaped;
}

BenchmarkResult runBenchmark(const BenchmarkCase& benchmark) {
    Session session;

    auto start = chrono::steady_clock::now();
    uploadFile(DATASET_PATH.string(), session);
    auto loaded = chrono::steady_clock::now();

    ostringstream ignored;
    streambuf* normalOutput = cout.rdbuf(ignored.rdbuf());
    string result;
    auto queryStarted = chrono::steady_clock::now();
    try {
        result = excecuteQuery(session, benchmark.query);
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

bool validResult(const BenchmarkCase& benchmark, const BenchmarkResult& result) {
    return result.records == EXPECTED_RECORDS && result.result == benchmark.expectedResult;
}

void printResult(const string& run, const BenchmarkResult& result) {
    cout << left << setw(8) << run
         << right << fixed << setprecision(3)
         << setw(12) << result.loadMs
         << setw(13) << result.queryMs
         << setw(13) << result.totalMs << "\n";
}

void appendCsvRow(ofstream& output, const string& timestamp, const BenchmarkOptions& options,
                  const string& benchmarkName, const string& sample, size_t datasetBytes,
                  const BenchmarkResult& result) {
    output << "1"
           << "," << csvField(timestamp)
           << "," << csvField(getVersionId())
           << "," << csvField(options.machine)
           << "," << csvField(STREAMLINE_SYSTEM_NAME)
           << "," << csvField(STREAMLINE_SYSTEM_PROCESSOR)
           << "," << csvField(STREAMLINE_COMPILER)
           << "," << csvField(STREAMLINE_BUILD_TYPE)
           << "," << csvField(DATASET_NAME)
           << "," << datasetBytes
           << "," << csvField(benchmarkName)
           << "," << result.records
           << "," << csvField(sample)
           << fixed << setprecision(3)
           << "," << result.loadMs
           << "," << result.queryMs
           << "," << result.totalMs << "\n";
}

void appendCsv(const BenchmarkOptions& options, const vector<BenchmarkRun>& benchmarkRuns) {
    filesystem::path parent = options.output.parent_path();
    if (!parent.empty()) {
        filesystem::create_directories(parent);
    }

    bool writeHeader = !filesystem::exists(options.output) || filesystem::file_size(options.output) == 0;
    ofstream output(options.output, ios::app);
    if (!output.is_open()) {
        throw runtime_error("could not open CSV output '" + options.output.string() + "'");
    }

    if (writeHeader) {
        output << "schema_version,timestamp_utc,version,machine,os,architecture,compiler,build_type,"
                  "dataset,dataset_bytes,query,records,sample,load_ms,query_ms,total_ms\n";
    }

    string timestamp = utcTimestamp();
    size_t datasetBytes = filesystem::file_size(DATASET_PATH);
    for (const BenchmarkRun& run : benchmarkRuns) {
        for (size_t i = 0; i < run.results.size(); i++) {
            appendCsvRow(output, timestamp, options, run.benchmark.name,
                         to_string(i + 1), datasetBytes, run.results[i]);
        }
        appendCsvRow(output, timestamp, options, run.benchmark.name,
                     "median", datasetBytes, run.median);
    }

    if (!output) {
        throw runtime_error("could not write CSV output '" + options.output.string() + "'");
    }
}

void printUsage() {
    cout << "usage: benchmark [--machine <label>] [--output <csv-path>]\n";
}

int main(int argc, char** argv) {
    try {
        BenchmarkOptions options = parseOptions(argc, argv);
        if (options.showHelp) {
            printUsage();
            return 0;
        }

        vector<BenchmarkRun> benchmarkRuns;

        cout << "Streamline Benchmark\n"
             << "Version: " << getVersionId() << "\n"
             << "Machine: " << options.machine << "\n"
             << "Dataset: " << DATASET_NAME << "\n";

        for (const BenchmarkCase& benchmark : BENCHMARK_CASES) {
            BenchmarkResult warmup = runBenchmark(benchmark);
            if (!validResult(benchmark, warmup)) {
                cerr << "benchmark: unexpected result for " << benchmark.name << "\n";
                return 1;
            }

            vector<BenchmarkResult> results;
            results.reserve(RUNS);
            for (int i = 0; i < RUNS; i++) {
                BenchmarkResult result = runBenchmark(benchmark);
                if (!validResult(benchmark, result)) {
                    cerr << "benchmark: unexpected result for " << benchmark.name << "\n";
                    return 1;
                }
                results.push_back(result);
            }

            cout << "\nBenchmark: " << benchmark.name << "\n"
                 << "Query: " << benchmark.query << "\n"
                 << "Records: " << results[0].records << "\n\n"
                 << left << setw(8) << "Run"
                 << right << setw(12) << "Load (ms)"
                 << setw(13) << "Query (ms)"
                 << setw(13) << "Total (ms)" << "\n";
            for (size_t i = 0; i < results.size(); i++) {
                printResult(to_string(i + 1), results[i]);
            }

            vector<BenchmarkResult> sortedResults = results;
            sort(sortedResults.begin(), sortedResults.end(), [](const BenchmarkResult& a,
                                                                const BenchmarkResult& b) {
                return a.totalMs < b.totalMs;
            });
            BenchmarkResult median = sortedResults[sortedResults.size() / 2];
            cout << string(46, '-') << "\n";
            printResult("Median", median);
            benchmarkRuns.push_back({benchmark, move(results), median});
        }

        appendCsv(options, benchmarkRuns);
        cout << "\nCSV: " << options.output << "\n";
    } catch (const exception& e) {
        cerr << "benchmark: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
