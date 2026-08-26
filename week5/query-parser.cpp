#include "query-parser.h"

void QueryParser::parse() {
    parsePipeline();

    if(currToken.type != QueryTokenType::End) {
        throw runtime_error("Unexpected token after query pipeline: " + string(currToken.value));
    }
}

void QueryParser::advance() {
    currToken = scanner.scan();
}

bool QueryParser::expect(QueryTokenType type) {
    if(currToken.type == type) {
        advance();
        return true;
    }
    return false;
}

NodeId QueryParser::allocateNode(ExpressionNode node) {
    expressionNodes.push_back(move(node));
    return static_cast<NodeId>(expressionNodes.size() - 1);
}

NodeId QueryParser::allocateGet(vector<string_view> props) {
    return allocateNode(GetOperand{move(props)});
}

NodeId QueryParser::allocateBinaryOp(NodeId left, QueryTokenType op, NodeId right) {
    return allocateNode(BinaryOperand{op, left, right});
}

void QueryParser::parsePipeline() {
    while(currToken.type != QueryTokenType::End) {
        if(currToken.type == QueryTokenType::Identifier) {
            string_view funcName = currToken.value;
            advance();

            if(funcName == "GET") {
                NodeId node = parseGet();
                functions.push_back(Get{node});
            } else if(funcName == "FILTER") {
                parseFilter();
            } else if(funcName == "SORT") {
                parseSort();
            } else if(funcName == "LIMIT") {
                parseLimit();
            } else if(funcName == "GROUPBY") {
                parseGroupBy();
            } else if(funcName == "AVERAGE") {
                NodeId node = parseAverage();
                functions.push_back(Average{node});
            } else {
                throw runtime_error("Unknown function: " + string(funcName));
            }
        } else {
            throw runtime_error("Expected function name, got: " + string(currToken.value));
        }

        if(currToken.type == QueryTokenType::Pipe) {
            advance();  // consume the pipe
        } else if(currToken.type != QueryTokenType::End) {
            throw runtime_error("Expected '|' or end of query, got: " + string(currToken.value));
        }
    }
}

NodeId QueryParser::parseGet() {
    if(!expect(QueryTokenType::LParen)) {
        throw runtime_error("Expected '(' after GET, instead got: " + string(currToken.value));
    }

    vector<string_view> props;

    while(currToken.type != QueryTokenType::RParen) {
        if(currToken.type != QueryTokenType::String) {
            if(currToken.type == QueryTokenType::Number) {
                string numStr(currToken.value);
                if(numStr.find('.') != string::npos || numStr.find('-') != string::npos || numStr.find('e') != string::npos || numStr.find('E') != string::npos) {
                    throw runtime_error("Invalid index number in GET- number must be a positive whole number");
                }
            } else {
                throw runtime_error("Expected property name or index in GET");
            }
        }
        props.push_back(currToken.value);
        advance();

        if(currToken.type == QueryTokenType::Comma) {
            advance();

            if(currToken.type == QueryTokenType::RParen) {
                throw runtime_error("Trailing comma in GET");
            }
        } else if(currToken.type != QueryTokenType::RParen) {
            throw runtime_error("Expected ',' or ')' in GET");
        }
    }
    advance();  // consume ')'

    NodeId node = allocateGet(move(props));
    return node;
}

void QueryParser::parseLimit() {
    if(!expect(QueryTokenType::LParen)) {
        throw runtime_error("Expected '(' after LIMIT");
    }

    if(currToken.type != QueryTokenType::Number) {
        throw runtime_error("Expected number in LIMIT");
    }

    int size;
    auto [ptr, ec] = from_chars(currToken.value.data(), currToken.value.data() + currToken.value.size(), size);
    if (ec != errc() || ptr != currToken.value.data() + currToken.value.size()) {
        throw runtime_error("Expected number in LIMIT");
    }
    if(size < 0) {
        throw runtime_error("LIMIT size must be non-negative");
    }
    advance();

    if(!expect(QueryTokenType::RParen)) {
        throw runtime_error("Expected ')' after LIMIT value");
    }

    functions.push_back(Limit{size});
}

void QueryParser::parseFilter() {
    if(!expect(QueryTokenType::LParen)) {
        throw runtime_error("Expected '(' after FILTER");
    }

    NodeId condition = parseExpression();

    if(!expect(QueryTokenType::RParen)) {
        throw runtime_error("Expected ')' after FILTER condition");
    }

    functions.push_back(Filter{condition});
}

