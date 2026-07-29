#include <cstdint>
#include <regex>
#include <string>
#include <string_view>
using namespace std;

enum class TokenType : uint8_t {Boolean, Colon, Comma, End, LBrace, LBracket, Null, Number, RBrace, RBracket, String};

struct Token {
    TokenType type;
    string_view value;

    Token(TokenType t, string_view v) : type(t), value(v) {}
};

class Scanner {
    private:
    const char* curr;
    const char* end;

    public:
    Scanner(std::string_view json) : curr(json.data()), end(json.data() + json.size()) {}

    Token scan();
};