# Sprint 4 Plan

## Recap: Sprint 3

Shipped: hand-rolled number validation in both scanners, replacing
`std::regex` (Jules); a fast path in `get()`'s field matching that skips
`unescapeString()` when a candidate key has no backslash, plus a
multithreaded parallel-reduction `AVERAGE` gated behind a size threshold
(Brent's-theorem-informed, so small inputs don't pay thread-spinup cost)
(Javier); `readline`/`libedit` wired into `promptLine()` via
`find_path`/`find_library` in `CMakeLists.txt`, giving arrow-key history and
in-line editing in the interactive menu on macOS/Linux, and `uploadFile()`
switched from `ostringstream` streaming to a single sized `read()` (Ryan).
Also landed: real multi-record `.jsonl` parsing (`Parser::parse()` now
loops per line into an `ArrayValue` instead of capturing only the first
record — the Sprint 2 gap is fixed), and an updated
[design page](https://ucr-cs-179k-summer-2026.github.io/project1/)
documenting the optimizations with before/after numbers.

**Known gaps carried into Sprint 4, not yet fixed:**

- `QueryScanner::scan()` still recognizes `FILTER`/`GROUPBY`/`AVERAGE`/etc.
  with one hand-written character-chain per keyword — the lookup-table
  rewrite was scoped for Sprint 3 but didn't land.
- `FILTER`/`GROUPBY` still deep-copy every matching `JSONValue`
  (`matches.push_back(record)` in `excecuteQuery()`) instead of carrying
  indices until final formatting.
- `formatValue()` still returns a new `string` at every nesting level and
  concatenates them back together instead of appending into one shared
  output buffer.
- `excecuteQuery()`'s branches (`Get`, `Filter`, `Sort`, `Limit`,
  `GroupBy`, `Average`, ...) still duplicate shape-checking/error
  construction — not yet pulled into a shared helper.
- A bare scalar value followed by trailing content (e.g. `"a": 1` with no
  wrapping braces, plus junk after it) can still slip through unrejected —
  `Parser::parse()`'s `JSON` branch returns after the first `parseObject()`
  without checking `currToken.type == End`. Open since Sprint 2, never
  assigned to anyone.

## Sprint 4 Goals

For this sprint, the goal is to try and get a final, optimized product to be ready for the final week of the class. We will also cleanup our website, fix small and empty code, and analyze more ways to optimize. 

## Assignments

### Person 1 (Jules) — Query parser optimization (carried over)

**Responsibilities**

- Finish the `QueryScanner::scan()` keyword-table rewrite scoped for
  Sprint 3: replace the one-chain-per-keyword matching (`FILTER`,
  `GROUPBY`, `AVERAGE`, `SORT`, `LIMIT`, `GET`, `ASC`, `DESC`, `AND`, `OR`)
  with a single lookup checked after scanning an identifier.
- Look Into RegEx to DFA converter possibly and analyze parser for any other imporvements
- Fix `Parser::parse()`'s `JSON` branch to reject trailing content after a
  top-level scalar (check `currToken.type == End` after `parseObject()`
  returns) — open since Sprint 2, still unassigned until now.

**Example milestone:**

    Before/after benchmark of QueryParser::parse() on a query with several
    chained pipeline stages and a multi-condition FILTER, showing the
    keyword-matching change actually moved the number (same milestone as
    Sprint 3 — still not measured because the change didn't land).

### Person 2 (Javier) — Feature cleanup & execution-engine optimization (carried over)

**Responsibilities**

- Stop deep-copying every matching `JSONValue` in `FILTER`/`GROUPBY` —
  carry indices or pointers through the pipeline and only materialize real
  copies at the final formatting step.
- Change `formatValue()` to append into one shared output buffer passed by
  reference through the recursion, instead of returning/concatenating a
  new `string` at every nesting level.
- Pass over `excecuteQuery()`'s branches for duplicated shape-checking/
  error-construction logic that can be pulled into a shared helper without
  changing behavior.

**Example milestone:**

    Re-run the FILTER-matching-~everything vs. matching-nothing benchmark
    on json/students.json and show the gap shrink now that matches aren't
    deep-copied.

### Person 3 (Ryan) — Build system & CLI cleanup

**Responsibilities**

- Continue on optimizing GroupBy function to see if there's any room for optimization
- Test for if super big JSON files work
- Try to find anymore optimization opportunities

**Example milestone:**

    GroupBy function now runs 3ms faster than before with some other structure

## Lab Session Check-In (Monday August 17th, 2026)

- Went over feedback given after the demo to try and analyze what exactly we should work on
- Specifically, we went over RegEx to DFA converter, how string_view works, and other optimization opportunities. 
