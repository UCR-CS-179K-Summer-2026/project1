# Final Submission

## Recap: Sprint 4

- Worked on final optimizations that were spotted going into sprint 4, cleaned up benchmarks, cleaned up website for final submission

**Known gaps carried into Final Submission, not yet fixed:**

- Number validation intentionally uses `<regex>`, not the faster hand-rolled
  version it replaced — rolled back for correctness under time pressure, not
  an oversight. The hand-rolled version is still there, commented out; with
  it back in, `uploadFile()`'s load time on `students.json` drops from
  ~379ms to ~135ms (about -64%), a real and known cost we chose not to pay.
- `formatValue()` is still the largest remaining cost in the system,
  half-fixed. Numbers now format with `to_chars` instead of `ostringstream`,
  which helps, but `formatValue()` still returns a new `string` per nesting
  level instead of appending into one shared buffer — the bigger,
  structural half of the cost, still open.
- Materialization happens per-stage, not deferred all the way to
  formatting. `FILTER`/`SORT`/`LIMIT`/`GROUPBY` only touch a record once
  it's known to survive, but each stage still builds a real, owned array
  instead of carrying indices through to the final format step.
- `GROUPBY` on a numeric field still runs its key-text conversion once per
  record instead of once per group. Cheaper per call now, but still the
  wrong number of calls — three fixes were considered (typed key, cheap
  `to_chars` swap, sort-based grouping) but none were implemented.
- `get()`'s field lookup is a linear scan over an object's keys —
  `O(object width)`, not `O(1)`. Fine at the object widths in our test/
  benchmark data; would need revisiting for very wide objects.
- Output only goes to `stdout` — no flag to write a query's result to a
  file.
- `FILTER(AVERAGE(...) > x)` only works one level of nesting deep — against
  a flat array of plain records it silently matches nothing rather than
  erroring.
- Test suite gaps: no test exercises Filter by Average (only the
  standalone `AVERAGE` stage is covered); the parallel `FILTER`/`GROUPBY`/
  `AVERAGE` reductions were checked by hand against a synthetic large input
  rather than by a permanent, automated test at that scale; no fuzz/
  property-based testing.

## Final Submission Goals

- The main goal is to finish up the benchmarking to account for bigger and bigger files, compare across the weeks to see how individual optimizations helped our project overtime, and create video submission


## Lab Session Check-In (Monday August 24th, 2026)

- Went over feedback given after the demo to try and analyze what exactly we should work on
- Going to implement more and more benchmarks to measure 
