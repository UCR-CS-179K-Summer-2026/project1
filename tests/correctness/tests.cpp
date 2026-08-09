// Unit tests for the parser, the file reader, and lookup().
//
// Build:
//   g++ -std=c++17 -Wall -Wextra -Iweek1 week1/scanner.cpp week1/parser.cpp
//     week1/file-reader.cpp tests/tests.cpp -o tests
// Run:
//   ./tests tests/data
#include <iostream>

#include "file-reader.h"

int passed = 0;
int failed = 0;

string dataDir = "tests/data";

void section(const string& name) {
    cout << "\n" << name << "\n";
}

void check(bool ok, const string& label) {
    if (ok) {
        passed++;
    } else {
        failed++;
        cout << "  FAIL " << label << "\n";
    }
}

string parseAndFormat(const string& text) {
    Parser parser(text, ParserType::JSON);
    JSONValue value = parser.parse();
    LookupResult whole = {true, &value, ""};
    return formatResult(whole);
}

void checkParse(const string& text, const string& want) {
    string got = parseAndFormat(text);
    if (got == want) {
        passed++;
    } else {
        failed++;
        cout << "  FAIL " << text << "\n         got  " << got << "\n         want " << want << "\n";
    }
}

bool parses(const string& text) {
    try {
        Parser parser(text, ParserType::JSON);
        parser.parse();
        return true;
    } catch (const exception&) {
        return false;
    }
}

void checkValid(const string& text) {
    check(parses(text), "should have parsed: " + text);
}

void checkInvalid(const string& text) {
    check(!parses(text), "should have been rejected: " + text);
}

// Splits a dotted/bracketed path like ".student.scores[0]" into the
// segments get() expects, e.g. {"student", "scores", "0"}.
vector<string_view> splitPath(const string& path) {
    vector<string_view> segments;
    size_t i = 0;
    while (i < path.size()) {
        if (path[i] == '.') {
            i++;
            size_t start = i;
            while (i < path.size() && path[i] != '.' && path[i] != '[') i++;
            segments.push_back(string_view(path).substr(start, i - start));
        } else if (path[i] == '[') {
            i++;
            size_t start = i;
            while (i < path.size() && path[i] != ']') i++;
            segments.push_back(string_view(path).substr(start, i - start));
            if (i < path.size()) i++;  // skip ']'
        } else {
            i++;  // skip an unexpected character rather than getting stuck
        }
    }
    return segments;
}

void checkLookup(const string& text, const string& path, const string& want) {
    Parser parser(text, ParserType::JSON);
    JSONValue value = parser.parse();
    string got = formatResult(get(value, splitPath(path)));
    if (got == want) {
        passed++;
    } else {
        failed++;
        cout << "  FAIL " << path << " on " << text << "\n         got  " << got
             << "\n         want " << want << "\n";
    }
}

// -1 means readFile threw.
int recordCount(const string& file) {
    try {
        return (int)readFile(dataDir + "/" + file).size();
    } catch (const exception&) {
        return -1;
    }
}

void checkCount(const string& file, int want) {
    int got = recordCount(file);
    if (got == want) {
        passed++;
    } else {
        failed++;
        cout << "  FAIL " << file << " has " << got << " records, wanted " << want << "\n";
    }
}

void checkField(const string& file, size_t index, const string& path, const string& want) {
    vector<JSONValue> records = readFile(dataDir + "/" + file);
    string got = index < records.size() ? formatResult(get(records[index], splitPath(path))) : "<no record>";
    if (got == want) {
        passed++;
    } else {
        failed++;
        cout << "  FAIL " << file << "[" << index << "] " << path << "\n         got  " << got
             << "\n         want " << want << "\n";
    }
}

