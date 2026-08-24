// The Streamline command line tool. Reads a JSON/JSONL file and runs a
// lookup on every record in it. See the README for how to build and run it.
#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <utility>

#ifdef STREAMLINE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

using namespace std;

#include "query-engine.h"
#include "session.h"
#include "version.h"

bool doUpload(Session& session);
bool doSearch(Session& session);
void runInteractive(Session &session);
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
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif

    Session session;
    vector<string> args;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "--version") {
            cout << "streamline " << getVersionId() << "\n";
            return 0;
        } else {
            args.push_back(arg);
        }
    }

    if (args.empty()) {
        runInteractive(session);
        return 0;
    }

    if (args.size() != 2) {
        cout << "streamline: need a file and a query\n\n";
        printUsage();
        return 1;
    }
    
    try {
        uploadFile(args[0], session);
        cout << executeQuery(session, args[1]) << "\n";
    } catch (const exception& e) {
        cout << "streamline: " << e.what() << "\n";
        return 1;
    }
    
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

        if(path == "q" || path == "quit") {
            printMenu();
            return true;
        }

        try {
            uploadFile(path, session);
        } catch (const exception& e) {
            cout << "streamline: " << e.what() << "\n";
            continue;
        }

        cout << "File " << session.name << " uploaded successfully. ("
             << session.records
             << (session.records == 1 ? " record)\n" : " records)\n");
        return true;
    }
}

bool doSearch(Session& session) {
    if (!session.isInitialized) {
        cout << "Please upload a file first\n";
        printMenu();
        return true;
    }

    cout << endl << "Querying " << session.name << ". Enter q to exit query mode." << endl;
    while (true) {
        string query;
        if (!promptLine("Enter your search query: ", query)) {
            return false;
        }

        string command = normalizeCommand(query);
        if(command == "u" || command == "upload") {
            doUpload(session);
        } else if (command == "q" || command == "quit") {
            printMenu();
            return true;
        } else if (query.empty()) {
            continue;
        }

        try {
            cout << executeQuery(session, query) << "\n";
        } catch (const exception& e) {
            cout << "streamline: " << e.what() << "\n";
        }
    }
}

void runInteractive(Session& session) {
    printBanner();
    printMenu();

    string input;

    while (true) {
        string prompt = session.isInitialized ? "[" + session.name + "] > " : "> ";
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
            if (session.isInitialized && !doSearch(session)) {
                break;
            }
        } else if (command == "s" || command == "search") {
            if (!doSearch(session)) {
                break;
            }
        } else {
            cout << "Unknown command. Enter m to view the menu.\n";
        }
    }

    cout << "Thanks for choosing our program!\n";
}

const int BOX_WIDTH = 37;

void printUsage() {
    cout << "usage: streamline <file.json|file.jsonl> \"<query>\"\n"
         << "       streamline students.json \"FILTER(GET(\\\"gpa\\\") > 3.5) | LIMIT(5)\"\n\n"
         << "       streamline --version\n\n"
         << "run streamline with no arguments to open the menu\n";
}

void printBanner() {
    cout << "\n"
         << "        W E L C O M E   T O\n"
         << "▄▀▀▀▀ ▀▀█▀▀ █▀▀▀▄ █▀▀▀▀ ▄▀▀▀▄ █▄ ▄█ █     ▀█▀ █▄  █ █▀▀▀▀\n"
         << "▀▄▄▄    █   █▄▄▄▀ █▄▄▄  █   █ █ █ █ █      █  █ █ █ █▄▄▄ \n"
         << "    █   █   █ ▀▄  █     █▀▀▀█ █   █ █      █  █  ▀█ █    \n"
         << "▀▄▄▄▀   █   █  ▀▄ █▄▄▄▄ █   █ █   █ █▄▄▄▄ ▄█▄ █   █ █▄▄▄▄\n"
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
    printBoxLine("    u   upload a .json/.jsonl file");
    printBoxLine("    s   search & query your file");
    printBoxLine("    m   view this menu");
    printBoxLine("    q   quit");
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
#ifdef STREAMLINE_READLINE
    char* input = readline(label.c_str());
    if (!input) {
        cout << "\n";
        return false;
    }
    line = input;
    if (!line.empty()) {
        add_history(input);
    }
    free(input);
#else
    cout << label;
    if (!getline(cin, line)) {
        cout << "\n";
        return false;
    }
#endif
    line = trim(line);
    return true;
}
