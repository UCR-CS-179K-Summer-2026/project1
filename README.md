# Streamline - A JSON/JSONL Parser

## Purpose & Function

This project is a command-line tool for parsing and querying JSON and JSONL
(newline-delimited JSON) files. It's built with performance in mind — the
goal is to read and query JSON data while keeping memory usage low, so it
stays usable on large files and modest hardware.

The tool supports a small query language for pulling data out of JSON
records: getting a field, filtering, sorting, limiting, grouping, and
aggregating with possibly more to come!

Design details (system architecture, module interfaces, and algorithms) are
documented on our [project design page](https://ucr-cs-179k-summer-2026.github.io/project1/) if any future developers want to contribute to this project.

## Team

- Javier Herrera Jr
- Jules
- Ryan

## Language

C++

## Build & Run

Requires CMake 3.16+, a C++17 compiler, and **Git LFS** (to fetch the large JSON/JSONL datasets).

### 1. Prerequisites (Git & Git LFS)

Ensure **Git** and **Git LFS** are installed on your system:

- **macOS:**
  ```sh
  # Install Git (if not already installed) & Git LFS via Homebrew
  brew install git git-lfs
  git lfs install
  ```

- **Windows:**
  - Download and run the installer from [git-scm.com](https://git-scm.com/) (Git LFS is bundled automatically), or install via **winget**:
    ```sh
    winget install Git.Git
    winget install GitHub.GitLFS
    git lfs install
    ```

- **Linux (Debian/Ubuntu):**
  ```sh
  sudo apt-get update && sudo apt-get install git git-lfs
  git lfs install
  ```

### 2. Clone the Repository

```sh
git clone https://github.com/UCR-CS-179K-Summer-2026/project1.git
cd project1
```

### 3. Download Data Files

After cloning, pull the actual dataset files tracked by Git LFS:

```sh
git lfs pull
```

### 4. Optional Readline Support (macOS)

On macOS, install Readline if you want to use the arrow keys to go through
past commands when using the streamline build:

```sh
brew install readline
```

The program still builds without Readline, but the input will be more basic.
Use `Command+V` to paste into the terminal.

### 5. Configure & Build

```sh
cmake -B build
cmake --build build --parallel
```

The build creates three programs: `streamline` for the CLI, `tests` for the
correctness tests, and `benchmark` for performance testing. Run them from
the main project folder:

```sh
./build/streamline
./build/tests
./build/benchmark
```

The tests use the sample files in `tests/data`. Run the tests from the main
project folder, or give the test program the path to the data folder.

## Using the CLI

Start it with no arguments and you get a menu:

| Command | What it does                    |
| ------- | ------------------------------- |
| `u`     | upload a `.json`/`.jsonl` file  |
| `s`     | search & query the loaded file  |
| `m`     | show the menu again             |
| `q`     | quit                            |

After a file uploads, the program goes straight to the query prompt. When a
query finishes, just type the next one. The file stays loaded instead of being
read again each time. Type `q` or `quit` when you are done.

```
> u
Enter path to your file: week1/students.json
Enter your search query: GET("student", 0, "name")
"Ryan"
Enter your search query: GET("student", 1, "scores", 0)
95
Enter your search query: q
Thanks for choosing our program!
```

`GET` takes field names and array indexes in the order they should be
followed. Other operations can be joined with `|` to form a query pipeline.

## Language Reference

### Get

Retrieve the value at a given path in a JSON object or array.

- **Input:** zero or more arguments, separated by commas, applied in order to walk into the JSON. Each argument is either a key name (for objects) or a number (for array indexes). Leave the arguments empty to return the whole JSON as-is.
- **Output:** the value found after following the path.

```
Query:  GET("a")
JSON:   {"a": 1, "b": 2}
Result: 1
```

Give it a number instead of a key name to index into an array:

```
Query:  GET(1)
JSON:   ["x", "y", "z"]
Result: "y"
```

Chain multiple arguments to reach a nested value, mixing keys and indexes as needed:

```
Query:  GET("student", 1, "scores", 0)
JSON:   {"student": [{"scores": [88]}, {"scores": [95, 70]}]}
Result: 95
```

### Filter & Comparison

Keep only the records matching a comparison.

- **Input:** a boolean expression containing one or more paths.
- **Output:** an array of objects for which the expression holds true.

```
Query:  FILTER(GET("a") > 1)
JSON:   [{"a": 1, "b": 2}, {"a": 2, "b": 2}]
Result: [{"a": 2, "b": 2}]
```

### Sort

Order an array of records by a field, ascending or descending.

- **Input:** a `GET` target and either `ASC` or `DESC`.
- **Output:** a sorted array.

```
Query:  SORT(GET("a"), ASC)
JSON:   [{"a": 1, "b": 2}, {"a": 0, "b": 3}]
Result: [{"a": 0, "b": 3}, {"a": 1, "b": 2}]
```

### Limit

Return only the first N records.

- **Input:** a number N.
- **Output:** an array of size N.

```
Query:  LIMIT(1)
JSON:   [{"a": 1, "b": 2}, {"a": 0, "b": 3}]
Result: [{"a": 1, "b": 2}]
```

### GroupBy

Bucket records by the value of a field.

- **Input:** a path.
- **Output:** an object whose keys are the distinct values of that field, each mapped to an array of the records that share it.

```
Query:  GROUPBY(GET("a"))
JSON:   [{"a": 1, "b": 2}, {"a": 0, "b": 3}, {"a": 1, "b": 5}]
Result: {
  "0": [{"a": 0, "b": 3}],
  "1": [{"a": 1, "b": 2}, {"a": 1, "b": 5}]
}
```

### Average

Compute the mean of a field, optionally after grouping.

- **Input:** a path to a number value.
- **Output:** a number (or, after a `GROUPBY`, one number per group).

```
Query:  GROUPBY(GET("a")) | AVERAGE(GET("price"))
JSON:   [{"id": 1, "city": "Riverside", "price": 450000}, {"id": 2, "city": "Riverside", "price": 480000}, {"id": 3, "city": "Los Angeles", "price": 800000}, {"id": 4, "city": "Riverside", "price": 460000}]
Result: {"Riverside": 463333.33, "Los Angeles": 800000}
```

`AVERAGE` can also be used *inside* a `FILTER` condition instead of as its
own pipeline stage, to keep only the records (or groups) whose own average
clears a threshold. `AVERAGE()` with no argument averages the numbers in
the current array being tested; `AVERAGE(GET("path"))` averages that field
across the objects in it.

```
Query:  FILTER(AVERAGE() > 4)
JSON:   [[1, 3, 5], [2, 4, 6], [3, 6, 12, 18]]
Result: [[3, 6, 12, 18]]
```

```
Query:  FILTER(AVERAGE(GET("price")) > 50)
JSON:   [[{"price": 100}, {"price": 200}, {"price": 300}], [{"price": 10}, {"price": 20}]]
Result: [[{"price": 100}, {"price": 200}, {"price": 300}]]
```

### Pipeline Operator

Chain multiple operations together so the output of one stage becomes the
input to the next.

- **Input:** two or more operations separated by `|`.
- **Output:** the result of the final stage, after each earlier stage has
  transformed the data in turn.

```
Query:  FILTER(GET("a") > 0) | SORT(GET("a"), DESC) | LIMIT(1)
JSON:   [{"a": 1, "b": 2}, {"a": 0, "b": 3}, {"a": 2, "b": 5}]
Result: [{"a": 2, "b": 5}]
```

## Performance Benchmarks

The benchmark suite tests performance across a diverse range of dataset sizes and formats:

- **20 MB:** `json/ram.jsonl` (141K records)
- **~50 MB:** `json/students.json` (85K records), `json/cars.json` (71K records), `json/housing.json` (48K records), `json/movies.json` (43K records)
- **100 MB:** `json/food.jsonl` (737K records)
- **809 MB:** `json/kaggle_datasets/matchups.json` (2.4M records)
- **1.3 GB:** `json/kaggle_datasets/spotify.jsonl` (498K records)

Every load and query gets one warmup run and five recorded runs. Loads are measured separately, and every query runs on a dataset that was loaded before its timer starts. The program prints each run and its median, then appends the results to a CSV file in `benchmarks/results/`.

- `nested_get` reads a nested value from the middle of the file.
- `average` averages a numeric field.
- `filter_none` scans the file without matching records.
- `sort` runs SORT by itself.
- `limit` runs LIMIT by itself.
- `groupby` runs GROUPBY by itself.
- `filter_all` matches every record and finishes with a GET.
- `sort_pipeline` sorts the records and finishes with a GET.
- `groupby_pipeline` groups the records and finishes with a GET.

### Running Benchmarks

```sh
# 1. Run all benchmarks across all available datasets (default)
./build/benchmark

# 2. Benchmark a specific dataset only (e.g. 20MB, 50MB, 100MB, or Kaggle files)
./build/benchmark --dataset ram.jsonl
./build/benchmark --dataset students.json
./build/benchmark --dataset food.jsonl
./build/benchmark --dataset matchups.json
./build/benchmark --dataset spotify.jsonl

# 3. Label your hardware environment for comparative analysis
./build/benchmark --machine "macbook-pro-m2"

# 4. Save results to a custom CSV location
./build/benchmark --output benchmarks/results/my-run.csv

# 5. Combine flags for full control
./build/benchmark --dataset ram.jsonl --machine "lab-mac" --output benchmarks/results/ram-test.csv
```

### Benchmark Command Options

| Option | Required? | Description |
|---|---|---|
| *(no flags)* | No | Runs every dataset in the benchmark suite sequentially. |
| `--dataset <name>` | Optional | Limits the benchmark run to a single configured dataset (e.g., `ram.jsonl`, `students.json`, `food.jsonl`, `matchups.json`, `spotify.jsonl`). |
| `--machine <label>` | Optional | Labels the machine name recorded in the CSV (e.g., `macbook-pro-m2`, `lab-linux`). Auto-detects hostname if omitted. |
| `--output <path>` | Optional | Custom CSV filepath. Defaults to `benchmarks/results/<machine-name>.csv`. |
| `--help` / `-h` | Optional | Displays the usage synopsis. |

> **Note on Custom Datasets:** The `--dataset` option selects from the datasets configured in the benchmark suite (since automated benchmarking requires predefined queries, expected field schemas, and verification assertions). To load and query any arbitrary `.json` or `.jsonl` file interactively, use the CLI tool (`./build/streamline`). To add a new custom file permanently to the benchmark suite, add an entry to the `BENCHMARK_DATASETS` array in `tests/performance/benchmark.cpp`.

The CSV records the version, machine, build information, query name, record count,
and load, query, and total times. Use `load_ms` to compare load cases and
`query_ms` to compare query cases. Query times do not include loading the file.
Standalone SORT, LIMIT, and GROUPBY include the time needed to format their
full results.