void testPrimitives() {
    section("primitive values");
    checkParse("42", "42");
    checkParse("-5", "-5");
    checkParse("3.14", "3.14");
    checkParse("-0.5", "-0.5");
    checkParse("0", "0");
    checkParse("1e3", "1000");
    checkParse("\"hello\"", "\"hello\"");
    checkParse("\"\"", "\"\"");
    checkParse("true", "true");
    checkParse("false", "false");
    checkParse("null", "null");

    section("types come back right");
    Parser p1("7", ParserType::JSON);
    check(p1.parse().getType() == ValueType::Number, "7 should be a Number");
    Parser p2("\"s\"", ParserType::JSON);
    check(p2.parse().getType() == ValueType::String, "\"s\" should be a String");
    Parser p3("true", ParserType::JSON);
    check(p3.parse().getType() == ValueType::Boolean, "true should be a Boolean");
    Parser p4("null", ParserType::JSON);
    check(p4.parse().getType() == ValueType::Null, "null should be a Null");
    Parser p5("{}", ParserType::JSON);
    check(p5.parse().getType() == ValueType::Object, "{} should be an Object");
    Parser p6("[]", ParserType::JSON);
    check(p6.parse().getType() == ValueType::Array, "[] should be an Array");
}

void testNested() {
    section("empty objects and arrays");
    checkParse("{}", "{}");
    checkParse("[]", "[]");
    checkParse("{\"a\":{}}", "{\"a\":{}}");
    checkParse("{\"a\":[]}", "{\"a\":[]}");

    section("nested objects");
    checkParse(R"({"a":{"b":"c"}})", R"({"a":{"b":"c"}})");
    checkParse(R"({"student":{"name":"Ryan"}})", R"({"student":{"name":"Ryan"}})");
    checkParse(R"({"a":{"b":{"c":{"d":1}}}})", R"({"a":{"b":{"c":{"d":1}}}})");
    checkParse(R"({"a":1,"b":2,"c":3})", R"({"a":1,"b":2,"c":3})");

    section("nested arrays");
    checkParse("[1,2,3]", "[1,2,3]");
    checkParse("[[1,2],[3,4]]", "[[1,2],[3,4]]");
    checkParse("[[[1]]]", "[[[1]]]");
    checkParse(R"({"scores":[90,85]})", R"({"scores":[90,85]})");

    section("arrays and objects mixed together");
    checkParse(R"([1,"two",true,null])", R"([1,"two",true,null])");
    checkParse(R"([{"k":"v"},[1,2],3])", R"([{"k":"v"},[1,2],3])");
    checkParse(R"({"a":[{"b":1}]})", R"({"a":[{"b":1}]})");
    checkParse(R"({"student":{"name":"Ryan","scores":[90,85]}})",
               R"({"student":{"name":"Ryan","scores":[90,85]}})");
    checkParse(R"({"student":[{"name":"Ryan","scores":[90,85]},{"name":"Nobody"}]})",
               R"({"student":[{"name":"Ryan","scores":[90,85]},{"name":"Nobody"}]})");
}

void testWhitespace() {
    section("whitespace");
    checkValid("  {\"a\":1}  ");
    checkValid("{\n\"a\":1}");
    checkValid("{\t\"a\" :1}");
    checkValid(R"({"a":1, "b":2})");

}

void testInvalid() {
    section("bad JSON gets rejected");
    checkInvalid("");
    checkInvalid("{");
    checkInvalid(R"({"a":1)");
    checkInvalid(R"({"a" 1})");
    checkInvalid(R"({"a":})");
    checkInvalid(R"({a:1})");
    checkInvalid(R"({'a':1})");
    checkInvalid(R"({"a":"oops)");
    checkInvalid(R"({"a":01})");
    checkInvalid("[1,2");
    checkInvalid("}");
    checkInvalid("]");
    checkInvalid(":");
    checkInvalid("nul");
    checkInvalid("tru");

    section("error messages");
    try {
        Parser parser(R"({"a" 1})", ParserType::JSON);
        parser.parse();
        check(false, "missing colon should have thrown");
    } catch (const exception& e) {
        string message = e.what();
        check(message.find("line 1") != string::npos, "JSON error should say the line number");
    }
    try {
        Parser parser(R"({"a" 1})", ParserType::JSONL);
        parser.parse();
        check(false, "missing colon should have thrown in JSONL mode too");
    } catch (const exception& e) {
        string message = e.what();
        check(message.find("colon") != string::npos, "JSONL error should mention the colon");
    }
}

