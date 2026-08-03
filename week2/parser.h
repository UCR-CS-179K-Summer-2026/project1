// This file is meant to define the Parser class, which is responsible for parsing a JSON or JSONL string and producing a JSONValue.
// It uses the Scanner class to tokenize the input string, and implements a recursive descent parser to build the JSONValue.
#pragma once

#include "scanner.h"
#include "value.h"

#include <iostream>

using namespace std;

class Parser {
    private:
    Scanner scanner;
    Token currToken;
    ParserType fileType;

    public:
    Parser(string_view json, ParserType type ) : scanner(json, type), currToken(scanner.scan()), fileType(type) {}

    JSONValue parse();
};