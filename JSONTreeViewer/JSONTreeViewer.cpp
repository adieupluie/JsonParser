#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>

#include "FileUtil.h"
#include "JsonParser.h"
#include "Resource.h"
#include "TreeViewUtil.h"

#pragma comment(lib, "Comctl32.lib")

static const wchar_t CLASS_NAME[] = L"JsonTreeViewerWindow";
static const int SPLITTER_WIDTH = 6;
static const int MIN_PANE_WIDTH = 120;

enum CommandId {
    CMD_OPEN = 101,
    CMD_SAVE = 102,
    CMD_EXIT = 103,
    CMD_PARSE = 201,
    CMD_FIND = 202,
    CMD_EXPAND_ALL = 203,
    CMD_COLLAPSE_ALL = 204,
    CMD_SELECT_ALL = 301,
    CMD_GO_TO_ERROR = 302,
};

static HWND g_hEdit = nullptr;
static HWND g_hTree = nullptr;
static HWND g_hStatus = nullptr;
static HWND g_hSearchEdit = nullptr;
static HACCEL g_hAccel = nullptr;
static int g_splitX = -1;
static bool g_isDraggingSplitter = false;
static bool g_searchVisible = false;
static bool g_hasLastError = false;
static int g_lastErrorLine = 1;
static int g_lastErrorColumn = 1;
static std::wstring g_lastSearch;

static void SetStatusText(const std::wstring& text) {
    SendMessageW(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(text.c_str()));
}

