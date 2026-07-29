---
title: JSON/JSONL Parser — Design
---

# Design

This page documents the system design for the JSON/JSONL parser. See the
[repo README](https://github.com/UCR-CS-179K-Summer-2026/project1) for the
project summary and [Sprint 1 plan](https://github.com/UCR-CS-179K-Summer-2026/project1/blob/main/week1/PLAN.md)
for current task assignments.

## System Architecture

Three modules, each owned by one team member for Sprint 1:

```
students.jsonl (file)
   │
   ▼
[ JSON parser ]  --JsonValue-->  [ JSONL reader / lookup engine ]  --LookupResult-->  [ CLI ]
  (Person 1)                              (Person 2)                                (Person 3)
  parseJson()                             readJsonlFile()                           sluice
                                           lookup()
                                           formatResult()
```

- **JSON parser** converts raw JSON text into an in-memory `JsonValue` tree.
- **JSONL reader / lookup engine** reads a `.jsonl` file line by line, parses
  each line via the JSON parser, and answers path queries (`.student.name`,
  `.scores[0]`) against the resulting tree.
- **CLI** ties the two together into the `sluice` executable, handles
  argument parsing, and owns the test suite.

## Module Interfaces

### `JsonValue` (owned by Person 1)

A tagged union representing any JSON value: `null`, `bool`, `number`,
`string`, `object` (`map<string, JsonValue>`), or `array`
(`vector<JsonValue>`).

```cpp
JsonValue parseJson(const std::string& text);
```

### JSONL reader / lookup engine (owned by Person 2)

```cpp
std::vector<JsonValue> readJsonlFile(const std::string& path);

struct LookupResult {
    bool ok;
    JsonValue value;    // valid when ok == true
    std::string error;  // valid when ok == false
};

LookupResult lookup(const JsonValue& value, const std::string& path);
std::string formatResult(const LookupResult& result);
```

Edge case handling:

- **File not found** — throws, with a message suggesting how to fix it.
- **Blank line** — stored as `null`, keeping `records[i]` aligned with line
  `i` in the file.
- **Malformed line** — a warning is printed with the line number, and the
  record is stored as `null` (same alignment reasoning as blank lines).

### CLI (owned by Person 3)

```
./sluice <file.jsonl> "<lookup expression>"
```

## Algorithms & Optimizations

_TBD — this section will grow as Sprint 1 lands. Planned direction: keep
memory usage low by streaming `.jsonl` files line-by-line rather than
loading the whole file into memory at once._

## Query Language

See the [feature list in the README](https://github.com/UCR-CS-179K-Summer-2026/project1#major-features)
for the full set of supported operations (`Get`, `Filter`, `Sort`, `Limit`,
`GroupBy`, `Average`) with examples.
