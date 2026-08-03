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

cd build
./demo         # Sprint 1 demo: reads students.json, runs a lookup query
./test_lookup  # lookup() test cases
```

`demo.cpp` opens `students.json` via a relative path, and the build copies
that file next to the built binary — so you have to run `demo` from
*inside* `build/`. Running it as `./build/demo` from the repo root will
fail with a "could not open 'students.json'" error, because your shell's
working directory (the repo root) is what relative paths resolve against,
not the location of the binary itself.

`CMakeLists.txt` lives at the repo root and currently builds from `week1/`
(Sprint 1's code) — it'll get repointed at whichever folder holds the
current sprint's code as later weeks land. See the
[Build System section](https://ucr-cs-179k-summer-2026.github.io/project1/#build-system)
of the design page for why we use CMake instead of manual `g++` commands.

## Major Features

- **Get** — retrieve the value at a given path in a JSON object.

    Query: `.a`
    JSON: `{"a": 1, "b": 2}`
    Result: `1`

- **Filter & Comparison** — keep only the records matching a comparison.

    Query: `FILTER(.a > 1)`
    JSON: `[{"a": 1, "b": 2}, {"a": 2, "b": 2}]`
    Result: `[{"a": 2, "b": 2}]`

- **Sort** — order an array of records by a field, ascending or descending.

    Query: `SORT(a, "asc")`
    JSON: `[{"a": 1, "b": 2}, {"a": 0, "b": 3}]`
    Result: `[{"a": 0, "b": 3}, {"a": 1, "b": 2}]`

- **Limit** — return only the first N records.

    Query: `LIMIT(1)`
    JSON: `[{"a": 1, "b": 2}, {"a": 0, "b": 3}]`
    Result: `{"a": 1, "b": 2}`

- **GroupBy** — bucket records by the value of a field.

    Query: `GROUPBY(.a)`
    JSON: `[{"a": 1, "b": 2}, {"a": 0, "b": 3}, {"a": 1, "b": 5}]`
    Result:
    ```json
    {
      "0": [{"a": 0, "b": 3}],
      "1": [{"a": 1, "b": 2}, {"a": 1, "b": 5}]
    }
    ```

- **Average** — compute the mean of a field, optionally after grouping.

    Query: `GROUPBY(.city) | AVERAGE(.price)`
    JSON: `[{"id": 1, "city": "Riverside", "price": 450000}, {"id": 2, "city": "Riverside", "price": 480000}, {"id": 3, "city": "Los Angeles", "price": 800000}, {"id": 4, "city": "Riverside", "price": 460000}]`
    Result: `{"Riverside": 463333.33, "Los Angeles": 800000}`