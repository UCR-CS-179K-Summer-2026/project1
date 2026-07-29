#include "parser.h"

JSONValue Parser::parse() {
    switch(currToken.type) {
        case TokenType::LBrace: {
            vector<pair<string_view, JSONValue>> object;
            currToken = scanner.scan();

            while(currToken.type != TokenType::RBrace) {
                if(currToken.type != TokenType::String) {
                    throw runtime_error("Expected string key in object");
                }
                string_view key = currToken.value;
                currToken = scanner.scan();

                if(currToken.type != TokenType::Colon) {
                    throw runtime_error("Expected colon after key in object");
                }
                currToken = scanner.scan();

                JSONValue value = parse();
                object.emplace_back(key, value);

                if(currToken.type == TokenType::Comma) {
                    currToken = scanner.scan();
                } else if(currToken.type != TokenType::RBrace) {
                    throw runtime_error("Expected comma or closing brace in object");
                }
            }
            currToken = scanner.scan();
            return JSONValue(ValueType::Object, object);
        }

        case TokenType::LBracket: {
            vector<JSONValue> array;
            currToken = scanner.scan();

            while(currToken.type != TokenType::RBracket) {
                JSONValue value = parse();
                array.push_back(value);

                if(currToken.type == TokenType::Comma) {
                    currToken = scanner.scan();
                } else if(currToken.type != TokenType::RBracket) {
                    throw runtime_error("Expected comma or closing bracket in array");
                }
            }
            currToken = scanner.scan();
            return JSONValue(ValueType::Array, array);
        }

        case TokenType::String: {
            string_view str = currToken.value;
            currToken = scanner.scan();
            return JSONValue(ValueType::String, str);
        }

        case TokenType::Number: {
            double num = stod(string(currToken.value));
            currToken = scanner.scan();
            return JSONValue(ValueType::Number, num);
        }

        case TokenType::Boolean: {
            bool b = (currToken.value == "true");
            currToken = scanner.scan();
            return JSONValue(ValueType::Boolean, b);
        }

        case TokenType::Null: {
            currToken = scanner.scan();
            return JSONValue(ValueType::Null, nullptr);
        }

        default:
            throw runtime_error("Unexpected token");
    }
}