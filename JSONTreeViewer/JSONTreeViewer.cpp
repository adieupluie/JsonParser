#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "Comctl32.lib")

static const wchar_t CLASS_NAME[] = L"JsonTreeViewerWindow";
static HWND g_hEdit = nullptr;
static HWND g_hTree = nullptr;
static HWND g_hStatus = nullptr;

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

class JsonParser {
public:
    JsonParser(const std::wstring& input) : data(input), pos(0), line(1), column(1) {}

    JsonNode parse(ParseError& error) {
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

private:
    const std::wstring& data;
    size_t pos;
    int line;
    int column;

    bool parseValue(JsonNode& node, ParseError& error) {
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

    bool parseObject(JsonNode& node, ParseError& error) {
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

    bool parseArray(JsonNode& node, ParseError& error) {
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

    bool parseString(std::wstring& out, ParseError& error) {
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
                    if (pos + 4 > data.size()) {
                        setError(error, L"Invalid Unicode escape in string");
                        return false;
                    }
                    wchar_t buf[5] = {0};
                    for (int i = 0; i < 4; ++i) buf[i] = data[pos++];
                    advancePosition(buf[0]); // approximate
                    unsigned int code = 0;
                    swscanf_s(buf, L"%04x", &code);
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

    bool parseNumber(JsonNode& node, ParseError& error) {
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

    bool parseLiteral(JsonNode& node, ParseError& error) {
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

    bool consume(wchar_t expected, ParseError& error) {
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

    wchar_t peek() const {
        return pos < data.size() ? data[pos] : L'\0';
    }

    void advance() {
        if (pos < data.size()) {
            advancePosition(data[pos]);
            pos++;
        }
    }

    void advancePosition(wchar_t ch) {
        if (ch == L'\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }

    void skipWhitespace() {
        while (pos < data.size() && iswspace(data[pos])) {
            advance();
        }
    }

    bool match(const wchar_t* expected) {
        size_t len = wcslen(expected);
        if (data.substr(pos, len) == expected) {
            for (size_t i = 0; i < len; ++i) advance();
            return true;
        }
        return false;
    }

    void setError(ParseError& error, const std::wstring& message) {
        if (!error.hasError) {
            error.hasError = true;
            error.message = message;
            error.line = line;
            error.column = column;
        }
    }
};

void AddTreeNode(HWND hTree, HTREEITEM hParent, const JsonNode& node) {
    TVINSERTSTRUCT tvis = {};
    std::wstring label = node.name.empty() ? L"(root)" : node.name;
    if (!node.type.empty()) {
        label += L" : ";
        label += node.type;
    }
    if (!node.value.empty() && node.children.empty()) {
        label += L" = ";
        label += node.value;
    }
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT;
    tvis.item.pszText = const_cast<LPWSTR>(label.c_str());
    HTREEITEM hItem = (HTREEITEM)SendMessage(hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
    for (const JsonNode& child : node.children) {
        AddTreeNode(hTree, hItem, child);
    }
}

void SetStatusText(const std::wstring& text) {
    SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)text.c_str());
}

bool ParseAndDisplayJson(HWND hwnd) {
    int length = GetWindowTextLengthW(g_hEdit);
    std::wstring text(length, L' ');
    GetWindowTextW(g_hEdit, &text[0], length + 1);
    ParseError error;
    JsonParser parser(text);
    JsonNode root = parser.parse(error);
    SendMessage(g_hTree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
    if (error.hasError) {
        SetStatusText(L"Parse failed: " + error.message + L" (line " + std::to_wstring(error.line) + L", col " + std::to_wstring(error.column) + L")");
        MessageBoxW(hwnd, error.message.c_str(), L"JSON Parse Error", MB_ICONERROR | MB_OK);
        return false;
    }
    AddTreeNode(g_hTree, TVI_ROOT, root);
    SetStatusText(L"Parsed successfully");
    return true;
}

void DoOpenFile(HWND hwnd) {
    OPENFILENAME ofn = {};
    wchar_t fileName[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Open JSON File";
    if (GetOpenFileNameW(&ofn)) {
        HANDLE hFile = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return;
        DWORD size = GetFileSize(hFile, nullptr);
        std::wstring buffer(size / sizeof(wchar_t), L' ');
        DWORD read = 0;
        if (ReadFile(hFile, &buffer[0], size, &read, nullptr) && read > 0) {
            SetWindowTextW(g_hEdit, buffer.c_str());
        }
        CloseHandle(hFile);
    }
}

void DoSaveFile(HWND hwnd) {
    OPENFILENAME ofn = {};
    wchar_t fileName[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Save JSON File";
    if (GetSaveFileNameW(&ofn)) {
        int length = GetWindowTextLengthW(g_hEdit);
        std::wstring text(length, L' ');
        GetWindowTextW(g_hEdit, &text[0], length + 1);
        HANDLE hFile = CreateFileW(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return;
        DWORD written = 0;
        WriteFile(hFile, text.c_str(), static_cast<DWORD>(text.size() * sizeof(wchar_t)), &written, nullptr);
        CloseHandle(hFile);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icex = {};
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_WIN95_CLASSES;
        InitCommonControlsEx(&icex);

        g_hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
            0, 0, 0, 0, hwnd, nullptr, nullptr, nullptr);

        g_hTree = CreateWindowExW(0, WC_TREEVIEW, nullptr,
            WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES |
            TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
            0, 0, 0, 0, hwnd, nullptr, nullptr, nullptr);

        g_hStatus = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hwnd, nullptr, nullptr, nullptr);

        HMENU hMenu = CreateMenu();
        HMENU hFileMenu = CreatePopupMenu();
        AppendMenuW(hFileMenu, MF_STRING, 101, L"Open...\tCtrl+O");
        AppendMenuW(hFileMenu, MF_STRING, 102, L"Save...\tCtrl+S");
        AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hFileMenu, MF_STRING, 103, L"Exit");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");

        HMENU hActionMenu = CreatePopupMenu();
        AppendMenuW(hActionMenu, MF_STRING, 201, L"Parse JSON\tCtrl+P");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hActionMenu, L"Action");
        SetMenu(hwnd, hMenu);

        SetStatusText(L"Ready");
        break;
    }
    case WM_SIZE: {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        int statusHeight = 20;
        int width = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top - statusHeight;
        int editWidth = width / 2;
        int treeWidth = width - editWidth;
        MoveWindow(g_hEdit, 0, 0, editWidth, height, TRUE);
        MoveWindow(g_hTree, editWidth, 0, treeWidth, height, TRUE);
        SendMessage(g_hStatus, WM_SIZE, 0, 0);
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 101: DoOpenFile(hwnd); break;
        case 102: DoSaveFile(hwnd); break;
        case 103: PostMessage(hwnd, WM_CLOSE, 0, 0); break;
        case 201: ParseAndDisplayJson(hwnd); break;
        }
        break;
    }
    case WM_KEYDOWN: {
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
            if (wParam == 'P') {
                ParseAndDisplayJson(hwnd);
                return 0;
            }
            if (wParam == 'O') {
                DoOpenFile(hwnd);
                return 0;
            }
            if (wParam == 'S') {
                DoSaveFile(hwnd);
                return 0;
            }
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register window class", L"Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"JSON Tree Viewer",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) {
        MessageBoxW(nullptr, L"Failed to create main window", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}
