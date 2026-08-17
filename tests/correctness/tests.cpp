#include <iostream>

#include "query-engine.h"
#include "file-scanner.h"
#include "session.h"
#include "version.h"

int passed = 0;
int failed = 0;

string dataDir = "tests/data";

void section(const string& name) {
    cout << "\n" << name << "\n";
}

void pass(const string& label) {
    passed++;
    cout << "  ✓ PASS  " << label << "\n";
}

void fail(const string& label) {
    failed++;
    cout << "  ✗ FAIL  " << label << "\n";
}

void check(bool ok, const string& label) {
    if (ok) {
        pass(label);
    } else {
        fail(label);
    }
}

void checkScannerToken(FileScanner& scanner, JSONTokenType type, const string& value, const string& name) {
    Token token = scanner.scan();
    check(token.type == type && token.value == value, name);
}

void checkScannerThrows(const string& input, const string& name) {
    bool threw = false;

    try {
        FileScanner scanner(input, ParserType::JSON);
        scanner.scan();
    } catch (const exception&) {
        threw = true;
    }

    check(threw, name);
}

void testScanner() {
    section("scanner");

    FileScanner scanner(R"({}[]:,"name" true false null -12.5)", ParserType::JSON);
    checkScannerToken(scanner, JSONTokenType::LBrace, "{", "left brace");
    checkScannerToken(scanner, JSONTokenType::RBrace, "}", "right brace");
    checkScannerToken(scanner, JSONTokenType::LBracket, "[", "left bracket");
    checkScannerToken(scanner, JSONTokenType::RBracket, "]", "right bracket");
    checkScannerToken(scanner, JSONTokenType::Colon, ":", "colon");
    checkScannerToken(scanner, JSONTokenType::Comma, ",", "comma");
    checkScannerToken(scanner, JSONTokenType::String, "name", "string");
    checkScannerToken(scanner, JSONTokenType::Boolean, "true", "true");
    checkScannerToken(scanner, JSONTokenType::Boolean, "false", "false");
    checkScannerToken(scanner, JSONTokenType::Null, "null", "null");
    checkScannerToken(scanner, JSONTokenType::Number, "-12.5", "number");
    checkScannerToken(scanner, JSONTokenType::End, "", "end");

    FileScanner whitespaceScanner("\n\n true", ParserType::JSON);
    checkScannerToken(whitespaceScanner, JSONTokenType::Boolean, "true", "whitespace");
    check(whitespaceScanner.getLineNumber() == 3, "line number");

    checkScannerThrows("@", "invalid character");
    checkScannerThrows("\"hello", "unterminated string");
}

string parseAndFormat(const string& text) {
    FileParser parser(text, ParserType::JSON);
    JSONValue value = parser.parse();
    LookupResult whole = {true, &value, ""};
    return formatResult(whole);
}

bool sameOutput(const string& got, const string& want) {
    if (got.rfind("Error:", 0) == 0 || want.rfind("Error:", 0) == 0) {
        return got == want;
    }
    try {
        return parseAndFormat(got) == parseAndFormat(want);
    } catch (const exception&) {
        return got == want;
    }
}

void checkParse(const string& text, const string& want) {
    string got = parseAndFormat(text);
    if (sameOutput(got, want)) {
        pass("parse " + text);
    } else {
        fail("parse " + text);
        cout << "         got  " << got << "\n         want " << want << "\n";
    }
}

