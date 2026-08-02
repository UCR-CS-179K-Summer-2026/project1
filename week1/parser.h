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