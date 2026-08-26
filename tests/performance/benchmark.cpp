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
const int RUNS = 5;

struct BenchmarkCase {
    string name;
    string query;
    string expectedResult;
};

struct BenchmarkDataset {
    string name;
    filesystem::path path;
    size_t expectedRecords;
    vector<BenchmarkCase> cases;
};

const vector<BenchmarkDataset> BENCHMARK_DATASETS = {
    {
        "json/students.json",
        PROJECT_ROOT / "json/students.json",
        85032,
        {
            {"nested_get", R"(GET(42516, "address", "city"))", "\"Riverside\""},
            {"average", R"(AVERAGE(GET("gpa")))", "3.00217"},
            {"filter_none", R"(FILTER(GET("gpa") > 4))", "[]"},
            {"sort", R"(SORT(GET("gpa"), DESC))", ""},
            {"limit", "LIMIT(100)", ""},
            {"groupby", R"(GROUPBY(GET("major")))", ""},
            {"filter_all",
             R"(FILTER(GET("gpa") >= 2) | GET(85031, "student_id"))", "85032"},
            {"sort_pipeline",
             R"(SORT(GET("gpa"), DESC) | GET(0, "student_id"))", "229"},
            {"groupby_pipeline",
             R"(GROUPBY(GET("major")) | GET("Computer Science", 0, "student_id"))", "4"}
        }
    },
    {
        "json/cars.json",
        PROJECT_ROOT / "json/cars.json",
        70792,
        {
            {"nested_get", R"(GET(35396, "dealer", "rating"))", "4.6"},
            {"average", R"(AVERAGE(GET("price_usd")))", "62258.6"},
            {"filter_none", R"(FILTER(GET("price_usd") > 120000))", "[]"},
            {"sort", R"(SORT(GET("price_usd"), DESC))", ""},
            {"limit", "LIMIT(100)", ""},
            {"groupby", R"(GROUPBY(GET("make")))", ""},
            {"filter_all",
             R"(FILTER(GET("price_usd") >= 0) | GET(70791, "car_id"))", "70792"},
            {"sort_pipeline",
             R"(SORT(GET("price_usd"), DESC) | GET(0, "car_id"))", "42250"},
            {"groupby_pipeline",
             R"(GROUPBY(GET("make")) | GET("Kia", 0, "car_id"))", "1"}
        }
    },
    {
        "json/housing.json",
        PROJECT_ROOT / "json/housing.json",
        47587,
        {
            {"nested_get", R"(GET(23793, "address", "city"))", "\"Rochester\""},
            {"average", R"(AVERAGE(GET("price_usd")))", "2.54748e+06"},
            {"filter_none", R"(FILTER(GET("price_usd") > 5000000))", "[]"},
            {"sort", R"(SORT(GET("price_usd"), DESC))", ""},
            {"limit", "LIMIT(100)", ""},
            {"groupby", R"(GROUPBY(GET("property_type")))", ""},
            {"filter_all",
             R"(FILTER(GET("price_usd") >= 0) | GET(47586, "listing_id"))", "47587"},
            {"sort_pipeline",
             R"(SORT(GET("price_usd"), DESC) | GET(0, "listing_id"))", "20024"},
            {"groupby_pipeline",
             R"(GROUPBY(GET("property_type")) | GET("Single Family", 0, "listing_id"))", "1"}
        }
    },
    {
        "json/movies.json",
        PROJECT_ROOT / "json/movies.json",
        43315,
        {
            {"nested_get", R"(GET(21657, "ratings", "viewer_score"))", "6.6"},
            {"average", R"(AVERAGE(GET("runtime_minutes")))", "140.059"},
            {"filter_none", R"(FILTER(GET("runtime_minutes") > 210))", "[]"},
            {"sort", R"(SORT(GET("runtime_minutes"), DESC))", ""},
            {"limit", "LIMIT(100)", ""},
            {"groupby", R"(GROUPBY(GET("country")))", ""},
            {"filter_all",
             R"(FILTER(GET("runtime_minutes") >= 0) | GET(43314, "movie_id"))", "43315"},
            {"sort_pipeline",
             R"(SORT(GET("runtime_minutes"), DESC) | GET(0, "movie_id"))", "28"},
            {"groupby_pipeline",
             R"(GROUPBY(GET("country")) | GET("Mexico", 0, "movie_id"))", "1"}
        }
    }
};

