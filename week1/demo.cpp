// Demo entry point matching the "Demo" section in PLAN.md: read a .jsonl
// file, run a lookup query against every record, print the results.
//
// Build:
//   g++ -std=c++17 -Wall -Wextra scanner.cpp parser.cpp jsonl-reader.cpp demo.cpp -o demo
// Run:
//   ./demo
#include <iostream>

#include "jsonl-reader.h"

int main() {
    std::string path = "students.jsonl";
    std::string query = ".student.name";

    std::cout << "Reading " << path << ", query: " << query << "\n\n";

    auto records = readJsonlFile(path);
    for (const auto& record : records) {
        LookupResult result = lookup(record, query);
        std::cout << formatResult(result) << "\n";
    }

    return 0;
}
