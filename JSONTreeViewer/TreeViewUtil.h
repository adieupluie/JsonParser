#pragma once

#include "JsonModel.h"

#include <windows.h>
#include <commctrl.h>
#include <string>

void AddTreeNode(HWND hTree, HTREEITEM hParent, const JsonNode& node);
void ExpandTree(HWND hTree, HTREEITEM hItem, UINT code);
bool SelectNextTreeMatch(HWND hTree, const std::wstring& query);
