// Jules contributed to this file. This file is meant to implement the Parser class, which is responsible for parsing a JSON or JSONL string and producing a JSONValue.
// It uses the Scanner class to tokenize the input string, and implements a recursive descent parser to build the JSONValue.
#include "parser.h"

JSONValue Parser::parse() {
    switch(currToken.type) {
        case TokenType::LBrace: {
            vector<pair<string, JSONValue>> object;
            currToken = scanner.scan();

            while(currToken.type != TokenType::RBrace) {
                if(currToken.type != TokenType::String) {
                    if(fileType == ParserType::JSON) {
                        throw runtime_error("Warning: line " + to_string(scanner.getLineNumber()) + " failed to parse");
                    } else {
                        throw runtime_error("Expected string key in object");
                    }
                }
                string key = unescapeString(currToken.value);
                currToken = scanner.scan();

                if(currToken.type != TokenType::Colon) {
                    if(fileType == ParserType::JSON) {
                        throw runtime_error("Warning: line " + to_string(scanner.getLineNumber()) + " failed to parse");
                    } else {
                        throw runtime_error("Expected colon after key in object");
                    }
                }
                currToken = scanner.scan();

                JSONValue value = parse();
                object.emplace_back(move(key), move(value));

                if(currToken.type == TokenType::Comma) {
                    currToken = scanner.scan();
                } else if(currToken.type != TokenType::RBrace) {
                    if(fileType == ParserType::JSON) {
                        throw runtime_error("Warning: line " + to_string(scanner.getLineNumber()) + " failed to parse");
                    } else {
                        throw runtime_error("Expected comma or closing brace in object");
                    }
                }
            }
            currToken = scanner.scan();
            return JSONValue(move(object));
        }

        case TokenType::LBracket: {
            vector<JSONValue> array;
            currToken = scanner.scan();

            while(currToken.type != TokenType::RBracket) {
                array.push_back(parse());

                if(currToken.type == TokenType::Comma) {
                    currToken = scanner.scan();
                } else if(currToken.type != TokenType::RBracket) {
                    if(fileType == ParserType::JSON) {
                        throw runtime_error("Warning: line " + to_string(scanner.getLineNumber()) + " failed to parse");
                    } else {
                        throw runtime_error("Expected comma or closing bracket in array");
                    }
                }
            }
            currToken = scanner.scan();
            return JSONValue(move(array));
        }

        case TokenType::String: {
            string str = unescapeString(currToken.value);
            currToken = scanner.scan();
            return JSONValue(move(str));
        }

        case TokenType::Number: {
            double num = stod(string(currToken.value));
            currToken = scanner.scan();
            return JSONValue(num);
        }

        case TokenType::Boolean: {
            bool b = (currToken.value == "true");
            currToken = scanner.scan();
            return JSONValue(b);
        }

        case TokenType::Null: {
            currToken = scanner.scan();
            return JSONValue(nullptr);
        }

        default:
            throw runtime_error("Unexpected token");
    }
}

string Parser::unescapeString(const string_view& str) {
    string result;

    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            i++;
            switch (str[i]) {
                case '\"': result += '\"'; break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'u':
                    if (i + 4 < str.size()) {
                        string hex = string(str.substr(i + 1, 4));
                        char16_t codePoint = static_cast<char16_t>(stoi(hex, nullptr, 16));
                        result += static_cast<char>(codePoint);
                        i += 4;
                    }

                    break;
                default:
                    continue;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}