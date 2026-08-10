// This file is meant to implement the Scanner class, which is responsible for scanning a JSON or JSONL string and producing a sequence of tokens.
// It also implements the scan() method, which returns the next token in the input string.
#include "scanner.h"

Token Scanner::scan() {
    while(curr < end && (*curr == ' ' || *curr == '\n' || *curr == '\t' || *curr == '\r')) {
        if(*curr == '\n') {
            ++currLine;
        }

        ++curr;
    }

    if(curr >= end) {
        return Token(JSONTokenType::End, {});
    }

    if(*curr == '{') {
        curr++;
        return Token(JSONTokenType::LBrace, "{");
    }
    if(*curr == '}') {
        curr++;
        return Token(JSONTokenType::RBrace, "}");
    }
    if(*curr == '[') {
        curr++;
        return Token(JSONTokenType::LBracket, "[");
    }
    if(*curr == ']') {
        curr++;
        return Token(JSONTokenType::RBracket, "]");
    }
    if(*curr == ':') {
        curr++;
        return Token(JSONTokenType::Colon, ":");
    }
    if(*curr == ',') {
        curr++;
        return Token(JSONTokenType::Comma, ",");
    }

    if(*curr == '\"') {
        curr++;
        const char* stringBegin = curr;

        while(curr < end && *curr != '\"') {
            if(*curr == '\\' && curr + 1 < end) {
                if(*(curr + 1) == 'u' && curr + 5 < end) {
                    curr += 6;  // skip \uXXXX
                } else if (*(curr + 1) == '\"' || *(curr + 1) == '\\' || *(curr + 1) == '/' ||
                           *(curr + 1) == 'b' || *(curr + 1) == 'f' || *(curr + 1) == 'n' ||
                           *(curr + 1) == 'r' || *(curr + 1) == 't') {
                    curr += 2;  // skip escaped character
                } else {
                    if(fileType == ParserType::JSON) {
                        throw runtime_error("Illegal escape sequence in string at line " + to_string(currLine));
                    } else {
                        throw runtime_error("Illegal escape sequence in string");
                    }
                }

            } else if(static_cast<unsigned char>(*curr) < 0x20) {
                if(fileType == ParserType::JSON) {
                    throw runtime_error("Illegal unescaped control character in string at line " + to_string(currLine));
                } else {
                    throw runtime_error("Illegal unescaped control character in string");
                }
            } else {
                curr++;
            }
        }

        if(curr >= end) {
            throw runtime_error("Unterminated string");
        }

        string_view str(stringBegin, curr - stringBegin);
        curr++;  // skip closing quote
        return Token(JSONTokenType::String, str);
    }

    if(*curr == 't' && curr + 3 < end && *(curr + 1) == 'r' && *(curr + 2) == 'u' && *(curr + 3) == 'e') {
        curr += 4;
        return Token(JSONTokenType::Boolean, "true");
    }

    if(*curr == 'f' && curr + 4 < end && *(curr + 1) == 'a' && *(curr + 2) == 'l' && *(curr + 3) == 's' && *(curr + 4) == 'e') {
        curr += 5;
        return Token(JSONTokenType::Boolean, "false");
    }

    if(*curr == 'n' && curr + 3 < end && *(curr + 1) == 'u' && *(curr + 2) == 'l' && *(curr + 3) == 'l') {
        curr += 4;
        return Token(JSONTokenType::Null, "null");
    }

    if((*curr >= '0' && *curr <= '9') || *curr == '-') {
        const char* numberBegin = curr;

        while(curr < end && *curr != ',' && *curr != ']' && *curr != '}' && *curr != ':'
                && *curr != ' ' && *curr != '\t' && *curr != '\n' && *curr != '\r') {
            curr++;
        }

        static const regex pattern(R"(-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)");
        string str(numberBegin, curr - numberBegin);

        if(regex_match(str, pattern)) {
            string_view num(numberBegin, curr - numberBegin);
            return Token(JSONTokenType::Number, num);
        }
    }

    if(fileType == ParserType::JSON) {
        throw runtime_error("Invalid JSON input at line " + to_string(currLine));
    } else {
        throw runtime_error("Invalid JSONL input");
    }
}