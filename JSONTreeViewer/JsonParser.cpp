#include "JsonParser.h"

#include <cwchar>
#include <cwctype>

JsonParser::JsonParser(const std::wstring& input) : data(input), pos(0), line(1), column(1) {}

JsonNode JsonParser::parse(ParseError& error) {
    skipWhitespace();
    JsonNode root;
    if (!parseValue(root, error)) {
        return root;
    }
    skipWhitespace();
    if (pos < data.size()) {
        setError(error, L"Unexpected characters after JSON value");
    }
    return root;
}

bool JsonParser::parseValue(JsonNode& node, ParseError& error) {
    skipWhitespace();
    if (pos >= data.size()) {
        setError(error, L"Unexpected end of input");
        return false;
    }
    wchar_t ch = data[pos];
    if (ch == L'{') {
        node.type = L"object";
        return parseObject(node, error);
    }
    if (ch == L'[') {
        node.type = L"array";
        return parseArray(node, error);
    }
    if (ch == L'\"') {
        node.type = L"string";
        return parseString(node.value, error);
    }
    if (ch == L't' || ch == L'f' || ch == L'n') {
        return parseLiteral(node, error);
    }
    return parseNumber(node, error);
}

bool JsonParser::parseObject(JsonNode& node, ParseError& error) {
    if (!consume(L'{', error)) return false;
    skipWhitespace();
    if (peek() == L'}') {
        consume(L'}', error);
        node.value = L"{}";
        return true;
    }
    while (true) {
        skipWhitespace();
        std::wstring key;
        if (!parseString(key, error)) return false;
        skipWhitespace();
        if (!consume(L':', error)) return false;
        JsonNode child;
        child.name = key;
        if (!parseValue(child, error)) return false;
        node.children.push_back(std::move(child));
        skipWhitespace();
        if (peek() == L',') {
            consume(L',', error);
            continue;
        }
        if (peek() == L'}') {
            consume(L'}', error);
            break;
        }
        setError(error, L"Expected ',' or '}' in object");
        return false;
    }
    node.value = L"{" + std::to_wstring(node.children.size()) + L" members}";
    return true;
}

bool JsonParser::parseArray(JsonNode& node, ParseError& error) {
    if (!consume(L'[', error)) return false;
    skipWhitespace();
    if (peek() == L']') {
        consume(L']', error);
        node.value = L"[]";
        return true;
    }
    int index = 0;
    while (true) {
        JsonNode child;
        child.name = std::to_wstring(index++);
        if (!parseValue(child, error)) return false;
        node.children.push_back(std::move(child));
        skipWhitespace();
        if (peek() == L',') {
            consume(L',', error);
            continue;
        }
        if (peek() == L']') {
            consume(L']', error);
            break;
        }
        setError(error, L"Expected ',' or ']' in array");
        return false;
    }
    node.value = L"[" + std::to_wstring(node.children.size()) + L" items]";
    return true;
}

