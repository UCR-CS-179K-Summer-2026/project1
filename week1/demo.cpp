// Demo entry point matching the "Demo" section in PLAN.md: read a .jsonl
// file, run a lookup query against every record, print the results.
//
// Build & run (see the repo README / design page's Build System section):
//   cmake -B build && cmake --build build
//   ./build/demo
#include <iostream>

#include "file-reader.h"

int main() {
    string path = "students.json";
    string query = ".student[2].scores";

    cout << "Reading " << path << ", query: " << query << "\n\n";

    try {
        auto records = readFile(path);
        for (const auto& record : records) {
            LookupResult result = lookup(record, query);
            cout << formatResult(result) << "\n";
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
