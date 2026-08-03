// This file is meant to define the Scanner class, which is responsible for scanning a JSON or JSONL string and producing a sequence of tokens.
// It also defines the TokenType enum to represent the type of a token, and the Token struct to hold a token's type and value.
#pragma once

#include <cstdint>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>

using namespace std;

enum class TokenType : uint8_t {Boolean, Colon, Comma, End, LBrace, LBracket, Null, Number, RBrace, RBracket, String};
enum class ParserType {JSONL, JSON};

struct Token {
    TokenType type;
    string_view value;

    Token(TokenType t, string_view v) : type(t), value(v) {}
};

class Scanner {
    private:
    const char* curr;
    const char* end;
    int currLine = 1;
    ParserType fileType;

    public:
    Scanner(string_view json, ParserType type) : curr(json.data()), end(json.data() + json.size()), fileType(type) {}
    
    int getLineNumber() const { return currLine; }
    Token scan();
};