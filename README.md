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

Requires CMake 3.16+ and a C++17 compiler.

```sh
cmake -B build
cmake --build build
```

The main binaries in `build/` are `streamline` (the CLI), `tests` (the
correctness suite), and `benchmark` (the performance benchmark). Run them
from the repo root.

```sh
./build/streamline        # opens the menu
./build/tests             # runs the test suite
./build/benchmark         # runs the performance benchmark
```

The test suite reads its sample files from `tests/data`, and that path is
resolved against your current directory — so run it from the repo root, or
pass the directory yourself with `./build/tests path/to/tests/data`.

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

## Version Control & Benchmarking

Builds are labeled with a version ID (`week<N>-v<N>`) so performance can be
compared across optimizations, and a separate benchmark binary measures
that performance against a large sample dataset.

```sh
./build/streamline --version   # check the current version ID
./build/benchmark               # run the performance benchmark
```

The versioning scheme, the workflow for measuring an optimization, the
benchmark's methodology, and how to compare results across versions are
documented on the [design page](https://ucr-cs-179k-summer-2026.github.io/project1/#versioning).

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