bool JsonParser::parseString(std::wstring& out, ParseError& error) {
    if (!consume(L'"', error)) return false;
    std::wstring ss;
    while (pos < data.size()) {
        wchar_t ch = data[pos++];
        advancePosition(ch);
        if (ch == L'\"') {
            out = ss;
            return true;
        }
        if (ch == L'\\') {
            if (pos >= data.size()) break;
            wchar_t esc = data[pos++];
            advancePosition(esc);
            switch (esc) {
            case L'\"': ss.push_back(L'\"'); break;
            case L'\\': ss.push_back(L'\\'); break;
            case L'/': ss.push_back(L'/'); break;
            case L'b': ss.push_back(L'\b'); break;
            case L'f': ss.push_back(L'\f'); break;
            case L'n': ss.push_back(L'\n'); break;
            case L'r': ss.push_back(L'\r'); break;
            case L't': ss.push_back(L'\t'); break;
            case L'u': {
                unsigned int code = 0;
                for (int i = 0; i < 4; ++i) {
                    if (pos >= data.size()) {
                        setError(error, L"Invalid Unicode escape in string");
                        return false;
                    }
                    wchar_t digit = data[pos++];
                    advancePosition(digit);
                    int value = hexDigitValue(digit);
                    if (value < 0) {
                        setError(error, L"Invalid Unicode escape in string");
                        return false;
                    }
                    code = (code << 4) | value;
                }
                if (code >= 0xD800 && code <= 0xDBFF) {
                    if (pos + 1 < data.size() && data[pos] == L'\\' && data[pos + 1] == L'u') {
                        pos += 2;
                        advancePosition(L'\\');
                        advancePosition(L'u');
                        unsigned int low = 0;
                        for (int i = 0; i < 4; ++i) {
                            if (pos >= data.size()) {
                                setError(error, L"Invalid Unicode surrogate escape in string");
                                return false;
                            }
                            wchar_t digit = data[pos++];
                            advancePosition(digit);
                            int value = hexDigitValue(digit);
                            if (value < 0) {
                                setError(error, L"Invalid Unicode surrogate escape in string");
                                return false;
                            }
                            low = (low << 4) | value;
                        }
                        if (low >= 0xDC00 && low <= 0xDFFF) {
                            ss.push_back(static_cast<wchar_t>(code));
                            ss.push_back(static_cast<wchar_t>(low));
                            break;
                        }
                        setError(error, L"Invalid Unicode surrogate pair in string");
                        return false;
                    }
                    setError(error, L"Invalid Unicode surrogate pair in string");
                    return false;
                }
                ss.push_back(static_cast<wchar_t>(code));
                break;
            }
            default:
                setError(error, L"Invalid escape sequence in string");
                return false;
            }
        } else {
            ss.push_back(ch);
        }
    }
    setError(error, L"Unterminated string literal");
    return false;
}

bool JsonParser::parseNumber(JsonNode& node, ParseError& error) {
    size_t start = pos;
    if (peek() == L'-') advance();
    if (!iswdigit(peek())) {
        setError(error, L"Invalid number format");
        return false;
    }
    if (peek() == L'0') {
        advance();
    } else {
        while (iswdigit(peek())) advance();
    }
    if (peek() == L'.') {
        advance();
        if (!iswdigit(peek())) {
            setError(error, L"Expected digit after decimal point");
            return false;
        }
        while (iswdigit(peek())) advance();
    }
    if (peek() == L'e' || peek() == L'E') {
        advance();
        if (peek() == L'+' || peek() == L'-') advance();
        if (!iswdigit(peek())) {
            setError(error, L"Expected exponent digits");
            return false;
        }
        while (iswdigit(peek())) advance();
    }
    node.type = L"number";
    node.value = data.substr(start, pos - start);
    return true;
}

bool JsonParser::parseLiteral(JsonNode& node, ParseError& error) {
    if (match(L"true")) {
        node.type = L"boolean";
        node.value = L"true";
        return true;
    }
    if (match(L"false")) {
        node.type = L"boolean";
        node.value = L"false";
        return true;
    }
    if (match(L"null")) {
        node.type = L"null";
        node.value = L"null";
        return true;
    }
    setError(error, L"Invalid literal value");
    return false;
}

bool JsonParser::consume(wchar_t expected, ParseError& error) {
    if (peek() == expected) {
        advance();
        return true;
    }
    std::wstring msg = L"Expected '";
    msg += expected;
    msg += L"'";
    setError(error, msg);
    return false;
}

wchar_t JsonParser::peek() const {
    return pos < data.size() ? data[pos] : L'\0';
}

void JsonParser::advance() {
    if (pos < data.size()) {
        advancePosition(data[pos]);
        pos++;
    }
}

void JsonParser::advancePosition(wchar_t ch) {
    if (ch == L'\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
}

void JsonParser::skipWhitespace() {
    while (pos < data.size() && iswspace(data[pos])) {
        advance();
    }
}

bool JsonParser::match(const wchar_t* expected) {
    size_t len = wcslen(expected);
    if (data.substr(pos, len) == expected) {
        for (size_t i = 0; i < len; ++i) advance();
        return true;
    }
    return false;
}

int JsonParser::hexDigitValue(wchar_t ch) const {
    if (ch >= L'0' && ch <= L'9') return ch - L'0';
    if (ch >= L'a' && ch <= L'f') return ch - L'a' + 10;
    if (ch >= L'A' && ch <= L'F') return ch - L'A' + 10;
    return -1;
}

void JsonParser::setError(ParseError& error, const std::wstring& message) {
    if (!error.hasError) {
        error.hasError = true;
        error.message = message;
        error.line = line;
        error.column = column;
    }
}