static std::wstring GetCurrentTimeText() {
    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t buffer[32];
    swprintf_s(buffer, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    return buffer;
}

static std::wstring GetEditText() {
    int length = GetWindowTextLengthW(g_hEdit);
    std::wstring text(length + 1, L'\0');
    if (length > 0) {
        GetWindowTextW(g_hEdit, &text[0], static_cast<int>(text.size()));
    }
    text.resize(length);
    return text;
}

static int GetTextOffsetForLineColumn(const std::wstring& text, int line, int column) {
    int currentLine = 1;
    int currentColumn = 1;
    for (size_t i = 0; i < text.size(); ++i) {
        if (currentLine == line && currentColumn == column) {
            return static_cast<int>(i);
        }
        if (text[i] == L'\n') {
            ++currentLine;
            currentColumn = 1;
        } else {
            ++currentColumn;
        }
    }
    return static_cast<int>(text.size());
}

static void GoToLastError() {
    if (!g_hasLastError) {
        SetStatusText(L"No parse error to navigate to");
        return;
    }

    std::wstring text = GetEditText();
    int offset = GetTextOffsetForLineColumn(text, g_lastErrorLine, g_lastErrorColumn);
    SetFocus(g_hEdit);
    SendMessageW(g_hEdit, EM_SETSEL, offset, offset);
    SendMessageW(g_hEdit, EM_SCROLLCARET, 0, 0);
    SetStatusText(L"Moved to last error at line " + std::to_wstring(g_lastErrorLine) +
        L", col " + std::to_wstring(g_lastErrorColumn));
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
    int searchHeight = g_searchVisible ? 28 : 0;
    int height = rcClient.bottom - rcClient.top - statusHeight - searchHeight;
    if (g_splitX < 0) {
        g_splitX = ClampSplitX((width - SPLITTER_WIDTH) / 2, width);
    }
    g_splitX = ClampSplitX(g_splitX, width);
    return RECT{ g_splitX, searchHeight, g_splitX + SPLITTER_WIDTH, searchHeight + max(0, height) };
}

static void LayoutChildren(HWND hwnd) {
    if (!g_hEdit || !g_hTree || !g_hStatus) return;

    SendMessageW(g_hStatus, WM_SIZE, 0, 0);

    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    RECT rcStatus = {};
    GetWindowRect(g_hStatus, &rcStatus);

    int width = rcClient.right - rcClient.left;
    int statusHeight = rcStatus.bottom - rcStatus.top;
    int searchHeight = g_searchVisible ? 28 : 0;
    int height = max(0, rcClient.bottom - rcClient.top - statusHeight - searchHeight);
    if (g_splitX < 0) {
        g_splitX = (width - SPLITTER_WIDTH) / 2;
    }
    g_splitX = ClampSplitX(g_splitX, width);

    int treeX = g_splitX + SPLITTER_WIDTH;
    ShowWindow(g_hSearchEdit, g_searchVisible ? SW_SHOW : SW_HIDE);
    MoveWindow(g_hSearchEdit, 0, 0, width, searchHeight, TRUE);
    MoveWindow(g_hEdit, 0, searchHeight, g_splitX, height, TRUE);
    MoveWindow(g_hTree, treeX, searchHeight, max(0, width - treeX), height, TRUE);

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

static bool FindNextInEditor(const std::wstring& query) {
    std::wstring text = GetEditText();
    if (query.empty() || text.empty()) return false;

    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(g_hEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));

    size_t found = text.find(query, max(start, end));
    if (found == std::wstring::npos) {
        found = text.find(query);
    }
    if (found == std::wstring::npos) return false;

    SetFocus(g_hEdit);
    SendMessageW(g_hEdit, EM_SETSEL, static_cast<WPARAM>(found), static_cast<LPARAM>(found + query.size()));
    SendMessageW(g_hEdit, EM_SCROLLCARET, 0, 0);
    return true;
}

static void DoFindNext() {
    int length = GetWindowTextLengthW(g_hSearchEdit);
    std::wstring query(length + 1, L'\0');
    if (length > 0) {
        GetWindowTextW(g_hSearchEdit, &query[0], static_cast<int>(query.size()));
    }
    query.resize(length);

    if (query.empty()) {
        SetStatusText(L"Type a search term");
        return;
    }
    g_lastSearch = query;

    if (FindNextInEditor(query)) {
        SetStatusText(L"Found in editor: " + query);
        return;
    }
    if (SelectNextTreeMatch(g_hTree, query)) {
        SetFocus(g_hTree);
        SetStatusText(L"Found in tree: " + query);
        return;
    }
    SetStatusText(L"No matches: " + query);
}

static void ShowFindBar(HWND hwnd) {
    if (!g_searchVisible) {
        g_searchVisible = true;
        LayoutChildren(hwnd);
    }
    SetFocus(g_hSearchEdit);
    SendMessageW(g_hSearchEdit, EM_SETSEL, 0, -1);
}

static void DoFind(HWND hwnd) {
    if (g_searchVisible && GetFocus() == g_hSearchEdit) {
        DoFindNext();
        return;
    }
    ShowFindBar(hwnd);
}

static bool ParseAndDisplayJson(HWND hwnd) {
    ParseError error;
    std::wstring text = GetEditText();
    JsonParser parser(text);
    JsonNode root = parser.parse(error);
    TreeView_DeleteAllItems(g_hTree);
    if (error.hasError) {
        g_hasLastError = true;
        g_lastErrorLine = error.line;
        g_lastErrorColumn = error.column;
        SetStatusText(L"Parse failed: " + error.message + L" (line " +
            std::to_wstring(error.line) + L", col " + std::to_wstring(error.column) + L")");
        MessageBoxW(hwnd, error.message.c_str(), L"JSON Parse Error", MB_ICONERROR | MB_OK);
        return false;
    }
    g_hasLastError = false;
    AddTreeNode(g_hTree, TVI_ROOT, root);
    SetStatusText(L"Parsed successfully - " + GetCurrentTimeText());
    return true;
}

static void DoOpenFile(HWND hwnd) {
    OPENFILENAMEW ofn = {};
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

static void DoSaveFile(HWND hwnd) {
    OPENFILENAMEW ofn = {};
    wchar_t fileName[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Save JSON File";
    if (GetSaveFileNameW(&ofn)) {
        WriteWStringToUtf8File(fileName, GetEditText());
    }
}

static void CreateMainMenu(HWND hwnd) {
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, CMD_OPEN, L"Open...\tCtrl+O");
    AppendMenuW(hFileMenu, MF_STRING, CMD_SAVE, L"Save...\tCtrl+S");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, CMD_EXIT, L"Exit");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"File");

    HMENU hActionMenu = CreatePopupMenu();
    AppendMenuW(hActionMenu, MF_STRING, CMD_PARSE, L"Parse JSON\tCtrl+P");
    AppendMenuW(hActionMenu, MF_STRING, CMD_FIND, L"Find...\tCtrl+F");
    AppendMenuW(hActionMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hActionMenu, MF_STRING, CMD_EXPAND_ALL, L"Expand All\tCtrl+E");
    AppendMenuW(hActionMenu, MF_STRING, CMD_COLLAPSE_ALL, L"Collapse All\tCtrl+Shift+E");
    AppendMenuW(hActionMenu, MF_STRING, CMD_GO_TO_ERROR, L"Go to Error\tF8");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hActionMenu), L"Action");

    SetMenu(hwnd, hMenu);
}

