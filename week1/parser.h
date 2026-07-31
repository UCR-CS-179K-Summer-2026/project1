#pragma once

#include "scanner.h"
#include "value.cpp"

#include <iostream>

using namespace std;

class Parser {
    private:
    Scanner scanner;
    Token currToken;

    public:
    Parser(std::string_view json) : scanner(json), currToken(scanner.scan()) {}

    JSONValue parse();
};