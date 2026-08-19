#pragma once

#include <charconv>

using namespace std;

#include "query-scanner.h"

//The following structs are used as nodes for an expression tree
using NodeId = uint32_t; //NodeIds are always index values to access expressionNodes or functions.

struct StringOperand {string_view val;};
struct NumberOperand {double val;};
struct BooleanOperand {bool val;};
struct GetOperand {vector<string_view> path;};
struct AverageOperand {vector<string_view> path;};

struct BinaryOperand {
    QueryTokenType op;
    NodeId left;
    NodeId right;
};

using ExpressionNode = variant<AverageOperand, BooleanOperand, StringOperand, NumberOperand, GetOperand, BinaryOperand>;

//The following structs are used to represent the functions in the query pipeline
struct Get{NodeId target;};
struct Filter{NodeId condition;};
struct Sort{NodeId target; Direction direction;};
struct Limit{int size;};
struct GroupBy{NodeId target;};
struct Average{NodeId target;};

using QueryFunction = variant<Get, Filter, Sort, Limit, GroupBy, Average>;

//Parser
class QueryParser {
    private:
    QueryScanner scanner;
    QueryToken currToken;

    vector<ExpressionNode> expressionNodes;
    vector<QueryFunction> functions;

    public:
    explicit QueryParser(string_view query) : scanner(query), currToken(scanner.scan()) {}

    const vector<ExpressionNode>& getExpressionNodes() const {return expressionNodes;}
    const vector<QueryFunction>& getFunctions() const {return functions;}

    void parse();

    private:
    void advance();
    bool expect(QueryTokenType type);

    NodeId allocateNode(ExpressionNode node);
    NodeId allocateGet(vector<string_view> props);
    NodeId allocateBinaryOp(NodeId left, QueryTokenType op, NodeId right);

    void parsePipeline();
    NodeId parseGet();
    void parseLimit();
    void parseFilter();
    void parseSort();
    void parseGroupBy();
    NodeId parseAverage();

    NodeId parseExpression();
    NodeId parseLogicalOr();
    NodeId parseLogicalAnd();
    NodeId parseComparison();
    NodeId parseOperand(); //This will route to allocateGet() when it sees GET as an operand
    
};