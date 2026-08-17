#include "query-parser.h"

QueryToken QueryScanner::scan() {
    while(curr < end && (*curr == ' ' || *curr == '\n' || *curr == '\t' || *curr == '\r')) {
        curr++;
    }

    if(curr >= end) {
        return QueryToken(QueryTokenType::End, {});
    }

    if(*curr == '(') {
        curr++;
        return QueryToken(QueryTokenType::LParen, "(");
    }
    if(*curr == ')') {
        curr++;
        return QueryToken(QueryTokenType::RParen, ")");
    }
    if(*curr == ',') {
        curr++;
        return QueryToken(QueryTokenType::Comma, ",");
    }
    if(*curr == '|') {
        curr++;
        return QueryToken(QueryTokenType::Pipe, "|");
    }

    if(*curr == '=' && curr + 1 < end && *(curr + 1) == '=') {
        curr += 2;
        return QueryToken(QueryTokenType::Eq, "==");
    }
    if(*curr == '!' && curr + 1 < end && *(curr + 1) == '=') {
        curr += 2;
        return QueryToken(QueryTokenType::Neq, "!=");
    }
    if(*curr == '<' && curr + 1 < end && *(curr + 1) == '=') {
        curr += 2;
        return QueryToken(QueryTokenType::Leq, "<=");
    }
    if(*curr == '<') {
        curr++;
        return QueryToken(QueryTokenType::Lt, "<");
    }
    if(*curr == '>' && curr + 1 < end && *(curr + 1) == '=') {
        curr += 2;
        return QueryToken(QueryTokenType::Geq, ">=");
    }
    if(*curr == '>') {
        curr++;
        return QueryToken(QueryTokenType::Gt, ">");
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

    if(*curr == 'A' && curr + 2 < end && *(curr + 1) == 'N' && *(curr + 2) == 'D') {
        curr += 3;
        return QueryToken(QueryTokenType::And, "AND");
    }

    if(*curr == 'O' && curr + 1 < end && *(curr + 1) == 'R') {
        curr += 2;
        return QueryToken(QueryTokenType::Or, "OR");
    }

    if(*curr == 'G' && curr + 2 < end && *(curr + 1) == 'E' && *(curr + 2) == 'T') {
        curr += 3;
        return QueryToken(QueryTokenType::Identifier, "GET");
    }

    if(*curr == 'F' && curr + 5 < end && *(curr + 1) == 'I' && *(curr + 2) == 'L' && *(curr + 3) == 'T' && *(curr + 4) == 'E' && *(curr + 5) == 'R') {
        curr += 6;
        return QueryToken(QueryTokenType::Identifier, "FILTER");
    }

    if(*curr == 'S' && curr + 3 < end && *(curr + 1) == 'O' && *(curr + 2) == 'R' && *(curr + 3) == 'T') {
        curr += 4;
        return QueryToken(QueryTokenType::Identifier, "SORT");
    }

    if(*curr == 'L' && curr + 4 < end && *(curr + 1) == 'I' && *(curr + 2) == 'M' && *(curr + 3) == 'I' && *(curr + 4) == 'T') {
        curr += 5;
        return QueryToken(QueryTokenType::Identifier, "LIMIT");
    }

    if(*curr == 'G' && curr + 6 < end && *(curr + 1) == 'R' && *(curr + 2) == 'O' && *(curr + 3) == 'U' && *(curr + 4) == 'P' && *(curr + 5) == 'B' && *(curr + 6) == 'Y') {
        curr += 7;
        return QueryToken(QueryTokenType::Identifier, "GROUPBY");
    }

    if(*curr == 'A' && curr + 6 < end && *(curr + 1) == 'V' && *(curr + 2) == 'E' && *(curr + 3) == 'R' && *(curr + 4) == 'A' && *(curr + 5) == 'G' && *(curr + 6) == 'E') {
        curr += 7;
        return QueryToken(QueryTokenType::Identifier, "AVERAGE");
    }

    if(*curr == 'A' && curr + 2 < end && *(curr + 1) == 'S' && *(curr + 2) == 'C') {
        curr += 3;
        return QueryToken(QueryTokenType::Identifier, "ASC");
    }

    if(*curr == 'D' && curr + 3 < end && *(curr + 1) == 'E' && *(curr + 2) == 'S' && *(curr + 3) == 'C') {
        curr += 4;
        return QueryToken(QueryTokenType::Identifier, "DESC");
    }

    if(*curr == 't' && curr + 3 < end && *(curr + 1) == 'r' && *(curr + 2) == 'u' && *(curr + 3) == 'e') {
        curr += 4;
        return QueryToken(QueryTokenType::Boolean, "true");
    }

    if(*curr == 'f' && curr + 4 < end && *(curr + 1) == 'a' && *(curr + 2) == 'l' && *(curr + 3) == 's' && *(curr + 4) == 'e') {
        curr += 5;
        return QueryToken(QueryTokenType::Boolean, "false");
    }

    if((*curr >= '0' && *curr <= '9') || *curr == '-') {
        const char* numberBegin = curr;

        while(curr < end && *curr != ',' && *curr != ')' && *curr != '\"' && *curr != '(' && *curr != '|' && *curr != '=' && *curr != '!' && *curr != '<' && *curr != '>'
                && *curr != ' ' && *curr != '\t' && *curr != '\n' && *curr != '\r') {
            curr++;
        }

        // Hand-rolled equivalent of -?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?,
        // matched in full (same as regex_match). Avoids allocating a string and
        // running a backtracking regex engine per number.
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
    }

    throw runtime_error("Query contains an unrecognized character");
}

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
    //functions.push_back(Get{node});
    return node;
}

void QueryParser::parseLimit() {
    if(!expect(QueryTokenType::LParen)) {
        throw runtime_error("Expected '(' after LIMIT");
    }

    if(currToken.type != QueryTokenType::Number) {
        throw runtime_error("Expected number in LIMIT");
    }

    int size = stoi(string(currToken.value));
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
            double num = stod(string(currToken.value));
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