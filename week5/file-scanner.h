// This file is meant to define the FileScanner class, which is responsible for scanning a JSON or JSONL string and producing a sequence of tokens.
// It also defines the JSONTokenType enum to represent the type of a token, and the Token struct to hold a token's type and value.
#pragma once

#include <cstdint>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace std;

// Named JSONTokenType (not TokenType) because <windows.h> declares an
// unscoped enum whose first enumerator is literally `TokenType` (from
// TOKEN_INFORMATION_CLASS in winnt.h), which collides with that name in the
// global namespace once windows.h is included anywhere in the same TU.
enum class JSONTokenType : uint8_t {Boolean, Colon, Comma, End, LBrace, LBracket, Null, Number, ObjEnd, RBrace, RBracket, String};
enum class ParserType {JSONL, JSON};

struct Token {
    JSONTokenType type;
    string_view value;

    Token(JSONTokenType t, string_view v) : type(t), value(v) {}
};

static const unordered_map<string_view, JSONTokenType> keywordTable = {
    {"true", JSONTokenType::Boolean},
    {"false", JSONTokenType::Boolean},
    {"null", JSONTokenType::Null}
};

class FileScanner {
    private:
    const char* curr;
    const char* end;
    int currLine = 1;
    ParserType fileType;

    public:
    FileScanner(string_view json, ParserType type) : curr(json.data()), end(json.data() + json.size()), fileType(type) {}
    
    int getLineNumber() const { return currLine; }
    Token scan();
};