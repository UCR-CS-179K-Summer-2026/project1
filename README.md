# JSON/JSONL Parser

## Purpose & Function

This project is a command-line tool for parsing and querying JSON and JSONL
(newline-delimited JSON) files. It's built with performance in mind — the
goal is to read and query JSON data while keeping memory usage low, so it
stays usable on large files and modest hardware.

The tool supports a small query language for pulling data out of JSON
records: getting a field, filtering, sorting, limiting, grouping, and
aggregating with possibly more to come!

Design details (system architecture, module interfaces, and algorithms) are
documented on our [project design page](https://ucr-cs-179k-summer-2026.github.io/project1/).

## Team

- Javier Herrera Jr
- Jules
- Ryan

## Language

C++

## Build & Run

Requires CMake 3.16+ and a C++17 compiler.

```sh
cmake -B build
cmake --build build
```

The main binaries in `build/` are `streamline` (the CLI), `tests` (the
correctness suite), `benchmark_legacy` (the legacy benchmark), and
`benchmark_current` (the current query benchmark). Run them from the repo
root.

```sh
./build/streamline        # opens the menu
./build/tests             # runs the test suite
./build/benchmark_legacy  # runs the legacy benchmark
./build/benchmark_current # runs the current query benchmark
```

The test suite reads its sample files from `tests/data`, and that path is
resolved against your current directory — so run it from the repo root, or
pass the directory yourself with `./build/tests path/to/tests/data`.

`CMakeLists.txt` lives at the repo root and builds from `week2/` (Sprint
2's code) — it gets repointed at whichever folder holds the current
sprint's code as later weeks land. See the
[Build System section](https://ucr-cs-179k-summer-2026.github.io/project1/#build-system)
of the design page for why we use CMake instead of manual `g++` commands.

## Using the CLI

Start it with no arguments and you get a menu:

| Command | What it does                    |
| ------- | ------------------------------- |
| `u`     | upload a `.json`/`.jsonl` file  |
| `s`     | search & query the loaded file  |
| `m`     | show the menu again             |
| `q`     | quit                            |

Upload a file once and you can run as many queries against it as you want
— the records stay loaded, so `s` doesn't re-read the file each time.

```
> u
Enter path to your file: week1/students.json
> s
Enter your search query: GET("student", 0, "name")
"Ryan"
> s
Enter your search query: GET("student", 1, "scores", 0)
95
> q
Thanks for choosing our program!
```

`GET` takes field names and array indexes in the order they should be
followed. Other operations can be joined with `|` to form a query pipeline.

## Version Control

The project uses version IDs to keep track of which code was used for each
performance test. The format is `week<number>-v<number>`.

- `week2-v1` is the Week 2 baseline.
- `week2-v2` is the first optimized Week 2 version.
- `week2-v3` is the next optimized Week 2 version.
- `week3-v1` starts a new baseline for Week 3.

The current version is stored in `week2/version.cpp`. Run this to check the
version of the program that was built:

```sh
./build/streamline --version
```

Before measuring an optimization:

1. Update the version ID in `week2/version.cpp` and its expected value in
   `tests/correctness/tests.cpp`.
2. Build the program in Release mode.
3. Run the correctness tests.
4. Run the performance tests.
5. Save the version ID with the performance results.

Every code change being measured gets a new version ID. A version ID should
not be reused after its performance results have been saved. The performance
log will use these IDs to compare speed changes between versions.

This system labels builds for performance comparisons. Git is still used
separately for source history, branches, and restoring old code.

## Benchmarks

Both Week 2 benchmarks use `json/students.json`, which is a 50 MB file with
85,032 records. The legacy benchmark uses the older `readFile()` and
string-path `get()` interfaces. The current benchmark uses `uploadFile()`,
`Session`, and the current query pipeline to run `AVERAGE(GET("gpa"))`.

Build in Release mode and run the correctness tests first:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/tests tests/data
```

Run both benchmarks from the repository root:

```sh
./build/benchmark_legacy
./build/benchmark_current
```

Each benchmark has one warm-up run followed by five measured runs. The last
row contains the median total time. Results are printed as CSV with the
benchmark name, version, dataset, query, record count, and times in
milliseconds.

The legacy benchmark measures a stable older interface, but it still
uses the code in the current build. It does not automatically run an older
source version. The current benchmark measures the same session and query
execution path used by the CLI.

Compare each benchmark only with the same benchmark from another version.
Do not compare the legacy time directly with the current query time
because they do different work. Results from different versions should use
the same computer, build type, and dataset.

## Major Features

- **Get** — retrieve the value at a given path in a JSON object.  
The input is the name of a key and the output is the value stored at said key.

    Query: `GET("a")`
    JSON: `{"a": 1, "b": 2}`
    Result: `1`

- **Filter & Comparison** — keep only the records matching a comparison.  
The input is a boolean expression containing one or more path and the output is an array of objects that hold true for the boolean expression.

    Query: `FILTER(GET("a") > 1)`
    JSON: `[{"a": 1, "b": 2}, {"a": 2, "b": 2}]`
    Result: `[{"a": 2, "b": 2}]`

- **Sort** — order an array of records by a field, ascending or descending.  
The input is a `GET` target and either `ASC` or `DESC`. The output is a sorted array.

    Query: `SORT(GET("a"), ASC)`
    JSON: `[{"a": 1, "b": 2}, {"a": 0, "b": 3}]`
    Result: `[{"a": 0, "b": 3}, {"a": 1, "b": 2}]`

- **Limit** — return only the first N records.  
The input is a number N and the output is an array of size N.

    Query: `LIMIT(1)`
    JSON: `[{"a": 1, "b": 2}, {"a": 0, "b": 3}]`
    Result: `[{"a": 1, "b": 2}]`

- **GroupBy** — bucket records by the value of a field.  
The input is a path and the output is an array with different properties as key, and an array with all items having that property as value.

    Query: `GROUPBY(GET("a"))`
    JSON: `[{"a": 1, "b": 2}, {"a": 0, "b": 3}, {"a": 1, "b": 5}]`
    Result:
    ```json
    {
      "0": [{"a": 0, "b": 3}],
      "1": [{"a": 1, "b": 2}, {"a": 1, "b": 5}]
    }
    ```

- **Average** — compute the mean of a field, optionally after grouping.  
The input is a path to a number value and the output is a number.

    Query: `GROUPBY(GET("a")) | AVERAGE(GET("price"))`
    JSON: `[{"id": 1, "city": "Riverside", "price": 450000}, {"id": 2, "city": "Riverside", "price": 480000}, {"id": 3, "city": "Los Angeles", "price": 800000}, {"id": 4, "city": "Riverside", "price": 460000}]`
    Result: `{"Riverside": 463333.33, "Los Angeles": 800000}`
