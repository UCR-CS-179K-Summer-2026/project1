// Standalone test driver for lookup()/formatResult(), built by hand instead
// of via parseJson() — so you can develop and test your part without
// waiting on Person 1's parser to exist. Swap in real parseJson() calls
// later, or leave this alongside real tests once Person 3 sets those up.
//
// Build (once Person 1's json_value.cpp exists, this needs it too — for now
// json_value.h is header-only, so this is enough):
//   g++ -std=c++17 -Wall -Wextra jsonl-reader.cpp test_lookup.cpp -o test_lookup
#include <iostream>

#include "jsonl-reader.h"
#include "json_value.h"

int main() {
    // Builds: {"student": {"name": "Ryan", "scores": [90, 85]}}
    JsonValue::Object scoresHolder;
    JsonValue student = JsonValue::Object{
        {"name", JsonValue("Ryan")},
        {"scores", JsonValue::Array{JsonValue(90.0), JsonValue(85.0)}},
    };
    JsonValue root = JsonValue::Object{{"student", student}};

    struct Case { std::string path; };
    std::vector<Case> cases = {
        {".student.name"},   // expect "Ryan"
        {".student.scores[0]"}, // expect 90
        {".student.missing"},   // expect an error
    };

    for (const auto& c : cases) {
        LookupResult result = lookup(root, c.path);
        std::cout << c.path << " -> " << formatResult(result) << "\n";
    }

    return 0;
}
