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

That produces two binaries in `build/`: `sluice` (the CLI) and `tests`
(the test suite). Run both from the repo root:

```sh
./build/sluice        # opens the menu
./build/tests         # runs the test suite
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
| `\u`    | upload a `.json`/`.jsonl` file  |
| `\s`    | search & query the loaded file  |
| `\m`    | show the menu again             |
| `\q`    | quit                            |

Upload a file once and you can run as many queries against it as you want
— the records stay loaded, so `\s` doesn't re-read the file each time. The
prompt shows which file you're working on.

```
> \u
Enter path to your file: week1/students.jsonl
File students.jsonl uploaded successfully. (4 records)
[students.jsonl] > \s
Enter your search query: .student.name
Ryan
Javier
Jules
Nobody
[students.jsonl] > \s
Enter your search query: .student.scores[0]
90
95
92
Error: field 'scores' not found
[students.jsonl] > \q
Thanks for choosing our program!
```

The query runs against every record in the file, so you get one line of
output per record. A record that doesn't have the field you asked for
reports an error on its own line instead of stopping the whole query.

Anything that isn't a `.json` or `.jsonl` file is rejected, and a path that
can't be opened or parsed just asks you for another one.

If you'd rather skip the menu, the direct form still works:

```sh
./build/sluice week1/students.jsonl ".student.name"
```

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