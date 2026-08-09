#include <iostream>
#include <string>

#include "scanner.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(bool condition, const string& name) {
    if (condition) {
        passed++;
    } else {
        failed++;
        cout << "FAIL: " << name << "\n";
    }
}

void checkToken(Scanner& scanner, TokenType type, const string& value, const string& name) {
    Token token = scanner.scan();
    check(token.type == type && token.value == value, name);
}

void checkThrows(const string& input, const string& name) {
    bool threw = false;

    try {
        Scanner scanner(input, ParserType::JSON);
        scanner.scan();
    } catch (const exception&) {
        threw = true;
    }

    check(threw, name);
}

void testTokens() {
    string input = R"({}[]:,"name" true false null -12.5)";
    Scanner scanner(input, ParserType::JSON);

    checkToken(scanner, TokenType::LBrace, "{", "left brace");
    checkToken(scanner, TokenType::RBrace, "}", "right brace");
    checkToken(scanner, TokenType::LBracket, "[", "left bracket");
    checkToken(scanner, TokenType::RBracket, "]", "right bracket");
    checkToken(scanner, TokenType::Colon, ":", "colon");
    checkToken(scanner, TokenType::Comma, ",", "comma");
    checkToken(scanner, TokenType::String, "name", "string");
    checkToken(scanner, TokenType::Boolean, "true", "true");
    checkToken(scanner, TokenType::Boolean, "false", "false");
    checkToken(scanner, TokenType::Null, "null", "null");
    checkToken(scanner, TokenType::Number, "-12.5", "number");
    checkToken(scanner, TokenType::End, "", "end");
}

void testWhitespace() {
    string input = "\n\n true";
    Scanner scanner(input, ParserType::JSON);

    checkToken(scanner, TokenType::Boolean, "true", "whitespace");
    check(scanner.getLineNumber() == 3, "line number");
}

void testErrors() {
    checkThrows("@", "invalid character");
    checkThrows("\"hello", "unterminated string");
}

int main() {
    testTokens();
    testWhitespace();
    testErrors();

    cout << "passed: " << passed << "\n";
    cout << "failed: " << failed << "\n";

    return failed == 0 ? 0 : 1;
}
