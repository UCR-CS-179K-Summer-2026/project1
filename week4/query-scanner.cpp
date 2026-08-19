#include "query-scanner.h"

QueryToken QueryScanner::scan() {
    while(curr < end && (*curr == ' ' || *curr == '\n' || *curr == '\t' || *curr == '\r')) {
        curr++;
    }

    if(curr >= end) {
        return QueryToken(QueryTokenType::End, {});
    }

    switch(*curr) {
        case '(':
            curr++;
            return QueryToken(QueryTokenType::LParen, "(");
        case ')':
            curr++;
            return QueryToken(QueryTokenType::RParen, ")");
        case ',':
            curr++;
            return QueryToken(QueryTokenType::Comma, ",");
        case '|':
            curr++;
            return QueryToken(QueryTokenType::Pipe, "|");

        case '=':
            if(curr + 1 < end && *(curr + 1) == '=') {
                curr += 2;
                return QueryToken(QueryTokenType::Eq, "==");
            }
            break;
        case '!':
            if(curr + 1 < end && *(curr + 1) == '=') {
                curr += 2;
                return QueryToken(QueryTokenType::Neq, "!=");
            }
            break;
        case '<':
            if(curr + 1 < end && *(curr + 1) == '=') {
                curr += 2;
                return QueryToken(QueryTokenType::Leq, "<=");
            }
            curr++;
            return QueryToken(QueryTokenType::Lt, "<");
        
        case '>':
            if(curr + 1 < end && *(curr + 1) == '=') {
                curr += 2;
                return QueryToken(QueryTokenType::Geq, ">=");
            }
            curr++;
            return QueryToken(QueryTokenType::Gt, ">");

        case '\"': {
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
                        throw runtime_error("Query contains illegal escape sequence in string");
                    }

                } else if(static_cast<unsigned char>(*curr) < 0x20) {
                    throw runtime_error("Query contains illegal escape sequence in string");
                } else {
                    curr++;
                }
            }

            if(curr >= end) {
                throw runtime_error("Unterminated string");
            }

            string_view str(stringBegin, curr - stringBegin);
            curr++;  // skip closing quote
            return QueryToken(QueryTokenType::String, str);
        }

        default:
            break;
    }

    if( (*curr >= 'A' && *curr <= 'Z') || (*curr >= 'a' && *curr <= 'z')) {
        const char* identStart = curr;

        while(curr < end && *curr != ',' && *curr != ')' && *curr != '\"' && *curr != '(' && *curr != '|' && *curr != '=' && *curr != '!' && *curr != '<' && *curr != '>'
                && *curr != ' ' && *curr != '\t' && *curr != '\n' && *curr != '\r') {
            curr++;
        }

        string_view ident(identStart, curr - identStart);

        auto it = queryKeywordTable.find(ident);
        if (it != queryKeywordTable.end()) {
            return QueryToken(it->second, ident);
        }
        throw runtime_error("Query contains an unrecognized keyword");
    }

    if((*curr >= '0' && *curr <= '9') || *curr == '-') {
        static const regex numberRegex(R"(-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)");

        cmatch match;
        if(regex_search(curr, end, match, numberRegex, regex_constants::match_continuous)) {
            string_view num(curr, match.length());
            curr += match.length();
            return QueryToken(QueryTokenType::Number, num);
        } 
        /*const char* numberBegin = curr;

        while(curr < end && *curr != ',' && *curr != ')' && *curr != '\"' && *curr != '(' && *curr != '|' && *curr != '=' && *curr != '!' && *curr != '<' && *curr != '>'
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
            return QueryToken(QueryTokenType::Number, num);
        }
        */
    }

    throw runtime_error("Query contains an unrecognized character");
}