const BenchmarkCase LOAD_BENCHMARK = {"load", "", ""};

struct BenchmarkResult {
    size_t records;
    bool valid;
    double loadMs;
    double queryMs;
    double totalMs;
};

struct BenchmarkRun {
    string datasetName;
    size_t datasetBytes;
    BenchmarkCase benchmark;
    vector<BenchmarkResult> results;
    BenchmarkResult median;
};

struct BenchmarkOptions {
    string machine;
    filesystem::path output;
    string dataset;
    bool outputWasSet = false;
    bool datasetWasSet = false;
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
        } else if (argument == "--dataset") {
            if (i + 1 >= argc) {
                throw invalid_argument("--dataset requires a name");
            }
            options.dataset = argv[++i];
            options.datasetWasSet = true;
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

BenchmarkResult runLoadBenchmark(const BenchmarkDataset& dataset) {
    Session session;

    auto start = chrono::steady_clock::now();
    uploadFile(dataset.path.string(), session);
    auto finished = chrono::steady_clock::now();

    double loadMs = chrono::duration<double, milli>(finished - start).count();
    bool valid = static_cast<size_t>(session.records) == dataset.expectedRecords;

    return {static_cast<size_t>(session.records), valid, loadMs, 0, loadMs};
}

BenchmarkResult runQueryBenchmark(Session& session, const BenchmarkDataset& dataset,
                                  const BenchmarkCase& benchmark) {
    ostringstream ignored;
    streambuf* normalOutput = cout.rdbuf(ignored.rdbuf());
    string result;
    auto queryStarted = chrono::steady_clock::now();
    try {
        result = executeQuery(session, benchmark.query);
    } catch (...) {
        cout.rdbuf(normalOutput);
        throw;
    }
    auto finished = chrono::steady_clock::now();
    cout.rdbuf(normalOutput);

    double queryMs = chrono::duration<double, milli>(finished - queryStarted).count();
    bool valid = static_cast<size_t>(session.records) == dataset.expectedRecords
                 && result.rfind("Error:", 0) != 0
                 && (benchmark.expectedResult.empty() || result == benchmark.expectedResult);

    return {static_cast<size_t>(session.records), valid, 0, queryMs, queryMs};
}

void printResult(const string& run, const BenchmarkResult& result) {
    cout << left << setw(8) << run
         << right << fixed << setprecision(3)
         << setw(12) << result.loadMs
         << setw(13) << result.queryMs
         << setw(13) << result.totalMs << "\n";
}

void appendCsvRow(ofstream& output, const string& timestamp, const BenchmarkOptions& options,
                  const string& datasetName, size_t datasetBytes, const string& benchmarkName,
                  const string& sample, const BenchmarkResult& result) {
    output << "1"
           << "," << csvField(timestamp)
           << "," << csvField(getVersionId())
           << "," << csvField(options.machine)
           << "," << csvField(STREAMLINE_SYSTEM_NAME)
           << "," << csvField(STREAMLINE_SYSTEM_PROCESSOR)
           << "," << csvField(STREAMLINE_COMPILER)
           << "," << csvField(STREAMLINE_BUILD_TYPE)
           << "," << csvField(datasetName)
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
    for (const BenchmarkRun& run : benchmarkRuns) {
        for (size_t i = 0; i < run.results.size(); i++) {
            appendCsvRow(output, timestamp, options, run.datasetName, run.datasetBytes,
                         run.benchmark.name, to_string(i + 1), run.results[i]);
        }
        appendCsvRow(output, timestamp, options, run.datasetName, run.datasetBytes,
                     run.benchmark.name, "median", run.median);
    }

    if (!output) {
        throw runtime_error("could not write CSV output '" + options.output.string() + "'");
    }
}

void printUsage() {
    cout << "usage: benchmark [--machine <label>] [--output <csv-path>] [--dataset <name>]\n";
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
             << "Machine: " << options.machine << "\n";

        bool datasetFound = false;
        for (const BenchmarkDataset& dataset : BENCHMARK_DATASETS) {
            if (options.datasetWasSet && dataset.name != options.dataset
                && dataset.path.filename().string() != options.dataset) {
                continue;
            }
            datasetFound = true;

            cout << "\nDataset: " << dataset.name << "\n";
            size_t datasetBytes = filesystem::file_size(dataset.path);

            BenchmarkResult loadWarmup = runLoadBenchmark(dataset);
            if (!loadWarmup.valid) {
                cerr << "benchmark: unexpected record count for " << dataset.name << "\n";
                return 1;
            }

            vector<BenchmarkResult> loadResults;
            loadResults.reserve(RUNS);
            for (int i = 0; i < RUNS; i++) {
                BenchmarkResult result = runLoadBenchmark(dataset);
                if (!result.valid) {
                    cerr << "benchmark: unexpected record count for " << dataset.name << "\n";
                    return 1;
                }
                loadResults.push_back(move(result));
            }

            cout << "\nBenchmark: load\n"
                 << "Records: " << loadResults[0].records << "\n\n"
                 << left << setw(8) << "Run"
                 << right << setw(12) << "Load (ms)"
                 << setw(13) << "Query (ms)"
                 << setw(13) << "Total (ms)" << "\n";
            for (size_t i = 0; i < loadResults.size(); i++) {
                printResult(to_string(i + 1), loadResults[i]);
            }

            vector<BenchmarkResult> sortedLoadResults = loadResults;
            sort(sortedLoadResults.begin(), sortedLoadResults.end(), [](const BenchmarkResult& a,
                                                                       const BenchmarkResult& b) {
                return a.totalMs < b.totalMs;
            });
            BenchmarkResult loadMedian = sortedLoadResults[sortedLoadResults.size() / 2];
            cout << string(46, '-') << "\n";
            printResult("Median", loadMedian);
            benchmarkRuns.push_back({dataset.name, datasetBytes, LOAD_BENCHMARK,
                                     move(loadResults), loadMedian});

            Session session;
            uploadFile(dataset.path.string(), session);
            if (static_cast<size_t>(session.records) != dataset.expectedRecords) {
                cerr << "benchmark: unexpected record count for " << dataset.name << "\n";
                return 1;
            }

            for (const BenchmarkCase& benchmark : dataset.cases) {
                BenchmarkResult warmup = runQueryBenchmark(session, dataset, benchmark);
                if (!warmup.valid) {
                    cerr << "benchmark: unexpected result for " << dataset.name
                         << " " << benchmark.name << "\n";
                    return 1;
                }

                vector<BenchmarkResult> results;
                results.reserve(RUNS);
                for (int i = 0; i < RUNS; i++) {
                    BenchmarkResult result = runQueryBenchmark(session, dataset, benchmark);
                    if (!result.valid) {
                        cerr << "benchmark: unexpected result for " << dataset.name
                             << " " << benchmark.name << "\n";
                        return 1;
                    }
                    results.push_back(move(result));
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
                benchmarkRuns.push_back({dataset.name, datasetBytes, benchmark,
                                         move(results), median});
            }
        }

        if (options.datasetWasSet && !datasetFound) {
            throw invalid_argument("unknown dataset '" + options.dataset + "'");
        }

        appendCsv(options, benchmarkRuns);
        cout << "\nCSV: " << options.output << "\n";
    } catch (const exception& e) {
        cerr << "benchmark: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
