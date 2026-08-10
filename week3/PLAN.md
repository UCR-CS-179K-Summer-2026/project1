# Sprint 3 Plan

## Recap: Sprint 2

Shipped: the full query language — `GET`, `FILTER` (with `AND`/`OR`), `SORT`,
`LIMIT`, `GROUPBY`, `AVERAGE`, and unlimited `|` chaining — implemented
against Jules's tokenizer/AST-based `QueryParser`, running through a
`Session`-based CLI instead of Sprint 1's fixed-record-list model. Also
landed: correct `\uXXXX`/surrogate-pair decoding (emoji now decode and
compare correctly), a pass of crash-safety fixes (type-mismatch comparisons,
non-numeric `AVERAGE`, invalid `GROUPBY` keys, malformed query syntax — all
previously uncaught exceptions that killed the whole program, now clean
errors), a real regression test suite, a performance benchmark harness, and
an updated [design page](https://ucr-cs-179k-summer-2026.github.io/project1/)
reflecting the current architecture.

**Known gaps carried into Sprint 3, not yet fixed:** a multi-record `.jsonl`
upload only captures its first record (`Session` holds one `JSONValue`,
parsed as a single document rather than line-by-line); a bare scalar value
followed by trailing content (e.g. `"a": 1` with no wrapping braces) can
slip through unrejected; `FILTER`/`GROUPBY` still deep-copy every matching
record instead of carrying indices until the final step.

## Sprint 3 Goals

Sprint 2 was about getting everything working. Sprint 3 is an optimization
and cleanup pass on top of a system that already works — each person owns
the layer they already know best. We will also aim to document our algorithms of optimization and aim to create diagrams to demonstrat our structure of things. 

## Assignments

### Person 1 (Jules) — Query parser optimization

**Responsibilities**

- `QueryScanner::scan()` currently recognizes each keyword (`FILTER`,
  `GROUPBY`, `AVERAGE`, `SORT`, `LIMIT`, `GET`, `ASC`, `DESC`) with its own
  hand-written chain of character comparisons
  (`*curr == 'F' && curr+5<end && *(curr+1)=='I' && ...`). Replace this with
  a single, faster, more maintainable lookup (a small keyword table checked
  after scanning an identifier, for example) instead of one bespoke
  character-chain per keyword.
- Audit `ExpressionNode`/`NodeId` allocation in `QueryParser` for
  unnecessary copies — confirm `string_view`s are threaded through without
  copying into owned `string`s until a literal actually needs decoding.
- Look at whether `parseComparison`/`parseLogicalAnd`/`parseLogicalOr`'s
  recursive descent does any redundant work on deeply chained
  `AND`/`OR` conditions, and whether that's worth precomputing anything for.

**Example milestone:**

    Before/after benchmark of QueryParser::parse() on a query with several
    chained pipeline stages and a multi-condition FILTER, showing the
    keyword-matching change actually moved the number.

### Person 2 (Javier) — Feature cleanup & execution-engine optimization

**Responsibilities**

- Stop deep-copying every matching `JSONValue` in `FILTER`/`GROUPBY` — carry
  indices or pointers through the pipeline and only materialize real copies
  at the final formatting step, instead of copying at every stage that
  touches the data.
- Change `formatValue()` to append into one shared output buffer passed by
  reference through the recursion, instead of returning a new `string` at
  every nesting level and concatenating them back together.
- Give `get()`'s field-matching a fast path that skips `unescapeString()`
  entirely when a key has no backslash in it (the common case), instead of
  re-decoding every candidate key on every comparison.
- Pass over `excecuteQuery()`'s five operation branches for duplicated
  logic (shape-checking, error construction) that can be pulled into a
  shared helper without changing behavior.

**Example milestone:**

    Re-run the same before/after benchmark from the optimization
    conversation this sprint (FILTER matching ~everything vs. matching
    nothing on json/students.json) and show the gap between them shrink.

### Person 3 (Ryan) — Memory management & CLI bug cleanup

**Responsibilities**

- Arrow keys don't work in the interactive menu on macOS/Linux (they do on
  Windows, which gets line editing for free from its console host). Root
  cause: `promptLine()` uses plain `getline(cin, line)`, which has no
  concept of cursor movement or history. Fix: link `readline`/`libedit` on
  non-Windows builds and swap `promptLine()` to use `readline()` — already
  scoped out this sprint: `-lreadline` resolves to macOS's built-in
  `/usr/lib/libedit.3.dylib` with no extra install needed, so this doesn't
  need a new dependency, just `find_library`/`find_path` in
  `CMakeLists.txt` guarded by `if(NOT WIN32)`, and the actual symbols
  available (confirmed: `readline()`, `add_history()`).
- Audit `Session`/`CliState` for unnecessary copies — `uploadFile()` reads
  the whole file into a `string` via `ostringstream`, then `.str()` copies
  the whole buffer again; see if that second copy can be avoided for large
  files.
- `integration.cpp`'s `readFile()`/`fileContents()` and `file-reader.cpp`'s
  `uploadFile()` independently re-implement the same "open file, read it
  into a string" logic. Decide whether that duplication is worth
  collapsing into one shared function now that both exist.
- Known crash-adjacent gap, not yet fixed: `QueryParser::parse()`'s
  exception isn't caught anywhere inside `excecuteQuery()` itself — it's
  only caught by whichever caller invoked it. Confirm every call site
  actually does catch it (menu search, direct `argv` form) and there's no
  path that doesn't.

**Example milestone:**

    Run the interactive menu, use the up-arrow to recall a previous query
    and left/right to edit it mid-line, on macOS.

## Lab Session Check-Ins

_(fill in as they happen)_