static void HandleCommand(HWND hwnd, int commandId) {
    switch (commandId) {
    case CMD_OPEN: DoOpenFile(hwnd); break;
    case CMD_SAVE: DoSaveFile(hwnd); break;
    case CMD_EXIT: PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
    case CMD_PARSE: ParseAndDisplayJson(hwnd); break;
    case CMD_FIND: DoFind(hwnd); break;
    case CMD_EXPAND_ALL:
        ExpandTree(g_hTree, nullptr, TVE_EXPAND);
        SetStatusText(L"Expanded all nodes");
        break;
    case CMD_COLLAPSE_ALL:
        ExpandTree(g_hTree, nullptr, TVE_COLLAPSE);
        SetStatusText(L"Collapsed all nodes");
        break;
    case CMD_SELECT_ALL:
        if (GetFocus() == g_hEdit) {
            SendMessageW(g_hEdit, EM_SETSEL, 0, -1);
        }
        break;
    case CMD_GO_TO_ERROR:
        GoToLastError();
        break;
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

        g_hSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
            WS_CHILD | ES_AUTOHSCROLL,
            0, 0, 0, 0, hwnd, nullptr, nullptr, nullptr);

        CreateMainMenu(hwnd);
        SetStatusText(L"Ready");
        LayoutChildren(hwnd);
        break;
    }
    case WM_SIZE:
        LayoutChildren(hwnd);
        break;
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
    case WM_MOUSEMOVE:
        if (g_isDraggingSplitter) {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            int width = rcClient.right - rcClient.left;
            g_splitX = ClampSplitX(GET_X_LPARAM(lParam) - SPLITTER_WIDTH / 2, width);
            LayoutChildren(hwnd);
            return 0;
        }
        break;
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
    case WM_COMMAND:
        HandleCommand(hwnd, LOWORD(wParam));
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && g_searchVisible && GetFocus() == g_hSearchEdit) {
            g_searchVisible = false;
            LayoutChildren(hwnd);
            SetFocus(g_hEdit);
            return 0;
        }
        break;
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
        { FVIRTKEY | FCONTROL, 'P', CMD_PARSE },
        { FVIRTKEY | FCONTROL, 'O', CMD_OPEN },
        { FVIRTKEY | FCONTROL, 'S', CMD_SAVE },
        { FVIRTKEY | FCONTROL, 'A', CMD_SELECT_ALL },
        { FVIRTKEY | FCONTROL, 'F', CMD_FIND },
        { FVIRTKEY | FCONTROL | FSHIFT, 'F', CMD_FIND },
        { FVIRTKEY | FCONTROL, 'E', CMD_EXPAND_ALL },
        { FVIRTKEY | FCONTROL | FSHIFT, 'E', CMD_COLLAPSE_ALL },
        { FVIRTKEY, VK_F8, CMD_GO_TO_ERROR },
    };
    g_hAccel = CreateAcceleratorTableW(accel, ARRAYSIZE(accel));

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.hwnd == g_hSearchEdit && msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_RETURN) {
                DoFindNext();
                continue;
            }
            if (msg.wParam == VK_ESCAPE) {
                g_searchVisible = false;
                LayoutChildren(hwnd);
                SetFocus(g_hEdit);
                continue;
            }
        }
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
