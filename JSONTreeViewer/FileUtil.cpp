#include "FileUtil.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

bool ReadFileToWString(const wchar_t* fileName, std::wstring& out) {
    HANDLE hFile = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD size = GetFileSize(hFile, nullptr);
    if (size == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return false;
    }

    if (size == 0) {
        out.clear();
        CloseHandle(hFile);
        return true;
    }

    std::vector<BYTE> buffer(size);
    DWORD read = 0;
    bool ok = ReadFile(hFile, buffer.data(), size, &read, nullptr) && read == size;
    CloseHandle(hFile);
    if (!ok) return false;

    if (size >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE) {
        size_t charCount = (size - 2) / sizeof(wchar_t);
        out.assign(reinterpret_cast<wchar_t*>(buffer.data() + 2), charCount);
        return true;
    }
    if (size >= 2 && buffer[0] == 0xFE && buffer[1] == 0xFF) {
        if ((size - 2) % 2 != 0) return false;
        out.clear();
        out.reserve((size - 2) / 2);
        for (DWORD i = 2; i + 1 < size; i += 2) {
            wchar_t wc = static_cast<wchar_t>((buffer[i] << 8) | buffer[i + 1]);
            out.push_back(wc);
        }
        return true;
    }

    size_t offset = 0;
    if (size >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) {
        offset = 3;
    }
    int chars = MultiByteToWideChar(CP_UTF8, 0,
        reinterpret_cast<LPCCH>(buffer.data() + offset), static_cast<int>(size - offset),
        nullptr, 0);
    if (chars <= 0) return false;
    out.resize(chars);
    MultiByteToWideChar(CP_UTF8, 0,
        reinterpret_cast<LPCCH>(buffer.data() + offset), static_cast<int>(size - offset),
        &out[0], chars);
    return true;
}

bool WriteWStringToUtf8File(const wchar_t* fileName, const std::wstring& text) {
    int utf8Len = 0;
    if (!text.empty()) {
        utf8Len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (utf8Len <= 0) return false;
    }
    std::vector<char> buffer(utf8Len);
    if (utf8Len > 0) {
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), buffer.data(), utf8Len, nullptr, nullptr);
    }

    HANDLE hFile = CreateFileW(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = true;
    if (utf8Len > 0) {
        ok = WriteFile(hFile, buffer.data(), static_cast<DWORD>(utf8Len), &written, nullptr) && written == utf8Len;
    }
    CloseHandle(hFile);
    return ok;
}