void testStrings() {
    section("strings and escapes");
    checkParse(R"({"a":"he said \"hi\""})", R"({"a":"he said \"hi\""})");
    checkParse(R"({"a":"c:\\path"})", R"({"a":"c:\\path"})");
    checkParse(R"({"key with spaces":1})", R"({"key with spaces":1})");
    checkParse(R"({"":1})", R"({"":1})");

    // escapes stay as written, not decoded
    checkLookup(R"({"a":"x\ny"})", ".a", R"("x\ny")");

    section("duplicate keys");
    checkParse(R"({"a":1,"a":2})", R"({"a":1,"a":2})");
    checkLookup(R"({"a":1,"a":2})", ".a", "1");

    section("number formatting");
    checkParse("90", "90");
    checkParse("0", "0");
    checkParse("-5", "-5");

}

void testLookup() {
    const string student =
        R"({"student":{"name":"Ryan","scores":[90,85],"active":true,"gpa":3.75,"advisor":null}})";
    const string roster =
        R"({"student":[{"name":"Ryan","scores":[90,85]},{"name":"Javier","scores":[95,88]},{"name":"Nobody"}]})";

    section("looking up fields");
    checkLookup(student, ".student.name", "\"Ryan\"");
    checkLookup(student, ".student.active", "true");
    checkLookup(student, ".student.gpa", "3.75");
    checkLookup(student, ".student.advisor", "null");
    checkLookup(student, ".student.scores", "[90,85]");

    section("nested paths and array indexes");
    checkLookup(student, ".student.scores[0]", "90");
    checkLookup(student, ".student.scores[1]", "85");
    checkLookup(roster, ".student[0].name", "\"Ryan\"");
    checkLookup(roster, ".student[1].name", "\"Javier\"");
    checkLookup(roster, ".student[1].scores[1]", "88");
    checkLookup(R"([{"x":1},{"x":2}])", "[1].x", "2");
    checkLookup(R"([[1,2],[3,4]])", "[1][0]", "3");
    checkLookup(R"({"a":{"b":{"c":{"d":"deep"}}}})", ".a.b.c.d", "\"deep\"");

    section("empty path gives back the whole record");
    checkLookup(R"({"a":1})", "", R"({"a":1})");
    checkLookup(student, ".student",
                R"({"name":"Ryan","scores":[90,85],"active":true,"gpa":3.75,"advisor":null})");

    section("missing fields");
    checkLookup(student, ".student.missing", "Error: field 'missing' not found");
    checkLookup(student, ".nope", "Error: field 'nope' not found");
    checkLookup(roster, ".student[2].scores", "Error: field 'scores' not found");
    checkLookup(R"({"a":1})", ".a.b", "Error: expected an object to look up field 'b'");
    checkLookup(R"({})", ".anything", "Error: field 'anything' not found");

    Parser parser(R"({"a":1})", ParserType::JSON);
    JSONValue root = parser.parse();
    check(get(root, splitPath(".missing")).ok == false, "missing field should not be ok");
    check(get(root, splitPath(".missing")).value == nullptr, "missing field should have no value");
    check(get(root, splitPath(".a")).ok == true, "found field should be ok");
    check(get(root, splitPath(".a")).value != nullptr, "found field should have a value");

    section("bad array indexes");
    checkLookup(student, ".student.scores[2]", "Error: index 2 out of range");
    checkLookup(student, ".student.scores[99]", "Error: index 99 out of range");
    checkLookup(student, ".student.scores[-1]", "Error: index -1 out of range");
    checkLookup(R"({"a":[]})", ".a[0]", "Error: index 0 out of range");
    checkLookup(student, ".student.scores[abc]", "Error: invalid array index 'abc'");
    checkLookup(student, ".student.scores[0", "Error: missing closing ']' in path");
    checkLookup(student, ".student.name[0]", "Error: expected an array to index with [0]");

    section("bad paths");
    checkLookup(student, "student.name", "Error: invalid path syntax at position 0");
    checkLookup(student, "junk", "Error: invalid path syntax at position 0");
    checkLookup(student, " .student", "Error: invalid path syntax at position 0");
    checkLookup(student, ".", "Error: field '' not found");
    checkLookup(R"({"":5})", ".", "5");

    section("how results get printed");
    check(formatResult({false, nullptr, "something went wrong"}) == "Error: something went wrong",
          "errors print with an Error: prefix");
    checkLookup(R"({"a":"text"})", ".a", "\"text\"");
    checkLookup(R"({"a":1})", ".a", "1");
    checkLookup(R"({"a":true})", ".a", "true");
    checkLookup(R"({"a":null})", ".a", "null");
    checkLookup(R"({"a":{"b":[1,{"c":2}]}})", ".a", R"({"b":[1,{"c":2}]})");
}

