// This file is meant to test the lookup function, which takes a JSONValue and a path string, and returns a LookupResult that contains either the value at that path or an error message.
// This is temporary until Ryan's demo.cpp is ready. The test cases here are similar to the ones in demo.cpp, but they are hardcoded for testing purposes.
//
// Build & run (see the repo README / design page's Build System section):
//   cmake -B build && cmake --build build
//   ./build/test_lookup
#include <iostream>

#include "file-reader.h"

int main() {
    Parser parser(R"({"student":{"name":"Ryan","scores":[90,85]}})", ParserType::JSON);
    JSONValue root = parser.parse();

    struct Case { string path; };
    vector<Case> cases = {
        {".student.name"},      // expect "Ryan"
        {".student.scores[0]"}, // expect 90
        {".student.missing"},   // expect a field-not-found error
        {".student.scores[9]"}, // expect an out-of-range error
    };

    for (const auto& c : cases) {
        LookupResult result = lookup(root, c.path);
        cout << c.path << " -> " << formatResult(result) << "\n";
    }

    return 0;
}
