#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "Comctl32.lib")

#define IDI_MAINICON 1001

static const wchar_t CLASS_NAME[] = L"JsonTreeViewerWindow";
static const int SPLITTER_WIDTH = 6;
static const int MIN_PANE_WIDTH = 120;
static HWND g_hEdit = nullptr;
static HWND g_hTree = nullptr;
static HWND g_hStatus = nullptr;
static HACCEL g_hAccel = nullptr;
static int g_splitX = -1;
static bool g_isDraggingSplitter = false;

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

    int hexDigitValue(wchar_t ch) const {
        if (ch >= L'0' && ch <= L'9') return ch - L'0';
        if (ch >= L'a' && ch <= L'f') return ch - L'a' + 10;
        if (ch >= L'A' && ch <= L'F') return ch - L'A' + 10;
        return -1;
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

static std::wstring GetCurrentTimeText() {
    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t buffer[32];
    swprintf_s(buffer, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    return buffer;
}

static int ClampSplitX(int splitX, int width) {
    if (width <= (MIN_PANE_WIDTH * 2 + SPLITTER_WIDTH)) {
        return max(0, (width - SPLITTER_WIDTH) / 2);
    }
    int minX = MIN_PANE_WIDTH;
    int maxX = width - MIN_PANE_WIDTH - SPLITTER_WIDTH;
    return min(max(splitX, minX), maxX);
}

static RECT GetSplitterRect(HWND hwnd) {
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    RECT rcStatus = {};
    int statusHeight = 20;
    if (g_hStatus && GetWindowRect(g_hStatus, &rcStatus)) {
        statusHeight = rcStatus.bottom - rcStatus.top;
    }
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top - statusHeight;
    if (g_splitX < 0) {
        g_splitX = ClampSplitX((width - SPLITTER_WIDTH) / 2, width);
    }
    g_splitX = ClampSplitX(g_splitX, width);
    return RECT{ g_splitX, 0, g_splitX + SPLITTER_WIDTH, max(0, height) };
}

static void LayoutChildren(HWND hwnd) {
    if (!g_hEdit || !g_hTree || !g_hStatus) return;

    SendMessage(g_hStatus, WM_SIZE, 0, 0);

    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    RECT rcStatus = {};
    GetWindowRect(g_hStatus, &rcStatus);

    int width = rcClient.right - rcClient.left;
    int statusHeight = rcStatus.bottom - rcStatus.top;
    int height = max(0, rcClient.bottom - rcClient.top - statusHeight);
    if (g_splitX < 0) {
        g_splitX = (width - SPLITTER_WIDTH) / 2;
    }
    g_splitX = ClampSplitX(g_splitX, width);

    int treeX = g_splitX + SPLITTER_WIDTH;
    MoveWindow(g_hEdit, 0, 0, g_splitX, height, TRUE);
    MoveWindow(g_hTree, treeX, 0, max(0, width - treeX), height, TRUE);

    RECT splitter = GetSplitterRect(hwnd);
    InvalidateRect(hwnd, &splitter, TRUE);
}

static void DrawSplitter(HWND hwnd, HDC hdc) {
    RECT splitter = GetSplitterRect(hwnd);
    HBRUSH brush = CreateSolidBrush(RGB(225, 225, 225));
    FillRect(hdc, &splitter, brush);
    DeleteObject(brush);

    int center = splitter.left + SPLITTER_WIDTH / 2;
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    MoveToEx(hdc, center, splitter.top, nullptr);
    LineTo(hdc, center, splitter.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static bool ReadFileToWString(const wchar_t* fileName, std::wstring& out) {
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

static bool WriteWStringToUtf8File(const wchar_t* fileName, const std::wstring& text) {
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

bool ParseAndDisplayJson(HWND hwnd) {
    int length = GetWindowTextLengthW(g_hEdit);
    std::wstring text(length, L' ');
    GetWindowTextW(g_hEdit, &text[0], length + 1);
    ParseError error;
    JsonParser parser(text);
    JsonNode root = parser.parse(error);
    TreeView_DeleteAllItems(g_hTree);
    if (error.hasError) {
        SetStatusText(L"Parse failed: " + error.message + L" (line " + std::to_wstring(error.line) + L", col " + std::to_wstring(error.column) + L")");
        MessageBoxW(hwnd, error.message.c_str(), L"JSON Parse Error", MB_ICONERROR | MB_OK);
        return false;
    }
    AddTreeNode(g_hTree, TVI_ROOT, root);
    SetStatusText(L"Parsed successfully - " + GetCurrentTimeText());
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
        std::wstring content;
        if (ReadFileToWString(fileName, content)) {
            SetWindowTextW(g_hEdit, content.c_str());
        }
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
        WriteWStringToUtf8File(fileName, text);
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
        LayoutChildren(hwnd);
        break;
    }
    case WM_SIZE: {
        LayoutChildren(hwnd);
        break;
    }
    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            RECT splitter = GetSplitterRect(hwnd);
            if (PtInRect(&splitter, pt) || g_isDraggingSplitter) {
                SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                return TRUE;
            }
        }
        break;
    }
    case WM_LBUTTONDOWN: {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT splitter = GetSplitterRect(hwnd);
        if (PtInRect(&splitter, pt)) {
            g_isDraggingSplitter = true;
            SetCapture(hwnd);
            SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
            return 0;
        }
        break;
    }
    case WM_MOUSEMOVE: {
        if (g_isDraggingSplitter) {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            int width = rcClient.right - rcClient.left;
            g_splitX = ClampSplitX(GET_X_LPARAM(lParam) - SPLITTER_WIDTH / 2, width);
            LayoutChildren(hwnd);
            return 0;
        }
        break;
    }
    case WM_LBUTTONUP:
        if (g_isDraggingSplitter) {
            g_isDraggingSplitter = false;
            ReleaseCapture();
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        g_isDraggingSplitter = false;
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawSplitter(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 101: DoOpenFile(hwnd); break;
        case 102: DoSaveFile(hwnd); break;
        case 103: PostMessage(hwnd, WM_CLOSE, 0, 0); break;
        case 201: ParseAndDisplayJson(hwnd); break;
        case 301:
            if (GetFocus() == g_hEdit) {
                SendMessageW(g_hEdit, EM_SETSEL, 0, -1);
            }
            break;
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
            if (wParam == 'A' && GetFocus() == g_hEdit) {
                SendMessageW(g_hEdit, EM_SETSEL, 0, -1);
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
    HICON hIcon = reinterpret_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_MAINICON),
        IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
    HICON hSmallIcon = reinterpret_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_MAINICON),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
    if (!hIcon) hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    if (!hSmallIcon) hSmallIcon = LoadIconW(nullptr, IDI_APPLICATION);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.hIcon = hIcon;
    wc.hIconSm = hSmallIcon;

    if (!RegisterClassExW(&wc)) {
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

    SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hSmallIcon));

    ACCEL accel[] = {
        { FVIRTKEY | FCONTROL, 'P', 201 },
        { FVIRTKEY | FCONTROL, 'O', 101 },
        { FVIRTKEY | FCONTROL, 'S', 102 },
        { FVIRTKEY | FCONTROL, 'A', 301 },
    };
    g_hAccel = CreateAcceleratorTableW(accel, ARRAYSIZE(accel));

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (g_hAccel && TranslateAcceleratorW(hwnd, g_hAccel, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_hAccel) {
        DestroyAcceleratorTable(g_hAccel);
    }

    return static_cast<int>(msg.wParam);
}