bool parses(const string& text) {
    try {
        FileParser parser(text, ParserType::JSON);
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

void checkLookup(const string& text, const string& path, const string& want) {
    FileParser parser(text, ParserType::JSON);
    Session session;
    session.initialize(parser.parse(), "test");
    string got = excecuteQuery(session, path);

    if (sameOutput(got, want)) {
        pass(path);
    } else {
        fail(path + " on " + text);
        cout << "         got  " << got
             << "\n         want " << want << "\n";
    }
}

void checkQuery(const string& text, const string& query, const string& want) {
    try {
        FileParser parser(text, ParserType::JSON);
        Session session;
        session.initialize(parser.parse(), "test");
        string got = excecuteQuery(session, query);
        if (sameOutput(got, want)) {
            pass(query);
        } else {
            fail(query);
            cout << "         got  " << got
                 << "\n         want " << want << "\n";
        }
    } catch (const exception& e) {
        fail(query);
        cout << "         " << e.what() << "\n";
    }
}

// -1 means readFile threw.
int recordCount(const string& file) {
    try {
        //return (int)readFile(dataDir + "/" + file).size();
        Session s;
        uploadFile(dataDir + "/" + file, s);

        return s.records;
    } catch (const exception&) {
        return -1;
    }
}

void checkCount(const string& file, int want) {
    int got = recordCount(file);
    if (got == want) {
        pass(file + " record count");
    } else {
        fail(file + " record count");
        cout << "         got  " << got << "\n         want " << want << "\n";
    }
}

void checkField(const string& file, size_t index, const string& path, const string& want) {
    //vector<JSONValue> records = readFile(dataDir + "/" + file);
    //string got = index < records.size() ? formatResult(get(records[index], path)) : "<no record>";
    Session s;
    uploadFile(dataDir + "/" + file, s);
    string got = index < s.records ? excecuteQuery(s, path) : "<no record>";

    if (got == want) {
        pass(file + "[" + to_string(index) + "] " + path);
    } else {
        fail(file + "[" + to_string(index) + "] " + path);
        cout << "         got  " << got
             << "\n         want " << want << "\n";
    }
}

void testVersion() {
    section("version");
    check(getVersionId() == "week3-v1", "version ID");
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
}

void testNested() {
    section("empty objects and arrays");
    checkParse("{}", "{}");
    checkParse("[]", "[]");

    section("nested objects");
    checkParse(R"({"a":{"b":"c"}})", R"({"a":{"b":"c"}})");
    checkParse(R"({"a":{"b":{"c":{"d":1}}}})", R"({"a":{"b":{"c":{"d":1}}}})");
    checkParse(R"({"a":1,"b":2,"c":3})", R"({"a":1,"b":2,"c":3})");

    section("nested arrays");
    checkParse("[1,2,3]", "[1,2,3]");
    checkParse("[[1,2],[3,4]]", "[[1,2],[3,4]]");

    section("arrays and objects mixed together");
    checkParse(R"([1,"two",true,null])", R"([1,"two",true,null])");
    checkParse(R"([{"k":"v"},[1,2],3])", R"([{"k":"v"},[1,2],3])");
}

void testWhitespace() {
    section("whitespace");
    checkValid("  {\"a\":1}  ");
    checkValid("{\n\"a\":1}");
    checkValid("{\t\"a\" :1}");

}

void testInvalid() {
    section("bad JSON gets rejected");
    //checkInvalid("");
    checkInvalid("{");
    checkInvalid(R"({"a":1)");
    checkInvalid(R"({"a" 1})");
    checkInvalid(R"({"a":})");
    checkInvalid(R"({a:1})");
    checkInvalid(R"({"a":"oops)");
    checkInvalid(R"({"a":01})");
    checkInvalid("[1,2");
    checkInvalid("nul");

    
    section("error messages");
    try {
        FileParser parser(R"({"a" 1})", ParserType::JSON);
        parser.parse();
        check(false, "missing colon should have thrown");
    } catch (const exception& e) {
        string message = e.what();
        check(message.find("line 1") != string::npos, "JSON error should say the line number");
    }
    try {
        FileParser parser(R"({"a" 1})", ParserType::JSONL);
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
    checkLookup(R"({"a":"x\ny"})", R"(GET("a"))", R"("x\ny")");

    section("duplicate keys");
    checkParse(R"({"a":1,"a":2})", R"({"a":1,"a":2})");
    checkLookup(R"({"a":1,"a":2})", R"(GET("a"))", "1");

}

void testLookup() {
    const string student =
        R"({"student":{"name":"Ryan","scores":[90,85],"active":true,"gpa":3.75,"advisor":null}})";
    const string roster =
        R"({"student":[{"name":"Ryan","scores":[90,85]},{"name":"Javier","scores":[95,88]},{"name":"Nobody"}]})";

    section("looking up fields");
    checkLookup(student, R"(GET("student", "name"))", "\"Ryan\"");
    checkLookup(student, R"(GET("student", "active"))", "true");
    checkLookup(student, R"(GET("student", "gpa"))", "3.75");
    checkLookup(student, R"(GET("student", "advisor"))", "null");
    checkLookup(student, R"(GET("student", "scores"))", "[90, 85]");

    section("nested paths and array indexes");
    checkLookup(student, R"(GET("student", "scores", 1))", "85");
    checkLookup(roster, R"(GET("student", 1, "name"))", "\"Javier\"");
    checkLookup(roster, R"(GET("student", 1, "scores", 1))", "88");
    checkLookup(R"([{"x":1},{"x":2}])", R"(GET(1, "x"))", "2");
    checkLookup(R"([[1,2],[3,4]])", R"(GET(1, 0))", "3");
    checkLookup(R"({"a":{"b":{"c":{"d":"deep"}}}})", R"(GET("a", "b", "c", "d"))", "\"deep\"");

    section("empty path gives back the whole record");
    checkLookup(R"({"a":1})", R"(GET())", R"({"a":1})");
    checkLookup(student, R"(GET("student"))",
                R"({"name":"Ryan","scores":[90,85],"active":true,"gpa":3.75,"advisor":null})");

    section("missing fields");
    checkLookup(student, R"(GET("student", "missing"))", "Error: field 'missing' not found");
    checkLookup(roster, R"(GET("student", 2, "scores"))", "Error: field 'scores' not found");
    checkLookup(R"({"a":1})", R"(GET("a", "b"))", "Error: cannot look up 'b' on a non-object, non-array value");

    FileParser parser(R"({"a":1})", ParserType::JSON);
    JSONValue root = parser.parse();
    LookupResult missing = get(root, {"missing"});
    check(!missing.ok && missing.value == nullptr, "missing field result");
    LookupResult found = get(root, {"a"});
    check(found.ok && found.value != nullptr, "found field result");

    section("bad array indexes");
    checkLookup(student, R"(GET("student", "scores", 2))", "Error: index 2 out of range");
    checkLookup(student, R"(GET("student", "name", 0))", "Error: cannot look up '0' on a non-object, non-array value");

    section("bad paths");
    checkLookup(student, R"(GET(" student"))", "Error: field ' student' not found");
    checkLookup(R"({"":5})", R"(GET(""))", "5");

    section("how results get printed");
    check(formatResult({false, nullptr, "something went wrong"}) == "Error: something went wrong",
          "errors print with an Error: prefix");
}

void testFiles() {
    section("empty files");
    checkCount("empty.jsonl", 0);
    checkCount("empty.json", 0);
    checkCount("blank-lines.jsonl", 0);

    section("jsonl files with several records");
    checkCount("records.jsonl", 3);
    checkField("records.jsonl", 0, R"(GET(0, "name"))", "\"Ryan\"");
    checkField("records.jsonl", 2, R"(GET(2, "name"))", "\"Jules\"");
    checkField("students.jsonl", 0, R"(GET(0, "student", "name"))", "\"Ryan\"");

    section("blank lines in the middle of a file");
    checkCount("gaps.jsonl", 3);
    checkField("gaps.jsonl", 1, R"(GET(1, "id"))", "2");

    section("json files");
    checkCount("nested.json", 1);
    checkField("nested.json", 0, R"(GET("student", "advisor", "name"))", "\"Dr. Smith\"");
    checkCount("scalar.json", 1);

    section("a top level array becomes one record per element");
    checkCount("array.json", 3);
    checkField("array.json", 0, R"(GET(0, "city"))", "\"Riverside\"");
    checkCount("mixed.json", 6);
    checkField("mixed.json", 1, R"(GET(1))", "\"two\"");
    checkField("mixed.json", 4, R"(GET(4, "k"))", "\"v\"");

    /*section("invalid file");
    checkCount("invalid.json", 0);*/

    section("file extensions");
    checkCount("upper.JSONL", 2);
    checkCount("noextension", 1);

    section("missing file");
    try {
        Session s;
        uploadFile(dataDir + "/does-not-exist.json", s);
        check(false, "a missing file should have thrown");
    } catch (const exception& e) {
        string message = e.what();
        check(message.find("Could not open") != string::npos, "error should say it could not open");
        check(message.find("does-not-exist.json") != string::npos, "error should name the file");
    }

}

void testQueries() {
    const string records =
        R"([{"id":1,"city":"Riverside","price":4,"active":true,"profile":{"name":"A"}},{"id":2,"city":"Riverside","price":2,"active":false,"profile":{"name":"B"}},{"id":3,"city":"Los Angeles","price":9,"active":true,"profile":{"name":"C"}}])";

    section("query execution");
    checkQuery(records, R"(GET(0,"profile","name"))", R"("A")");
    checkQuery(records, R"(FILTER(GET("price") > 3))",
               R"([{"id":1,"city":"Riverside","price":4,"active":true,"profile":{"name":"A"}},{"id":3,"city":"Los Angeles","price":9,"active":true,"profile":{"name":"C"}}])");
    checkQuery(records, R"(SORT(GET("price"), ASC))",
               R"([{"id":2,"city":"Riverside","price":2,"active":false,"profile":{"name":"B"}},{"id":1,"city":"Riverside","price":4,"active":true,"profile":{"name":"A"}},{"id":3,"city":"Los Angeles","price":9,"active":true,"profile":{"name":"C"}}])");
    checkQuery(records, "LIMIT(2)",
               R"([{"id":1,"city":"Riverside","price":4,"active":true,"profile":{"name":"A"}},{"id":2,"city":"Riverside","price":2,"active":false,"profile":{"name":"B"}}])");
    checkQuery(records, R"(FILTER(GET("active") == true) | SORT(GET("price"), DESC) | LIMIT(1))",
               R"([{"id":3,"city":"Los Angeles","price":9,"active":true,"profile":{"name":"C"}}])");
    checkQuery(records, R"(GROUPBY(GET("city")))",
               R"({"Riverside":[{"id":1,"city":"Riverside","price":4,"active":true,"profile":{"name":"A"}},{"id":2,"city":"Riverside","price":2,"active":false,"profile":{"name":"B"}}],"Los Angeles":[{"id":3,"city":"Los Angeles","price":9,"active":true,"profile":{"name":"C"}}]})");
    checkQuery(records, R"(AVERAGE(GET("price")))", "5");
    checkQuery(records, R"(GROUPBY(GET("city")) | AVERAGE(GET("price")))",
               R"({"Riverside":3,"Los Angeles":9})");
}

int main(int argc, char** argv) {
    if (argc > 1) dataDir = argv[1];

    testScanner();
    testPrimitives();
    testNested();
    testWhitespace();
    testInvalid();
    testStrings();
    testLookup();
    testFiles();
    testQueries();
    testVersion();

    const int total = passed + failed;
    cout << "\n══════════ TEST RESULTS ══════════\n";
    cout << (failed == 0 ? "✓ PASS" : "✗ FAIL") << "\n";
    cout << passed << " passed · " << failed << " failed · " << total << " total\n";
    cout << "════════════════════════════════════\n";

    return failed == 0 ? 0 : 1;
}
