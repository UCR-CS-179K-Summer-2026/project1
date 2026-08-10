// The sluice command line tool. Reads a JSON/JSONL file and runs a lookup
// on every record in it.
//
// Build:
//   g++ -std=c++20 -Wall -Wextra scanner.cpp parser.cpp query-parser.cpp file-reader.cpp main.cpp -o sluice
// Run:
//   ./sluice students.jsonl ".student.name"
#include "file-reader.h"
#include "session.h"

#include <algorithm>
#include <iostream>
#include <memory>

using namespace std;

bool doUpload(Session& session);
bool doSearch(Session& session);
void runInteractive();
void printUsage();
void printBanner();
void printBoxBorder(const string& left, const string& right);
void printBoxLine(const string& text);
void printMenu();
string trim(const string& text);
string toLower(const string& text);
string normalizeCommand(const string& input);
bool promptLine(const string& label, string& line);

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

    runInteractive();

    return 0;
}

bool doUpload(Session& session) {
    string path;
    while (true) {
        if (!promptLine("Enter path to your file: ", path)) {
            return false;
        }
        if (path.empty()) {
            return true;
        }

        uploadFile(path, session);

        return true;
    }
}

bool doSearch(Session& session) {
    if (!session.isInitialized) {
        cout << "Please upload a file first\n";
        printMenu();
        return true;
    }

    string query;
    if (!promptLine("Enter your search query: ", query)) {
        return false;
    }
    if (query.empty()) {
        return true;
    }

    string result = excecuteQuery(session, query);
    cout << result << endl;

    return true;
}

void runInteractive() {
    printBanner();
    printMenu();

    Session session;
    string input;

    while (true) {
        //string prompt = session.loaded ? "[" + session.name + "] > " : "> ";
        string prompt = "> ";
        if (!promptLine(prompt, input)) {
            break;
        }

        string command = normalizeCommand(input);
        if (command.empty()) {
            continue;
        }

        if (command == "q" || command == "quit") {
            break;
        } else if (command == "m" || command == "menu") {
            printMenu();
        } else if (command == "u" || command == "upload") {
            if (!doUpload(session)) {
                break;
            }
        } else if (command == "s" || command == "search") {
            if (!doSearch(session)) {
                break;
            }
        } else {
            cout << "Unknown command. Enter \\m to view the menu.\n";
        }
    }

    cout << "Thanks for choosing our program!\n";
}

const int BOX_WIDTH = 37;

void printUsage() {
    cout << "usage: sluice <file.json|file.jsonl> \"<lookup expression>\"\n"
         << "       sluice students.jsonl \".student.name\"\n\n"
         << "run sluice with no arguments to open the menu\n\n"
         << "lookup expressions look like .name, .student.name, or .scores[0]\n";
}

void printBanner() {
    cout << "\n"
         << "        W E L C O M E   T O\n"
         << "     ██ ███████  ██████  ███    ██\n"
         << "     ██ ██      ██    ██ ████   ██\n"
         << "     ██ ███████ ██    ██ ██ ██  ██\n"
         << "██   ██      ██ ██    ██ ██  ██ ██\n"
         << " █████  ███████  ██████  ██   ████\n"
         << "\n"
         << "     by Javier, Jules and Ryan\n";
}

void printBoxBorder(const string& left, const string& right) {
    cout << left;
    for (int i = 0; i < BOX_WIDTH; i++) {
        cout << "─";
    }
    cout << right << "\n";
}

void printBoxLine(const string& text) {
    cout << "│" << text;
    for (int i = static_cast<int>(text.size()); i < BOX_WIDTH; i++) {
        cout << " ";
    }
    cout << "│\n";
}

void printMenu() {
    cout << "\n";
    printBoxBorder("┌", "┐");
    printBoxLine("  MENU");
    printBoxLine("    \\u   upload a .json/.jsonl file");
    printBoxLine("    \\s   search & query your file");
    printBoxLine("    \\m   view this menu");
    printBoxLine("    \\q   quit");
    printBoxBorder("└", "┘");
    cout << "\n";
}

string trim(const string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == string::npos) {
        return "";
    }
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

string toLower(const string& text) {
    string lowered = text;
    transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return tolower(c);
    });
    return lowered;
}

string normalizeCommand(const string& input) {
    string command = toLower(input);
    if (!command.empty() && command[0] == '\\') {
        command = command.substr(1);
    }
    return command;
}

bool promptLine(const string& label, string& line) {
    cout << label;
    if (!getline(cin, line)) {
        cout << "\n";
        return false;
    }
    line = trim(line);
    return true;
}