// This file is meant to define the FileParser class, which is responsible for parsing a JSON or JSONL string and producing a JSONValue.
// It uses the FileScanner class to tokenize the input string, and implements a recursive descent parser to build the JSONValue.
#pragma once

#include "file-scanner.h"
#include "value.h"

#include <charconv>
#include <iostream>
#include <string>

using namespace std;

class FileParser {
    private:
    FileScanner scanner;
    Token currToken;
    ParserType fileType;

    public:
    FileParser(string_view json, ParserType type ) : scanner(json, type), currToken(scanner.scan()), fileType(type) {}

    JSONValue parse();
    JSONValue parseObject();

};