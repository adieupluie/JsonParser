#pragma once

#include <string>
#include <vector>

struct JsonNode {
    std::wstring name;
    std::wstring value;
    std::wstring type;
    std::vector<JsonNode> children;
};

struct ParseError {
    bool hasError = false;
    std::wstring message;
    int line = 1;
    int column = 1;
};
