#include "value.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace std;

enum class QueryTokenType : uint8_t {And, Boolean, Comma, End, Eq, Geq, Gt, Identifier, Leq, Lt, LParen, Neq, Number, Or, Pipe, RParen, String};
enum class Direction : uint8_t {Asc, Desc};

struct QueryToken {
    QueryTokenType type;
    string_view value;

    QueryToken(QueryTokenType t, string_view v) : type(t), value(v) {}

    string getUnescaped() const {
        if (type != QueryTokenType::String) {
            throw runtime_error("getUnescaped() called on non-string token");
        }
        return unescapeString(value);
    }
};

class QueryScanner {
    private:
    const char* curr;
    const char* end;

    public:
    explicit QueryScanner(string_view query) : curr(query.data()), end(query.data() + query.size()) {}

    QueryToken scan();
};