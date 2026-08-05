# Sprint 2 Plan

## Recap: Sprint 1 Demo (Monday, Aug 3)

Shipped: a working `.json`/`.jsonl` reader, `JSONValue` as a `std::variant`-based
value type (see the [design page](https://ucr-cs-179k-summer-2026.github.io/project1/#classes)
for the polymorphism-vs-variant writeup), path lookup (`Get`), a CMake build,
and a pass of bug fixes (crash on malformed top-level JSON, crash on
file-not-found, numbers-followed-by-whitespace parsing).

**Decision carried into Sprint 2:** we're keeping `std::variant` for
`JSONValue`. We compared it against polymorphism (what we started with),
a manual tagged union, `std::any`, and a flat/tape-style encoding
(simdjson-style) — variant wins on the combination that matters for a
3-person team on a deadline: zero per-node heap allocation, no vtable, and
compiler-enforced exhaustiveness, without the correctness risk of a manual
union or the much larger engineering lift of a tape encoding. Not
revisiting this again — build on top of it.

## Sprint 2 Goals

Sprint 1 built the foundation (read a file, look up a path). Sprint 2 is
the actual query language from the [README's Major Features](../README.md):
`FILTER`, `SORT`, `LIMIT`, `GROUPBY`, `AVERAGE`, and chaining them with `|`
(e.g. `GROUPBY(.city) | AVERAGE(.price)`), plus a real CLI in front of it.

Also in scope: **JSON string decoding currently doesn't decode anything.**
Verified this against the actual parser before writing this plan —
`{"a": "line1\nline2"}` today stores the literal two characters `\` and `n`
in the string, not a real newline, and `"grin 😀 end"` comes back
out **unchanged**, `\uXXXX` escape and all, instead of the emoji it's
supposed to represent. Raw literal UTF-8 bytes sitting directly in a file
(an actual 😀 character typed straight into the JSON, not escaped) already
pass through fine today, since the scanner treats string bytes as opaque —
the gap is specifically escape-sequence decoding: `\"`, `\\`, `\/`, `\b`,
`\f`, `\n`, `\r`, `\t`, and `\uXXXX` (including surrogate pairs, which is
how emoji are usually encoded in JSON text). This was never a Sprint 1
requirement, but it's a real correctness gap worth closing before it shows
up as a silent bug inside `FILTER`/`SORT` comparisons. We also will work on 
other weird edge cases like null bytes. 

## Assignments

### Person 1 (Jules) — Query language parser & string decoding

**Responsibilities**

- Design and implement a tokenizer/parser for the query mini-language —
  structurally the same job as the JSON parser, different grammar:
  `FILTER(<path> <op> <literal>)`, `SORT(<path>, "asc"|"desc")`,
  `LIMIT(<n>)`, `GROUPBY(<path>)`, `AVERAGE(<path>)`, chained with `|`.
- Support comparison operators inside `FILTER`: `>`, `<`, `>=`, `<=`, `==`,
  `!=`.
- Produce a structured operation list/AST that Person 2's execution engine
  can run against a `vector<JSONValue>` — this is the same handoff shape as
  Sprint 1's `Parser::parse() -> JSONValue`, just for queries instead of
  JSON.
- Implement escape-sequence decoding in the scanner/string handling: `\"`,
  `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`, `\uXXXX` (including surrogate
  pairs for codepoints outside the Basic Multilingual Plane, i.e. most
  emoji) decoded into real UTF-8 bytes, not left as literal escape text.
  This lives in the same string-scanning code Jules already owns from
  Sprint 1.
- Decide and document what happens on an invalid escape (e.g. `\q`) or an
  unpaired surrogate (`\uD83D` with no matching low surrogate) — reject
  with a clear error, matching the existing "throw with a helpful message"
  pattern rather than silently producing garbage.

**Example milestone:**

    parseQuery(`GROUPBY(.city) | AVERAGE(.price)`)
      -> [ GroupByOp{field: ".city"}, AverageOp{field: ".price"} ]

    "grin 😀 end"  ->  decodes to the actual 😀 character

### Person 2 (Javier) — Query execution engine

**Responsibilities**

- Implement each operator against a `vector<JSONValue>`, built on top of
  the existing `lookup()` from Sprint 1 rather than a new tree-walk:
    - `FilterOp` — keep only records where `lookup(record, path)` compares
      true against the literal.
    - `SortOp` — order records ascending/descending by a field's value;
      decide the comparison rule across mixed/incompatible types (e.g.
      sorting a field that's a string on some records and a number on
      others) and document it, same spirit as Sprint 1's edge-case table.
    - `LimitOp` — first N records.
    - `GroupByOp` — bucket records into `vector<pair<string, vector<JSONValue>>>`
      (or similar) keyed by a field's stringified value — same
      insertion-order-preserving-vector philosophy as `ObjectValue`, not a
      `map`, unless there's a concrete reason to switch.
    - `AverageOp` — numeric mean over a field, applied per-group when
      chained after `GroupBy`.
- Wire operators together so a pipeline (`GROUPBY | AVERAGE`) threads one
  stage's output into the next stage's input.
- Decide what happens when an operator hits a record where the field is
  missing or the wrong type (e.g. `AVERAGE` on a non-numeric field) —
  skip the record, or error out the whole query? Needs a decision and a
  doc entry, same as the JSONL malformed-line decision from Sprint 1.

**Example milestone:**

    GROUPBY(.city) | AVERAGE(.price)
      over [{"city":"Riverside","price":450000}, {"city":"Riverside","price":480000}]
      -> {"Riverside": 465000}

### Person 3 (Ryan) — CLI, directories, integration & tests

**Responsibilities**

- Expand the CLI beyond the fixed-query `demo.cpp`: accept a path that's
  either a single file **or a directory** as the first argument, plus a
  query string as the second — `./sluice <path> "<query>"`. When given a
  directory, run the query across every `.json`/`.jsonl` file inside it.
- Decide and document directory edge cases: empty directory, directory
  with no matching file extensions, one file in the directory that fails
  to parse (skip that file and continue, or abort the whole run? — same
  category of decision as Sprint 1's "one bad JSONL line" call).
- Wire the full pipeline together: read file(s) → Person 1's query parser
  → Person 2's execution engine → `formatResult()`/`formatValue()` for
  output.
- Migrate `test_lookup.cpp`'s cases into a real suite in `tests/` (already
  scaffolded), and add coverage for every new operator, chained pipelines,
  and Person 1's escape-decoding work (`\n`, `\t`, `\uXXXX` emoji cases
  especially — these are easy to silently regress).
- Update the CMake build once the CLI target exists (replacing/joining
  `demo` the way `demo`/`test_lookup` are set up now).
- Implement a couple features that can possibly use the same file for multiple 
  queries, an escape fail safe to terminate the program or start a query with 
  a new file, etc. 
- If finished early, try to see if Javier or Jules need help with the other 
  functions or implementations

**Example milestone:**

    ./sluice json/ "GROUPBY(.city) | AVERAGE(.price)"

    runs across every file in json/ and prints one merged result.

## Demo

Target for Sprint 2's demo: a chained query (`GROUPBY` piped into
`AVERAGE`) run against Ryan's large datasets in `json/`, showing it
handles escaped/emoji strings correctly, and the CLI accepting a directory
instead of a single hardcoded file. Also handling all edge cases and implement
all functions listed. 

## Lab Session Check-In (Monday August 3, 2026)

**What did we accomplish since Sprint 1?**

- We were able to decide what structure we wanted to use for our JSONValue parsing, 
we were able to get a base query file structure down, and list possible edge cases to 
account for when inputting queries

**What's left before the next demo?**

- We need to having a working query parser and custom file path reader for usability
- Hopefully by next lab, we can also have at least 2 functions working from our lists
- We also hope to have small optimizations if possible

## Lab Session Check-In (Wednesday August 5, 2026)