void QueryParser::parseSort() {
    if(!expect(QueryTokenType::LParen)) {
        throw runtime_error("Expected '(' after SORT");
    }

    if(!expect(QueryTokenType::Identifier)) {
        if(currToken.value != "GET") {
            throw runtime_error("Expected GET(...) as target for SORT");
        }
    }
    NodeId target = parseGet();

    if(!expect(QueryTokenType::Comma)) {
        throw runtime_error("Expected ',' after SORT target");
    }

    if(currToken.type != QueryTokenType::Identifier || (currToken.value != "ASC" && currToken.value != "DESC")) {
        throw runtime_error("Expected 'ASC' or 'DESC' in SORT");
    }

    Direction direction = (currToken.value == "ASC") ? Direction::Asc : Direction::Desc;
    advance();

    if(!expect(QueryTokenType::RParen)) {
        throw runtime_error("Expected ')' after SORT direction");
    }

    functions.push_back(Sort{target, direction});
}

void QueryParser::parseGroupBy() {
    if(!expect(QueryTokenType::LParen)) {
        throw runtime_error("Expected '(' after GROUPBY");
    }

    if(!expect(QueryTokenType::Identifier)) {
        if(currToken.value != "GET") {
            throw runtime_error("Expected GET(...) as target for GROUPBY");
        }
    }
    NodeId target = parseGet();

    if(!expect(QueryTokenType::RParen)) {
        throw runtime_error("Expected ')' after GROUPBY target");
    }

    functions.push_back(GroupBy{target});
}

NodeId QueryParser::parseAverage() {
    if(!expect(QueryTokenType::LParen)) {
        throw runtime_error("Expected '(' after AVERAGE");
    }

    NodeId target;
    if(currToken.type == QueryTokenType::RParen) {
        target = allocateGet({});  // AVERAGE() with no args targets the current JSON value
    } else {
        if(!expect(QueryTokenType::Identifier)) {
            if(currToken.value != "GET") {
                throw runtime_error("Expected GET(...) as target for AVERAGE");
            }
        }
        target = parseGet();
    }

    if(!expect(QueryTokenType::RParen)) {
        throw runtime_error("Expected ')' after AVERAGE target");
    }

    vector<string_view> path = get<GetOperand>(expressionNodes[target]).path;
    return allocateNode(AverageOperand{move(path)});
}

NodeId QueryParser::parseExpression() {
    return parseLogicalOr(); //OR has lower precedence
}

NodeId QueryParser::parseLogicalOr() {
    NodeId left = parseLogicalAnd();

    while(currToken.type == QueryTokenType::Or) {
        advance();
        NodeId right = parseLogicalAnd();
        left = allocateBinaryOp(left, QueryTokenType::Or, right);
    }

    return left;
}

NodeId QueryParser::parseLogicalAnd() {
    NodeId left = parseComparison();

    while(currToken.type == QueryTokenType::And) {
        advance();
        NodeId right = parseComparison();
        left = allocateBinaryOp(left, QueryTokenType::And, right);
    }

    return left;
}

NodeId QueryParser::parseComparison() {
    NodeId left = parseOperand();

    if(currToken.type == QueryTokenType::Eq || currToken.type == QueryTokenType::Neq ||
       currToken.type == QueryTokenType::Lt || currToken.type == QueryTokenType::Leq ||
       currToken.type == QueryTokenType::Gt || currToken.type == QueryTokenType::Geq) {
        QueryTokenType op = currToken.type;
        advance();
        NodeId right = parseOperand();
        left = allocateBinaryOp(left, op, right);
    }

    return left;
}

NodeId QueryParser::parseOperand() {
    switch(currToken.type) {
        case QueryTokenType::LParen: {
            advance();
            NodeId node = parseExpression();

            if(!expect(QueryTokenType::RParen)) {
                throw runtime_error("Expected ')' after expression");
            }
            return node;
        }

        case QueryTokenType::String: {
            NodeId node = allocateNode(StringOperand{currToken.value});
            advance();
            return node;
        }

        case QueryTokenType::Number: {
            double num;
            auto [ptr, ec] = from_chars(currToken.value.data(), currToken.value.data() + currToken.value.size(), num);
            if (ec != errc() || ptr != currToken.value.data() + currToken.value.size()) {
                throw runtime_error("Invalid number literal " + string(currToken.value));
            }
            NodeId node = allocateNode(NumberOperand{num});
            advance();
            return node;
        }

        case QueryTokenType::Boolean: {
            bool b = (currToken.value == "true");
            NodeId node = allocateNode(BooleanOperand{b});
            advance();
            return node;
        }

        case QueryTokenType::Identifier: {
            if(currToken.value == "AVERAGE") {
                advance();
                return parseAverage();
            }
            if(currToken.value != "GET") {
                throw runtime_error("Unexpected identifier in expression: " + string(currToken.value));
            }
            advance();

            return parseGet();
        }

        default:
            throw runtime_error("Unexpected token in expression");
    }
}