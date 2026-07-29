# JSON/JSONL Parser
## Team

| Javier Herrera Jr 
| Jules
| Ryan

---

## Lab Session Check-In (Monday, July 27)

**What option did you choose?**

Our group chose option 2, the JSON analysis

**What exactly will your software do?**

The software will try to parse through any JSON files while using the least RAM possible so that it's versitile for any file and any computer. The plan is to use parallels to try and accomplish this

**What language(s) will you use?**

We will only use C++

**What do you hope to accomplish in the remaining 2 hours?**

    - Lay out the 4 sprints
    - Describe feature sets
    - Work on sprint 1 which is a basic JSON parser with no performance

## JSON Examples

    - Sort
        Query: SORT(a, “asc”)

        JSON: [{"a": 1, "b": 2}, {"a": 0, "b": 3}]

        The answer should be [{"a": 0, "b": 3}, {"a": 1, "b": 2}]
    - Limit
        Query: LIMIT(1)

        JSON: [{"a": 1, "b": 2}, {"a": 0, "b": 3}]

        The answer should be {"a": 1, "b": 2}
    - Get
        Query: .a

        JSON: {“a”: 1, “b”: 2}

        The answer should be 1
    - GroupBy
        Query: GROUPBY(.a)

        JSON: [{"a": 1, "b": 2}, {"a": 0, "b": 3}, {"a": 1, "b": 5}]

        The answer should be: {
 	    "0": [{"a": 0, "b": 3}],
 	    "1": [{"a": 1, "b": 2}, {"a": 1, "b": 5}]
        }
    - Filter & Comparison
        Query: FILTER(.a > 1)

	    JSON: [{"a": 1, "b": 2}, {"a": 2, "b": 2}]

	    The answer should be [{"a": 2, "b": 2}]
    - Average
        Query: GROUPBY(.city) | AVERAGE(.price)

        JSON: [{"id": 1, "city": "Riverside", "price": 450000}, {"id": 2, "city": "Riverside", "price": 480000}, {"id": 3, "city": "Los Angeles", "price": 800000}, {"id": 4, "city": "Riverside", "price": 460000}]

        The answer should be {"Riverside": 463333.33, "Los Angeles": 800000}

## Assignments

### Person 1 (Jules) — JSON parser and data model

Own the actual conversion from JSON text into an internal C++ structure.

**Responsibilities**

- Define a `JsonValue` type supporting:
    - null
    - boolean
    - number
    - string
    - object
    - array
- Parse basic JSON values.
- Parse nested objects and arrays.
- Handle whitespace, escaped strings, commas, braces, and brackets.
- Return understandable errors for malformed JSON.

**Example milestone:**

    JsonValue value = parseJson(
        R"({"student":{"name":"Ryan","scores":[90,85]}})"
    );

This is probably the most technically involved Sprint 1 assignment.

### Person 2 (Javier) — JSONL reader and lookup engine

Own everything that happens after one JSON object can be parsed.

**Responsibilities**

- Read a `.jsonl` file one line at a time.
- Send each line into Person 1's parser.
- Store or return the parsed records.
- Implement simple field lookup:
    - `.name`
    - `.student.name`
    - `.scores[0]`
- Decide what happens when:
    - A field does not exist.
    - An array index is invalid.
    - One JSONL line is malformed.
- Format lookup results for terminal output.

**Example:**

    Query: .student.name
    Result: "Ryan"

This gives you the beginning of the query engine without prematurely building the full Sprint 2 language.

### Person 3 (Ryan) — CLI, testing, and integration

Own the executable that connects everyone's work and proves it works.

**Responsibilities**

- Build the command-line interface.
- Accept:
    - Input filename.
    - Lookup expression.
- Create unit tests for:
    - Primitive values.
    - Nested objects.
    - Nested arrays.
    - Mixed arrays and objects.
    - Invalid JSON.
    - Empty files.
    - Multiple JSONL records.
    - Missing lookup fields.
- Prepare small test datasets.
- Set up CMake and the shared project structure.
- Integrate Person 1 and Person 2's branches.
- Document how to build and run the program.

**Example command:**

    ./sluice students.jsonl ".student.name"

**Example output:**

    Ryan
    Javier
    Jules

## Demo:

- For this sprint, we plan to have a demon that is able to read in JSON/JSONL files, read or parse through them, and lookup simple fields. We will also be implementing CLI, testing, and integration

    - Example:
        - Query: .student.name
        - Result: "Ryan"