void testFiles() {
    section("empty files");
    checkCount("empty.jsonl", 0);
    checkCount("empty.json", 0);
    checkCount("blank-lines.jsonl", 0);

    section("jsonl files with several records");
    checkCount("records.jsonl", 3);
    checkField("records.jsonl", 0, ".name", "\"Ryan\"");
    checkField("records.jsonl", 1, ".name", "\"Javier\"");
    checkField("records.jsonl", 2, ".name", "\"Jules\"");
    checkField("records.jsonl", 2, ".id", "3");
    checkCount("single.jsonl", 1);
    checkCount("students.jsonl", 4);
    checkField("students.jsonl", 0, ".student.name", "\"Ryan\"");
    checkField("students.jsonl", 3, ".student.name", "\"Nobody\"");
    checkField("students.jsonl", 3, ".student.scores", "Error: field 'scores' not found");

    section("blank lines in the middle of a file");
    checkCount("gaps.jsonl", 3);
    checkField("gaps.jsonl", 1, ".id", "2");

    section("json files");
    checkCount("nested.json", 1);
    checkField("nested.json", 0, ".student.advisor.name", "\"Dr. Smith\"");
    checkField("nested.json", 0, ".student.scores[1]", "85");
    checkCount("primitives.json", 1);
    checkField("primitives.json", 0, ".number", "42");
    checkField("primitives.json", 0, ".nothing", "null");
    checkField("primitives.json", 0, ".no", "false");
    checkCount("scalar.json", 1);

    section("a top level array becomes one record per element");
    checkCount("array.json", 3);
    checkField("array.json", 0, ".city", "\"Riverside\"");
    checkField("array.json", 2, ".city", "\"Los Angeles\"");
    checkCount("mixed.json", 6);
    checkField("mixed.json", 1, "", "\"two\"");
    checkField("mixed.json", 4, ".k", "\"v\"");

    section("invalid file");
    checkCount("invalid.json", 0);

    section("file extensions");
    checkCount("upper.JSONL", 2);
    checkCount("noextension", 1);

    section("missing file");
    checkCount("does-not-exist.json", -1);
    try {
        readFile(dataDir + "/does-not-exist.json");
        check(false, "a missing file should have thrown");
    } catch (const exception& e) {
        string message = e.what();
        check(message.find("Could not open") != string::npos, "error should say it could not open");
        check(message.find("does-not-exist.json") != string::npos, "error should name the file");
    }

}

int main(int argc, char** argv) {
    if (argc > 1) dataDir = argv[1];

    testPrimitives();
    testNested();
    testWhitespace();
    testInvalid();
    testStrings();
    testLookup();
    testFiles();

    cout << "\n--------------------\n";
    cout << "passed: " << passed << "\n";
    cout << "failed: " << failed << "\n";

    return failed == 0 ? 0 : 1;
}
