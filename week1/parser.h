#include "scanner.h"
#include "value.cpp"

class Parser {
    private:
    Scanner scanner;
    Token currToken;

    public:
    Parser(std::string_view json) : scanner(json), currToken(scanner.scan()) {}

    JSONValue parse();
};