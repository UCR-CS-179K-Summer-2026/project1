# Sprint 1 Plan

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

## Assignments

### Person 1 (Jules) — JSON parser and data model

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

### Person 2 (Javier) — JSONL reader and lookup engine

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

### Person 3 (Ryan) — CLI, testing, and integration

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

## Lab Session Check-In (Wednesday, July 29th)

**What we had done before coming into lab:**

- Sample parser from Jules that was untested
- Skeleton code for parser reader and lookup that wasn't tested and only accounted for edge cases

**What needs to be done by 4:00 PM**

- Have a working demo to present about the project
    - This includes a parser that can parse through a JSON file, find a field, and output it accordingly
- Talk very briefly about the structure and overall plan of attack for this weekend to prepare for the demo on Monday during discussion

**What's left to do after lab**

- Clean up the parser to accept all types of JSON files and read in properly
    - At the time of demo, we only tested a simple 3 line JSON file with our names and scores
- Work on final README for the project and GitHub Pages Website for presentation on Monday
- Clean up polymorphism structure more for the JSON reader
- Get an actual tester and CLI functioning for demo
- Everyone has the same roles as Monday, just working on the final tweaks for Monday demo