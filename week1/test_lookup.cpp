// Standalone test driver for lookup()/formatResult(), now running against
// Jules's real Parser instead of a hand-built tree.
//
// Build:
//   g++ -std=c++17 -Wall -Wextra scanner.cpp parser.cpp file-reader.cpp test_lookup.cpp -o test_lookup
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
