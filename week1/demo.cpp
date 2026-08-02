// Demo entry point matching the "Demo" section in PLAN.md: read a .jsonl
// file, run a lookup query against every record, print the results.
//
// Build:
//   g++ -std=c++17 -Wall -Wextra scanner.cpp parser.cpp file-reader.cpp demo.cpp -o demo
// Run:
//   ./demo
#include <iostream>

#include "file-reader.h"

int main() {
    string path = "students.json";
    string query = ".student[4].scores";

    cout << "Reading " << path << ", query: " << query << "\n\n";

    auto records = readFile(path);
    for (const auto& record : records) {
        LookupResult result = lookup(record, query);
        cout << formatResult(result) << "\n";
    }

    return 0;
}
