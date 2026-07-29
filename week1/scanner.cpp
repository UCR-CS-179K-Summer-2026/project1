#include "scanner.h"

Token Scanner::scan() {
    while(curr < end && (*curr == ' ' || *curr == '\n' || *curr == '\t' || *curr == '\r')) {
        ++curr;
    }

    if(curr >= end) {
        return Token(TokenType::End, {});
    }

    if(*curr == '{') {
        curr++;
        return Token(TokenType::LBrace, "{");
    }
    if(*curr == '}') {
        curr++;
        return Token(TokenType::RBrace, "}");
    }
    if(*curr == '[') {
        curr++;
        return Token(TokenType::LBracket, "[");
    }
    if(*curr == ']') {
        curr++;
        return Token(TokenType::RBracket, "]");
    }
    if(*curr == ':') {
        curr++;
        return Token(TokenType::Colon, ":");
    }
    if(*curr == ',') {
        curr++;
        return Token(TokenType::Comma, ",");
    }

    if(*curr == '\"') {
        curr++;
        const char* stringBegin = curr;

        while(curr < end && *curr != '\"') {
            if(*curr == '\\' && curr + 1 < end) {
                curr += 2;
            } else {
                curr++;
            }
        }

        if(curr >= end) {
            throw runtime_error("Unterminated string");
        }

        string_view str(stringBegin, curr - stringBegin);
        curr++;  // skip closing quote
        return Token(TokenType::String, str);
    }

    if(*curr == 't' && curr + 3 < end && *(curr + 1) == 'r' && *(curr + 2) == 'u' && *(curr + 3) == 'e') {
        curr += 4;
        return Token(TokenType::Boolean, "true");
    }

    if(*curr == 'f' && curr + 4 < end && *(curr + 1) == 'a' && *(curr + 2) == 'l' && *(curr + 3) == 's' && *(curr + 4) == 'e') {
        curr += 5;
        return Token(TokenType::Boolean, "false");
    }

    if(*curr == 'n' && curr + 3 < end && *(curr + 1) == 'u' && *(curr + 2) == 'l' && *(curr + 3) == 'l') {
        curr += 4;
        return Token(TokenType::Null, "null");
    }

    if((*curr >= '0' && *curr <= '9') || *curr == '-') {
        const char* numberBegin = curr;

        while(curr < end && *curr != ',' && *curr != ']' && *curr != '}' && *curr != ':') {
            curr++;
        }

        regex pattern(R"(-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)");
        string str(numberBegin, curr - numberBegin);

        if(regex_match(str, pattern)) {
            string_view num(numberBegin, curr - numberBegin);
            return Token(TokenType::Number, num);
        }
    }

    throw runtime_error("Invalid JSON input");
}