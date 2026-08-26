// Jules contributed to this file. This file is meant to implement the FileParser class, which is responsible for parsing a JSON or JSONL string and producing a JSONValue.
// It uses the FileScanner class to tokenize the input string, and implements a recursive descent parser to build the JSONValue.
#include "file-parser.h"

JSONValue FileParser::parse() {
    if(fileType == ParserType::JSON) {
        if(currToken.type != JSONTokenType::End) {
            JSONValue result = parseObject();

            if(currToken.type != JSONTokenType::End) {
                throw runtime_error("Trailing content");
            }

            return result;
        }

        return JSONValue(nullptr);
    }

    ArrayValue records;
    while(currToken.type != JSONTokenType::End) {
        while(currToken.type != JSONTokenType::ObjEnd && currToken.type != JSONTokenType::End) {
            records.push_back(parseObject());
        }
        currToken = scanner.scan();
    }

    return JSONValue(move(records));
}

JSONValue FileParser::parseObject() {
    switch(currToken.type) {
        case JSONTokenType::LBrace: {
            vector<pair<string, JSONValue>> object;
            currToken = scanner.scan();

            while(currToken.type != JSONTokenType::RBrace) {
                if(currToken.type != JSONTokenType::String) {
                    throw runtime_error("Expected string key in object at line " + to_string(scanner.getLineNumber()));
                }
                string key(currToken.value);
                currToken = scanner.scan();

                if(currToken.type != JSONTokenType::Colon) {
                    throw runtime_error("Expected colon after key in object at line " + to_string(scanner.getLineNumber()));
                }
                currToken = scanner.scan();

                JSONValue value = parseObject();
                object.emplace_back(move(key), move(value));

                if(currToken.type == JSONTokenType::Comma) {
                    currToken = scanner.scan();
                } else if(currToken.type != JSONTokenType::RBrace) {
                    throw runtime_error("Expected comma or closing brace in object at line " + to_string(scanner.getLineNumber()));
                }
            }
            currToken = scanner.scan();
            return JSONValue(move(object));
        }

        case JSONTokenType::LBracket: {
            vector<JSONValue> array;
            currToken = scanner.scan();

            while(currToken.type != JSONTokenType::RBracket) {
                array.push_back(parseObject());

                if(currToken.type == JSONTokenType::Comma) {
                    currToken = scanner.scan();
                } else if(currToken.type != JSONTokenType::RBracket) {
                    throw runtime_error("Expected comma or closing bracket in array at line " + to_string(scanner.getLineNumber()));
                }
            }
            currToken = scanner.scan();
            return JSONValue(move(array));
        }

        case JSONTokenType::String: {
            string str(currToken.value);
            currToken = scanner.scan();
            return JSONValue(move(str));
        }

        case JSONTokenType::Number: {
            double num;
            auto [ptr, ec] = from_chars(currToken.value.data(), currToken.value.data() + currToken.value.size(), num);
            if (ec != errc() || ptr != currToken.value.data() + currToken.value.size()) {
                throw runtime_error("Invalid number literal at line " + to_string(scanner.getLineNumber()));
            }
            currToken = scanner.scan();
            return JSONValue(num);
        }

        case JSONTokenType::Boolean: {
            bool b = (currToken.value == "true");
            currToken = scanner.scan();
            return JSONValue(b);
        }

        case JSONTokenType::Null: {
            currToken = scanner.scan();
            return JSONValue(nullptr);
        }

        default:
            throw runtime_error("Unexpected token");
    }
}