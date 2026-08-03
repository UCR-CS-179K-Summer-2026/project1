// The sluice command line tool. Reads a JSON/JSONL file and runs a lookup
// on every record in it.
//
// Build:
//   g++ -std=c++17 -Wall -Wextra scanner.cpp parser.cpp file-reader.cpp main.cpp -o sluice
// Run:
//   ./sluice students.jsonl ".student.name"
#include <iostream>

#include "file-reader.h"

void printUsage() {
    cout << "usage: sluice <file.json|file.jsonl> \"<lookup expression>\"\n"
         << "       sluice students.jsonl \".student.name\"\n\n"
         << "lookup expressions look like .name, .student.name, or .scores[0]\n";
}

int main(int argc, char** argv) {
    vector<string> args;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--help") {
            printUsage();
            return 0;
        } else {
            args.push_back(arg);
        }
    }

    if (args.size() != 2) {
        cout << "sluice: need a file and a lookup expression\n\n";
        printUsage();
        return 1;
    }

    string path = args[0];
    string query = args[1];

    vector<JSONValue> records;
    try {
        records = readFile(path);
    } catch (const exception& e) {
        cout << "sluice: " << e.what() << "\n";
        return 1;
    }

    if (records.empty()) {
        cout << "sluice: no records in " << path << "\n";
        return 1;
    }

    for (const auto& record : records) {
        LookupResult result = lookup(record, query);

        if (result.ok && result.value->getType() == ValueType::String) {
            cout << get<string>(result.value->getValue()) << "\n";
        } else {
            cout << formatResult(result) << "\n";
        }
    }

    return 0;
}
