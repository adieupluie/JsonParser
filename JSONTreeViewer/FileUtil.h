#pragma once

#include <string>

bool ReadFileToWString(const wchar_t* fileName, std::wstring& out);
bool WriteWStringToUtf8File(const wchar_t* fileName, const std::wstring& text);
