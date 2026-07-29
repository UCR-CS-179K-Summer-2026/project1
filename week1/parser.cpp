#include "parser.h"

JSONValuePtr Parser::parse() {
    switch(currToken.type) {
        case TokenType::LBrace: {
            vector<pair<string, JSONValuePtr>> object;
            currToken = scanner.scan();

            while(currToken.type != TokenType::RBrace) {
                if(currToken.type != TokenType::String) {
                    throw runtime_error("Expected string key in object");
                }
                string key(currToken.value);
                currToken = scanner.scan();

                if(currToken.type != TokenType::Colon) {
                    throw runtime_error("Expected colon after key in object");
                }
                currToken = scanner.scan();

                JSONValuePtr value = parse();
                object.emplace_back(std::move(key), std::move(value));

                if(currToken.type == TokenType::Comma) {
                    currToken = scanner.scan();
                } else if(currToken.type != TokenType::RBrace) {
                    throw runtime_error("Expected comma or closing brace in object");
                }
            }
            currToken = scanner.scan();
            return make_unique<JSONObject>(std::move(object));
        }

        case TokenType::LBracket: {
            vector<JSONValuePtr> array;
            currToken = scanner.scan();

            while(currToken.type != TokenType::RBracket) {
                array.push_back(parse());

                if(currToken.type == TokenType::Comma) {
                    currToken = scanner.scan();
                } else if(currToken.type != TokenType::RBracket) {
                    throw runtime_error("Expected comma or closing bracket in array");
                }
            }
            currToken = scanner.scan();
            return make_unique<JSONArray>(std::move(array));
        }

        case TokenType::String: {
            string str(currToken.value);
            currToken = scanner.scan();
            return make_unique<JSONString>(std::move(str));
        }

        case TokenType::Number: {
            double num = stod(string(currToken.value));
            currToken = scanner.scan();
            return make_unique<JSONNumber>(num);
        }

        case TokenType::Boolean: {
            bool b = (currToken.value == "true");
            currToken = scanner.scan();
            return make_unique<JSONBoolean>(b);
        }

        case TokenType::Null: {
            currToken = scanner.scan();
            return make_unique<JSONNull>();
        }

        default:
            throw runtime_error("Unexpected token");
    }
}