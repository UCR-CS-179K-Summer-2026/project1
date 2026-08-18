// This file is meant to implement the Scanner class, which is responsible for scanning a JSON or JSONL string and producing a sequence of tokens.
// It also implements the scan() method, which returns the next token in the input string.
#include "file-scanner.h"

Token FileScanner::scan() {
    while(curr < end && (*curr == ' ' || *curr == '\n' || *curr == '\t' || *curr == '\r')) {
        if(*curr == '\n') {
            ++currLine;
            ++curr;

            if(fileType == ParserType::JSONL) {
                return Token(JSONTokenType::ObjEnd, {});
            }
        } else {
            ++curr;
        }
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
                    throw runtime_error("Illegal escape sequence in string at line " + to_string(currLine));
                }

            } else if(static_cast<unsigned char>(*curr) < 0x20) {
                throw runtime_error("Illegal unescaped control character in string at line " + to_string(currLine));
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
        static const regex numberRegex(R"(-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)");

        cmatch match;
        if(regex_search(curr, end, match, numberRegex, regex_constants::match_continuous)) {
            string_view num(curr, match.length());
            curr += match.length();
            return Token(JSONTokenType::Number, num);
        } 

        /*const char* numberBegin = curr;

        while(curr < end && *curr != ',' && *curr != ']' && *curr != '}' && *curr != ':'
                && *curr != ' ' && *curr != '\t' && *curr != '\n' && *curr != '\r') {
            curr++;
        }

        const char* numberEnd = curr;
        const char* p = numberBegin;
        bool valid = true;

        if(p < numberEnd && *p == '-') p++;

        if(p < numberEnd && *p == '0') {
            p++;
        } else if(p < numberEnd && *p >= '1' && *p <= '9') {
            p++;
            while(p < numberEnd && *p >= '0' && *p <= '9') p++;
        } else {
            valid = false;
        }

        if(valid && p < numberEnd && *p == '.') {
            p++;
            if(p >= numberEnd || *p < '0' || *p > '9') {
                valid = false;
            } else {
                while(p < numberEnd && *p >= '0' && *p <= '9') p++;
            }
        }

        if(valid && p < numberEnd && (*p == 'e' || *p == 'E')) {
            p++;
            if(p < numberEnd && (*p == '+' || *p == '-')) p++;
            if(p >= numberEnd || *p < '0' || *p > '9') {
                valid = false;
            } else {
                while(p < numberEnd && *p >= '0' && *p <= '9') p++;
            }
        }

        if(valid && p == numberEnd) {
            string_view num(numberBegin, numberEnd - numberBegin);
            return Token(JSONTokenType::Number, num);
        }*/
    }

    if(fileType == ParserType::JSON) {
        throw runtime_error("Invalid JSON input at line " + to_string(currLine));
    } else {
        throw runtime_error("Invalid JSONL input at line " + to_string(currLine));
    }
}