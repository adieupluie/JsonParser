#include "TreeViewUtil.h"

static std::wstring ToLower(std::wstring text) {
    for (wchar_t& ch : text) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    return text;
}

static std::wstring GetTreeItemText(HWND hTree, HTREEITEM hItem) {
    wchar_t buffer[1024] = {};
    TVITEMW item = {};
    item.mask = TVIF_TEXT;
    item.hItem = hItem;
    item.pszText = buffer;
    item.cchTextMax = ARRAYSIZE(buffer);
    TreeView_GetItem(hTree, &item);
    return buffer;
}

static HTREEITEM GetNextVisibleItem(HWND hTree, HTREEITEM hItem) {
    HTREEITEM next = TreeView_GetNextSibling(hTree, hItem);
    if (next) return next;

    HTREEITEM parent = TreeView_GetParent(hTree, hItem);
    while (parent) {
        next = TreeView_GetNextSibling(hTree, parent);
        if (next) return next;
        parent = TreeView_GetParent(hTree, parent);
    }
    return nullptr;
}

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
    HTREEITEM hItem = TreeView_InsertItem(hTree, &tvis);
    for (const JsonNode& child : node.children) {
        AddTreeNode(hTree, hItem, child);
    }
}

void ExpandTree(HWND hTree, HTREEITEM hItem, UINT code) {
    if (!hItem) {
        hItem = TreeView_GetRoot(hTree);
    }
    while (hItem) {
        TreeView_Expand(hTree, hItem, code);
        HTREEITEM child = TreeView_GetChild(hTree, hItem);
        if (child) {
            ExpandTree(hTree, child, code);
        }
        hItem = TreeView_GetNextSibling(hTree, hItem);
    }
}

bool SelectNextTreeMatch(HWND hTree, const std::wstring& query) {
    if (query.empty()) return false;

    std::wstring needle = ToLower(query);
    HTREEITEM root = TreeView_GetRoot(hTree);
    if (!root) return false;

    HTREEITEM selected = TreeView_GetSelection(hTree);
    HTREEITEM item = selected ? GetNextVisibleItem(hTree, selected) : root;
    if (!item) item = root;

    HTREEITEM start = item;
    do {
        std::wstring text = ToLower(GetTreeItemText(hTree, item));
        if (text.find(needle) != std::wstring::npos) {
            TreeView_SelectItem(hTree, item);
            TreeView_EnsureVisible(hTree, item);
            return true;
        }
        item = GetNextVisibleItem(hTree, item);
        if (!item) item = root;
    } while (item && item != start);

    return false;
}
