#pragma once

#include "JsonModel.h"

#include <string>

class JsonParser {
public:
    explicit JsonParser(const std::wstring& input);

    JsonNode parse(ParseError& error);

private:
    const std::wstring& data;
    size_t pos;
    int line;
    int column;

    bool parseValue(JsonNode& node, ParseError& error);
    bool parseObject(JsonNode& node, ParseError& error);
    bool parseArray(JsonNode& node, ParseError& error);
    bool parseString(std::wstring& out, ParseError& error);
    bool parseNumber(JsonNode& node, ParseError& error);
    bool parseLiteral(JsonNode& node, ParseError& error);
    bool consume(wchar_t expected, ParseError& error);
    wchar_t peek() const;
    void advance();
    void advancePosition(wchar_t ch);
    void skipWhitespace();
    bool match(const wchar_t* expected);
    int hexDigitValue(wchar_t ch) const;
    void setError(ParseError& error, const std::wstring& message);
};
