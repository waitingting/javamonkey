/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#include "JavaMonkey.h"
#include "utils.h"
#include "string.h"
#include "AccessInfo.h"
#include <exception>
#include "highlighter.h"
#include <memory>
#include <shlobj.h>

#define ID_REFRESH_TREE      200
#define ID_CLEAR_SEARCH      201
#define ID_FOCUS_SEARCH      202
#define ID_COPY_VALUE        203
#define ID_SEARCH_ENTER      204

#define ID_LISTVIEW 101 // 列表视图控件ID
#define ID_SPLITTER 102 // 分割条ID
// 搜索相关控件ID
#define ID_SEARCH_EDIT     103
#define ID_SEARCH_BUTTON   104
#define ID_CLEAR_BUTTON    105
#define ID_NEXT_RESULT         106
#define ID_PREV_RESULT         107
#define ID_STATUSBAR 108  // 状态栏ID
#define IDC_TOOLBAR 109   // 工具栏ID
#define ID_HIGHLIGHT_BUTTON 110   // 工具栏ID
HWND ourHwnd;
HWND topLevelWindow;
int depth = -1;
FILE *logfile;
HMENU popupMenu;

char theMonkeyClassName[] = "MonkeyWin";
char theAccessInfoClassName[] = "AccessInfoWin";

HWND hwndToolbar = NULL; // 工具栏容器
HWND theMonkeyWindow;
HWND theTreeControlWindow;
HWND hwndListView; // 表格控件句柄
HWND hStatusBar;
HWND hwndHighlightButton = NULL; // 高亮按钮
// 分割条相关
HWND hwndSplitter = NULL;
#define MIN_PANE_WIDTH 100
#define SPLITTER_WIDTH 8
int g_splitterPos = 300; // 初始分割位置
bool g_dragging = false;
// 搜索相关控件
HWND hwndSearchEdit = NULL;
HWND hwndSearchButton = NULL;
HWND hwndClearButton = NULL;
HWND hwndPrevButton = NULL;
HWND hwndNextButton = NULL;
HTREEITEM g_hCurrentSearchItem = NULL;
std::vector<HTREEITEM> g_searchResults;
int g_currentResultIndex = -1;

HINSTANCE theInstance;
JavaMonkey *theMonkey;
AccessibleNode *theSelectedNode;
AccessibleNode *thePopupNode;
AccessibleContext theSelectedAccessibleContext;
HWND hwndTV;    // handle of tree-view control 
std::shared_ptr<uia::testing::WindowsHighlighter> highlighter = std::make_shared<uia::testing::WindowsHighlighter>(false);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

    if (logfile == null) {
        logfile = fopen(JAVA_MONKEY_LOG, "w"); // overwrite existing log file
        logString(logfile, "Starting JavaMonkey.exe %s\n", getTimeAndDate());
    }

    theInstance = hInstance;

    // start JavaMonkey
    theMonkey = new JavaMonkey(nCmdShow);

    return 0;
} 

LRESULT CALLBACK AccessInfoWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
JavaMonkey::JavaMonkey(int nCmdShow) {

    HWND hwnd;
    static char szAppName[] = "JavaMonkey";
    static char szMenuName[] = "MONKEYMENU";
    MSG msg;
    WNDCLASSEX wc;

    // JavaMonkey window
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WinProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = theInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = szMenuName;
    wc.lpszClassName = szAppName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&wc);

    // AccessInfo Window
    wc.cbSize = sizeof(WNDCLASSEX);

    wc.hInstance = theInstance;
    wc.lpszClassName = theAccessInfoClassName;
    wc.lpfnWndProc = AccessInfoWindowProc;
    wc.style = 0;

    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    wc.lpszMenuName = "";
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;

    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);

    RegisterClassEx(&wc);

    // 定义加速键表
    ACCEL accels[] = {
        {FVIRTKEY, VK_F3,           ID_SEARCH_ENTER},
        {FVIRTKEY, VK_F5,           ID_REFRESH_TREE},
        {FVIRTKEY, VK_ESCAPE,       ID_CLEAR_SEARCH},
        {FCONTROL, 'F',             ID_FOCUS_SEARCH},
        {FCONTROL, 'C',             ID_COPY_VALUE},
        {FVIRTKEY | FALT, VK_RETURN, ID_SEARCH_ENTER}
    };
    // 创建加速键表句柄
    HACCEL hAccel = CreateAcceleratorTable(accels, ARRAYSIZE(accels));

    BOOL result = initializeAccessBridge();
    // Sleep(100);
    // create the JavaMonkey window
    hwnd = CreateWindow(szAppName,
                        szAppName,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        NULL,
                        NULL,
                        theInstance,
                        NULL);

    ourHwnd = hwnd;

    /* Initialize the common controls. */
    // INITCOMMONCONTROLSEX cc;
    // cc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    // cc.dwICC = ICC_TREEVIEW_CLASSES;
    // InitCommonControlsEx(&cc); 
    ShowWindow(hwnd, nCmdShow);

    UpdateWindow(hwnd);

    if (result != FALSE) {
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (!TranslateAccelerator(hwnd, hAccel, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        DestroyAcceleratorTable(hAccel);
        shutdownAccessBridge();
    }
}

void UpdatePropertyTable(AccessibleNode* node) {
    // 清空现有内容
    ListView_DeleteAllItems(hwndListView);
    if(node == NULL)
    {
        return;
    }
    auto nodeInfo = node->GetNodeInfo();
    if (!nodeInfo) {
        // 错误处理
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.pszText = "Error";
        ListView_InsertItem(hwndListView, &lvi);
        
        lvi.iSubItem = 1;
        lvi.pszText = "Failed to get context info";
        ListView_SetItem(hwndListView, &lvi);
        return;
    }
    
    int index = 0;
    // 辅助函数：添加行
    auto AddRow = [&](const char* prop, const wchar_t* val) {
        char buf[512];
        WideCharToMultiByte(CP_ACP, 0, val, -1, buf, sizeof(buf), NULL, NULL);
        AddTableRow(index++, prop, buf);
    };
    
    auto AddRowInt = [&](const char* prop, int val) {
        char buf[32];
        sprintf_s(buf, "%d", val);
        AddTableRow(index++, prop, buf);
    };

     auto AddRowBool = [&](const char* prop, bool val) {
        AddTableRow(index++, prop, val ? "Yes" : "No");
    };

    // 添加属性行
    AddRow("Name", nodeInfo->name.c_str());
    AddRow("Description", nodeInfo->description.c_str());
    AddRow("Role", nodeInfo->role.c_str());
    AddRow("Role (en_US)", nodeInfo->role_en_US.c_str());
    AddRow("States", nodeInfo->states.c_str());
    AddRow("States (en_US)", nodeInfo->states_en_US.c_str());
    
    AddRowInt("Index In Parent", nodeInfo->indexInParent);
    AddRowInt("Children Count", nodeInfo->childrenCount);
    
    AddRowInt("X", nodeInfo->x);
    AddRowInt("Y", nodeInfo->y);
    AddRowInt("Width", nodeInfo->width);
    AddRowInt("Height", nodeInfo->height);
    
    AddRowBool("Accessible Component", nodeInfo->accessibleComponent);
    AddRowBool("Accessible Action", nodeInfo->accessibleAction);
    AddRowBool("Accessible Selection", nodeInfo->accessibleSelection);
    AddRowBool("Accessible Text", nodeInfo->accessibleText);
    AddRowBool("Accessible Interfaces", nodeInfo->accessibleInterfaces);
    auto str = util::string::ToString(nodeInfo->role_en_US);
    std::replace(str.begin(), str.end(), ' ', '_');
    highlighter->Highlight({nodeInfo->x,nodeInfo->y,nodeInfo->x + nodeInfo->width, nodeInfo->y + nodeInfo->height}, str);
}

void AddTableRow(int rowIndex, const char* property, const wchar_t* value) {
    // 转换宽字符到多字节
    char mbStr[256];
    WideCharToMultiByte(CP_ACP, 0, value, -1, mbStr, sizeof(mbStr), NULL, NULL);
    AddTableRow(rowIndex, property, mbStr);
}

void AddTableRow(int rowIndex, const char* property, const char* value) {
    LVITEM lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = rowIndex;
    lvi.iSubItem = 0;
    lvi.pszText = (char*)property;
    ListView_InsertItem(hwndListView, &lvi);
    
    lvi.iSubItem = 1;
    lvi.pszText = (char*)value;
    ListView_SetItem(hwndListView, &lvi);
}

// 静态控件子类化过程
LRESULT CALLBACK SplitterSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
        case WM_LBUTTONDOWN: {
            g_dragging = true;
            SetCapture(hwnd);
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            if (g_dragging) {
                // 获取鼠标位置
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(GetParent(hwnd), &pt);
                
                // 计算新位置
                int newPos = pt.x - SPLITTER_WIDTH / 2;
                RECT rcParent;
                GetClientRect(GetParent(hwnd), &rcParent);
                
                if (newPos < MIN_PANE_WIDTH) newPos = MIN_PANE_WIDTH;
                if (newPos > rcParent.right - MIN_PANE_WIDTH - SPLITTER_WIDTH) 
                    newPos = rcParent.right - MIN_PANE_WIDTH - SPLITTER_WIDTH;
                
                if (newPos != g_splitterPos) {
                    g_splitterPos = newPos;
                    
                    // 通知父窗口更新布局
                    SendMessage(GetParent(hwnd), WM_SIZE, 0, MAKELPARAM(rcParent.right, rcParent.bottom));
                }
            }
            return 0;
        }
        
        case WM_LBUTTONUP: {
            if (g_dragging) {
                g_dragging = false;
                ReleaseCapture();
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                // SendMessage(GetParent(hwnd), WM_USER + 101, 0, 0);
            }
            return 0;
        }
        
        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                return TRUE;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // 绘制纯色背景（无阴影）
            HBRUSH hBrush = CreateSolidBrush(RGB(220, 220, 220));
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            
            // 绘制简洁边框（无阴影效果）
            HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
            
            // 绘制左右边框线
            MoveToEx(hdc, 0, 0, NULL);
            LineTo(hdc, 0, rc.bottom);
            
            MoveToEx(hdc, rc.right - 1, 0, NULL);
            LineTo(hdc, rc.right - 1, rc.bottom);
            
            // 恢复GDI对象
            SelectObject(hdc, hOldPen);
            DeleteObject(hBorderPen);
            
            // 绘制拖动手柄（简洁风格）
            HPEN hHandlePen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
            SelectObject(hdc, hHandlePen);
            
            // // 在分割条中心绘制垂直线
            // int centerX = rc.right / 2;
            // for (int y = 0; y < rc.bottom; y += 4) {
            //     MoveToEx(hdc, centerX, y, NULL);
            //     LineTo(hdc, centerX, y + 2);
            // }
            // int centerX = rc.right / 2;  // 分割条中心X坐标
            // int startY = 0;              // 斜线起始Y（避免贴边）
            // int endY = rc.bottom;    // 斜线结束Y（避免贴边）
            int centerX = rc.right / 2;  // 分割条中心X坐标
            int centerY = rc.bottom / 2; // 中心点Y坐标
            int regionHeight = 25;       // 中心区域的高度（可调整）
            int startY = centerY - regionHeight / 2;
            int endY = centerY + regionHeight / 2;
            
            // 确保不超出客户区
            if (startY < 0) startY = 0;
            if (endY > rc.bottom) endY = rc.bottom;
            int step = 4;                 // 斜线垂直间隔（与原垂直线步长一致）
            int lineLen = 6;              // 斜线长度（水平+垂直各3像素，45°方向）
            
            // 第一组：45°右下斜线（同上）
            for (int y = startY; y < endY; y += step) {
                MoveToEx(hdc, centerX - 2, y, NULL);
                LineTo(hdc, centerX + 2, y + 4);  // 短斜线
            }

            // 第二组：45°左上斜线（右上到左下）
            for (int y = startY; y < endY; y += step) {
                MoveToEx(hdc, centerX + 2, y, NULL);
                LineTo(hdc, centerX - 2, y + 4);  // 反向短斜线
            }
            
            // 恢复GDI对象
            SelectObject(hdc, hOldPen);
            DeleteObject(hHandlePen);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    
    // 调用原始窗口过程
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

// 创建工具栏函数
void CreateToolbar(HWND hwndParent) {
    // 创建工具栏容器（使用Rebar控件）
    hwndToolbar = CreateWindowEx(0, REBARCLASSNAME, NULL, 
                              WS_CHILD | WS_VISIBLE | RBS_BANDBORDERS | RBS_VARHEIGHT | CCS_NODIVIDER,
                              0, 0, 0, 0, hwndParent, (HMENU)IDC_TOOLBAR, theInstance, NULL);
    
    // 创建工具栏带（包含搜索控件）
    REBARBANDINFO rbBand = {0};
    rbBand.cbSize = sizeof(REBARBANDINFO);
    rbBand.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbBand.fStyle = RBBS_CHILDEDGE;

    // 创建搜索面板
    HWND hwndSearchPanel = CreateWindowEx(0, "STATIC", NULL, 
                                         WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
                                         0, 0, 0, 0, hwndToolbar, NULL, theInstance, NULL);

    // 创建搜索控件
    hwndSearchEdit = CreateWindow("EDIT", "", 
                                 WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                 0, 0, 150, 22, hwndSearchPanel, (HMENU)ID_SEARCH_EDIT, theInstance, NULL);
    
    // 创建图标按钮
    HICON hIconSearch = LoadIcon(NULL, IDI_HAND); // 搜索图标
    HICON hIconClear = LoadIcon(NULL, IDI_HAND); // 清除图标
    HICON hIconPrev = LoadIcon(NULL, IDI_HAND); // 上一个图标
    HICON hIconNext = LoadIcon(NULL, IDI_HAND); // 下一个图标
    HICON hIconHighlight = LoadIcon(NULL, IDI_HAND); // 高亮图标

    hwndSearchButton = CreateWindow("BUTTON", "", 
                                    WS_CHILD | WS_VISIBLE | BS_ICON | BS_PUSHBUTTON,
                                    155, 0, 24, 22, hwndSearchPanel, (HMENU)ID_SEARCH_BUTTON, theInstance, NULL);
    SendMessage(hwndSearchButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconSearch);

    hwndClearButton = CreateWindow("BUTTON", "", 
                                   WS_CHILD | WS_VISIBLE | BS_ICON | BS_PUSHBUTTON,
                                   180, 0, 24, 22, hwndSearchPanel, (HMENU)ID_CLEAR_BUTTON, theInstance, NULL);
    SendMessage(hwndClearButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconClear);
    
    hwndPrevButton = CreateWindow("BUTTON", "", 
                                   WS_CHILD | WS_VISIBLE | BS_ICON | BS_PUSHBUTTON,
                                   205, 0, 24, 22, hwndSearchPanel, (HMENU)ID_PREV_RESULT, theInstance, NULL);
    SendMessage(hwndPrevButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconPrev);
    
    hwndNextButton = CreateWindow("BUTTON", "", 
                                   WS_CHILD | WS_VISIBLE | BS_ICON | BS_PUSHBUTTON,
                                   230, 0, 24, 22, hwndSearchPanel, (HMENU)ID_NEXT_RESULT, theInstance, NULL);
    SendMessage(hwndNextButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconNext);

    // 创建高亮按钮
    // hwndHighlightButton = CreateWindow("BUTTON", "", 
    //                                    WS_CHILD | WS_VISIBLE | BS_ICON | BS_PUSHBUTTON,
    //                                    255, 0, 24, 22, hwndSearchPanel, (HMENU)ID_HIGHLIGHT_BUTTON, theInstance, NULL);
    // SendMessage(hwndHighlightButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconHighlight);

    // 设置按钮提示文本
    SendMessage(hwndSearchButton, BCM_SETNOTE, 0, (LPARAM)"Search");
    SendMessage(hwndClearButton, BCM_SETNOTE, 0, (LPARAM)"Clear");
    SendMessage(hwndPrevButton, BCM_SETNOTE, 0, (LPARAM)"Previous");
    SendMessage(hwndNextButton, BCM_SETNOTE, 0, (LPARAM)"Next");
    // SendMessage(hwndHighlightButton, BCM_SETNOTE, 0, (LPARAM)"Highlight");
    
    // 设置字体
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hwndSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

    // 添加带区到ReBar
    RECT rcSearch;
    GetClientRect(hwndSearchPanel, &rcSearch);
    rbBand.hwndChild = hwndSearchPanel;
    rbBand.cxMinChild = rcSearch.right;
    rbBand.cyMinChild = rcSearch.bottom;
    rbBand.cx = rcSearch.right;
    SendMessage(hwndToolbar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
}

// 递归搜索树节点
void SearchTreeNodes(HTREEITEM hItem, const std::string& searchText, std::vector<HTREEITEM>& results) {
    if (!hItem) return;
    
    // 获取节点文本
    char buffer[512];
    TVITEM item = {0};
    item.mask = TVIF_TEXT | TVIF_HANDLE;
    item.hItem = hItem;
    item.pszText = buffer;
    item.cchTextMax = 512;
    TreeView_GetItem(theTreeControlWindow, &item);
    
    // 检查是否匹配搜索文本
    std::string nodeText(buffer);
    if (nodeText.find(searchText) != std::string::npos) {
        results.push_back(hItem);
    }
    
    // 搜索子节点
    HTREEITEM hChild = TreeView_GetChild(theTreeControlWindow, hItem);
    while (hChild) {
        SearchTreeNodes(hChild, searchText, results);
        hChild = TreeView_GetNextSibling(theTreeControlWindow, hChild);
    }
}

// 高亮显示搜索结果
void HighlightSearchResult(HTREEITEM hItem) {
    if (hItem) {
        TreeView_EnsureVisible(theTreeControlWindow, hItem);
        TreeView_SelectItem(theTreeControlWindow, hItem);
        
        // 展开所有父节点以便显示结果
        HTREEITEM hParent = TreeView_GetParent(theTreeControlWindow, hItem);
        while (hParent) {
            TreeView_Expand(theTreeControlWindow, hParent, TVE_EXPAND);
            hParent = TreeView_GetParent(theTreeControlWindow, hParent);
        }
    }
}

// 清除搜索高亮
void ClearSearchHighlight() {
    if (g_hCurrentSearchItem) {
        TreeView_SelectItem(theTreeControlWindow, NULL);
        g_hCurrentSearchItem = NULL;
    }
}

// 添加状态栏文本更新函数
void SetStatusText(const char* text) {
    if (hStatusBar) {
        SendMessageA(hStatusBar, SB_SETTEXTA, 0, (LPARAM)text);
    }
}

// 更新状态栏显示当前搜索位置
void UpdateSearchStatus() {
    if (g_searchResults.empty()) {
        SetStatusText("No search results");
        return;
    }
    
    char statusMsg[128];
    sprintf_s(statusMsg, "Result %d of %d", 
              g_currentResultIndex + 1, 
              (int)g_searchResults.size());
    SetStatusText(statusMsg);
}

// 导航到上一个搜索结果
void NavigateToPrevResult() {
    if (g_searchResults.empty()) {
        SetStatusText("No search results to navigate");
        return;
    }
    
    g_currentResultIndex--;
    if (g_currentResultIndex < 0) {
        g_currentResultIndex = (int)g_searchResults.size() - 1; // 循环到最后
    }
    
    g_hCurrentSearchItem = g_searchResults[g_currentResultIndex];
    HighlightSearchResult(g_hCurrentSearchItem);
    UpdateSearchStatus();
    SetFocus(theTreeControlWindow);
}

// 导航到下一个搜索结果
void NavigateToNextResult() {
    if (g_searchResults.empty()) {
        SetStatusText("No search results to navigate");
        return;
    }
    
    g_currentResultIndex++;
    if (g_currentResultIndex >= (int)g_searchResults.size()) {
        g_currentResultIndex = 0; // 循环到开头
    }
    
    g_hCurrentSearchItem = g_searchResults[g_currentResultIndex];
    HighlightSearchResult(g_hCurrentSearchItem);
    UpdateSearchStatus();
    SetFocus(theTreeControlWindow);
}

/*
 * the Monkey window proc
 */
LRESULT CALLBACK WinProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {

    int command;
    short width, height;
    switch(iMsg) {

    case WM_CREATE:
    {
        // create the accessibility tree view
        // 初始化通用控件
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        // 创建工具栏
        CreateToolbar(hwnd);

        theTreeControlWindow = CreateATreeView(hwnd);
        hwndListView = CreateAListView(hwnd);

        // 创建分割条控件（使用静态控件）
        hwndSplitter = CreateWindow("STATIC", 
                                    "", 
                                    WS_CHILD | WS_VISIBLE | SS_NOTIFY,
                                    g_splitterPos, 0, SPLITTER_WIDTH, 0, 
                                    hwnd, 
                                    (HMENU)ID_SPLITTER, 
                                    theInstance, 
                                    NULL);
        // 子类化分割条控件
        SetWindowSubclass(hwndSplitter, SplitterSubclassProc, 0, 0);

        // 创建状态栏
        hStatusBar = CreateWindow(STATUSCLASSNAME, NULL, 
                                WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 
                                0, 0, 0, 0, 
                                hwnd, 
                                (HMENU)ID_STATUSBAR, 
                                theInstance, 
                                NULL);
        
        // 设置状态栏分区（单分区）
        int parts[] = {-1}; // 单分区，自动填充
        SendMessage(hStatusBar, SB_SETPARTS, 1, (LPARAM)parts);
        
        // 设置初始状态文本
        SetStatusText("Ready");

        // load the popup menu
        popupMenu = LoadMenu(theInstance, "PopupMenu");
        popupMenu = GetSubMenu(popupMenu, 0);
        PostMessage(hwnd, WM_USER + 100, 0, 0);
    }
    break;
    case WM_USER + 100: // 自定义消息：窗口加载完成后刷新
        theMonkey->buildAccessibilityTree();
        break;
    // case WM_USER + 101: // 自定义消息：窗口加载完成后刷新
    //     InvalidateRect(hwndListView, NULL, TRUE);   
    //     break;

    case WM_CLOSE:
        EndDialog(hwnd, TRUE);
        PostQuitMessage (0);
        break;

    case WM_SIZE:
        width = LOWORD(lParam);
        height = HIWORD(lParam);
        if (width > 0 && height > 0) {
            // 调整工具栏大小
            // 布局搜索栏（固定在顶部）
            int searchBarHeight = 25;
            SetWindowPos(hwndToolbar, NULL, 0, 0, width, 25, SWP_NOZORDER);
     
            // 调整状态栏位置和大小
            RECT rcStatus;
            SendMessage(hStatusBar, WM_SIZE, 0, 0);
            GetWindowRect(hStatusBar, &rcStatus);
            int statusHeight = rcStatus.bottom - rcStatus.top;
            SetWindowPos(hStatusBar, NULL, 0, height - statusHeight, width, statusHeight, SWP_NOZORDER);
            // 调整主内容区域高度（减去状态栏高度）
            int contentHeight = height - statusHeight - searchBarHeight;
            if (contentHeight < 0) contentHeight = 0;

            // 确保分割位置在合理范围内
            if (g_splitterPos < MIN_PANE_WIDTH) g_splitterPos = MIN_PANE_WIDTH;
            if (g_splitterPos > width - MIN_PANE_WIDTH - SPLITTER_WIDTH) 
                g_splitterPos = width - MIN_PANE_WIDTH - SPLITTER_WIDTH;

            HDWP hdwp = BeginDeferWindowPos(3);
            if (hdwp) {
                int treeY = searchBarHeight;
                int treeHeight = contentHeight - searchBarHeight;

                hdwp = DeferWindowPos(hdwp, theTreeControlWindow, NULL, 0, treeY, g_splitterPos, treeHeight, SWP_NOZORDER | SWP_NOACTIVATE);
                

                hdwp = DeferWindowPos(hdwp, hwndSplitter, NULL, g_splitterPos, treeY, SPLITTER_WIDTH, contentHeight, SWP_NOZORDER | SWP_NOACTIVATE);

    
                hdwp = DeferWindowPos(hdwp, hwndListView, NULL, g_splitterPos + SPLITTER_WIDTH, treeY, width - g_splitterPos - SPLITTER_WIDTH, contentHeight, SWP_NOZORDER | SWP_NOACTIVATE);
                EndDeferWindowPos(hdwp);
            }        
        }
        return(FALSE);			// let windows finish handling this

    case WM_COMMAND:
        command = LOWORD(wParam);
        switch(command) {
            
        case cExitMenuItem:
            EndDialog(hwnd, TRUE);
            PostQuitMessage (0);
            break;
        case ID_REFRESH_TREE:
        case cRefreshTreeItem:
            // update the accessibility tree
            theMonkey->buildAccessibilityTree();
            break;
            
        case cAPIMenuItem:
            // open a new window with the Accessibility API in it for the
            // selected element in the tree
            if (theSelectedNode != (AccessibleNode *) 0) {
                theSelectedNode->displayAPIWindow();
            }
            break;
            
        case cAPIPopupItem:
            // open a new window with the Accessibility API in it for the
            // element in the tree adjacent to the popup menu
            if (thePopupNode != (AccessibleNode *) 0) {
                thePopupNode->displayAPIWindow();
            }
            break;
        // 搜索相关命令
        case ID_NEXT_RESULT:
            NavigateToNextResult();
            break;
        case ID_PREV_RESULT:
            NavigateToPrevResult();
            break;
        case ID_SEARCH_ENTER:
        case ID_SEARCH_BUTTON:
            {
                char searchText[256];
                GetWindowText(hwndSearchEdit, searchText, 256);
                
                if (strlen(searchText) > 0) {
                    // 执行搜索
                    g_searchResults.clear();
                    g_currentResultIndex = -1;
                    
                    SearchTreeNodes(TreeView_GetRoot(theTreeControlWindow), searchText, g_searchResults);
                    
                    if (!g_searchResults.empty()) {
                        g_currentResultIndex = 0;
                        g_hCurrentSearchItem = g_searchResults[g_currentResultIndex];
                        HighlightSearchResult(g_hCurrentSearchItem);
                        UpdateSearchStatus();
                        // 搜索完成后聚焦到树控件
                        SetFocus(theTreeControlWindow);
                    } else {
                        SetStatusText("No matches found");
                    }
                }
                else 
                {
                    // 搜索框为空时清除搜索
                    SetWindowText(hwndSearchEdit, "");
                    ClearSearchHighlight();
                    g_searchResults.clear();
                    g_currentResultIndex = -1;
                    SetStatusText("Search cleared");
                }
            }
            break;
        case ID_CLEAR_BUTTON:
            {
                SetWindowText(hwndSearchEdit, "");
                ClearSearchHighlight();
                g_searchResults.clear();
                g_currentResultIndex = -1;
                SetStatusText("Search cleared");
            }
            break;
        }
        break;
    case WM_NOTIFY:				// receive tree messages

        NMTREEVIEW *nmptr = (LPNMTREEVIEW) lParam;
        switch (nmptr->hdr.code) {

        case TVN_SELCHANGED:
            // get the selected tree node
            {
                theSelectedNode = (AccessibleNode *) nmptr->itemNew.lParam;	
                // 更新属性表格
                UpdatePropertyTable(theSelectedNode);
            }
            break;

        case NM_RCLICK:

            // display a popup menu over the tree node
            POINT p;
            GetCursorPos(&p);
            TrackPopupMenu(popupMenu, 0, p.x, p.y, 0, hwnd, NULL);

            // get the tree node under the popup menu
            TVHITTESTINFO hitinfo;
            ScreenToClient(theTreeControlWindow, &p);
            hitinfo.pt = p;
            HTREEITEM node = TreeView_HitTest(theTreeControlWindow, &hitinfo);

            if (node != null) {
                TVITEMEX tvItem;
                tvItem.hItem = node;
                if (TreeView_GetItem(hwndTV, &tvItem) == TRUE) {
                    thePopupNode = (AccessibleNode *)tvItem.lParam;
                }
            }
            break;
        }
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

/*
 * Accessibility information window proc
 */
LRESULT CALLBACK AccessInfoWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    short width, height;
    HWND dlgItem;
    
    switch (message) {
    case WM_CREATE:
        RECT rcClient;    // dimensions of client area 
        HWND hwndEdit;    // handle of tree-view control 
        
        // Get the dimensions of the parent window's client area,  
        // and create the edit control. 
        GetClientRect(hWnd, &rcClient); 
        hwndEdit = CreateWindow("Edit", 
                                "",
                                WS_VISIBLE | WS_TABSTOP | WS_CHILD |
                                ES_MULTILINE | ES_AUTOVSCROLL | 
                                ES_READONLY | WS_VSCROLL,
                                0, 0, rcClient.right, rcClient.bottom, 
                                hWnd,
                                (HMENU) cAccessInfoText,
                                theInstance,
                                NULL);
        break;
        
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
        
    case WM_SIZE:
        width = LOWORD(lParam);
        height = HIWORD(lParam);
        dlgItem = GetDlgItem(hWnd, cAccessInfoText); 
        SetWindowPos(dlgItem, NULL, 0, 0, width, height, 0); 
        return(FALSE);			// let windows finish handling this
        break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    
    return 0;   
}

/**
 * Build a tree (and the treeview control) of all accessible Java components
 *
 */
void JavaMonkey::buildAccessibilityTree() {
    auto hwnds = rpa::java::GetJavaWindows();
    TreeView_DeleteAllItems(theTreeControlWindow);
    for(auto hwnd: hwnds)
    {
        if (IsJavaWindow(hwnd)) {
            long vmID;
            AccessibleContext ac;
            if (GetAccessibleContextFromHWND(hwnd, &vmID, &ac) == TRUE) {
                theMonkey->addComponentNodes(vmID, ac, (AccessibleNode *) NULL, 
                                            hwnd, TVI_ROOT, theTreeControlWindow);
            }
            topLevelWindow = hwnd;
        }
    }
    // have MS-Windows call EnumWndProc() with all of the top-level windows
    // EnumWindows((WNDENUMPROC) EnumWndProc, NULL);
}

/**
 * Create (and display) the accessible component nodes of a parent AccessibleContext
 *
 */
BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lParam) {
    if (IsJavaWindow(hwnd)) {
        long vmID;
        AccessibleContext ac;
        if (GetAccessibleContextFromHWND(hwnd, &vmID, &ac) == TRUE) {
            theMonkey->addComponentNodes(vmID, ac, (AccessibleNode *) NULL, 
                                         hwnd, TVI_ROOT, theTreeControlWindow);
        }
        topLevelWindow = hwnd;
    }
    return(TRUE);
}

// CreateATreeView - creates a tree-view control. 
// Returns the handle of the new control if successful or NULL
//     otherwise. 
// hwndParent - handle of the control's parent window 
HWND CreateATreeView(HWND hwndParent) { 
    RECT rcClient;  // dimensions of client area 
    
    // Get the dimensions of the parent window's client area, and create 
    // the tree-view control. 
    GetClientRect(hwndParent, &rcClient); 
    // 计算树控件的可用区域（减去搜索栏高度）
    int searchBarHeight = 35;
    int treeY = searchBarHeight;
    int treeHeight = rcClient.bottom - searchBarHeight;
    hwndTV = CreateWindow(WC_TREEVIEW, 
                          "",
                          WS_VISIBLE | WS_TABSTOP | WS_CHILD |
                          TVS_HASLINES | TVS_HASBUTTONS |
                          TVS_LINESATROOT, 
                          0, treeY, rcClient.right, treeHeight, 
                          hwndParent,
                          (HMENU) cTreeControl,
                          theInstance,
                          NULL);
    
    return hwndTV; 
} 

HWND CreateAListView(HWND hwndParent) { 
    // 创建列表视图控件（表格）
    hwndListView = CreateWindow(WC_LISTVIEW, 
                                "", 
                                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                0, 0, 0, 0, 
                                hwndParent, 
                                (HMENU)ID_LISTVIEW, 
                                theInstance, 
                                NULL);
    
    // 设置列表视图扩展样式
    ListView_SetExtendedListViewStyle(hwndListView, 
                                      LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    
    // 添加列
    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    
    lvc.iSubItem = 0;
    lvc.pszText = "Property";
    lvc.cx = 150;
    ListView_InsertColumn(hwndListView, 0, &lvc);
    
    lvc.iSubItem = 1;
    lvc.pszText = "Value";
    lvc.cx = 300;
    ListView_InsertColumn(hwndListView, 1, &lvc);
    return hwndListView;
} 

/**
 * Create (and display) the accessible component nodes of a parent AccessibleContext
 *
 */
void JavaMonkey::addComponentNodes(long vmID, AccessibleContext context,
                                   AccessibleNode *parent, HWND hwnd, 
                                   HTREEITEM treeNodeParent, HWND treeWnd) {

    AccessibleNode *newNode = new AccessibleNode(vmID, context, parent, hwnd, treeNodeParent);

    AccessibleContextInfo info;
    if (GetAccessibleContextInfo(vmID, context, &info) != FALSE) {
        char s[LINE_BUFSIZE];

        wsprintf(s, "%ls", info.name);
        newNode->setAccessibleName(s);
        wsprintf(s, "%ls", info.role);
        newNode->setAccessibleRole(s);

        wsprintf(s, "%ls [%ls]", info.name, info.role);

        TVITEM tvi;
        tvi.mask = TVIF_PARAM | TVIF_TEXT;
        tvi.pszText = (char *) s; // Accessible name and role
        tvi.cchTextMax = (int)strlen(s);
        tvi.lParam = (LONG_PTR) newNode; // Accessibility information

        TVINSERTSTRUCT tvis;
        tvis.hParent = treeNodeParent;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item = tvi;

        HTREEITEM treeNodeItem = TreeView_InsertItem(treeWnd, &tvis);

        for (int i = 0; i < info.childrenCount; i++) {
            addComponentNodes(vmID, GetAccessibleChildFromContext(vmID, context, i), 
                              newNode, hwnd, treeNodeItem, treeWnd);
        }
    } else {
        char s[LINE_BUFSIZE];
        sprintf(s, "ERROR calling GetAccessibleContextInfo; vmID = %X, context = %X", vmID, context);

        TVITEM tvi;
        tvi.mask = TVIF_PARAM | TVIF_TEXT;	// text and lParam are only valid parts
        tvi.pszText = (char *) s;
        tvi.cchTextMax = (int)strlen(s);
        tvi.lParam = (LONG_PTR) newNode;

        TVINSERTSTRUCT tvis;
        tvis.hParent = treeNodeParent;
        tvis.hInsertAfter = TVI_LAST;	// make tree in order given
        tvis.item = tvi;

        HTREEITEM treeNodeItem = TreeView_InsertItem(treeWnd, &tvis);
    }
}

// -----------------------------

/**
 * Create an AccessibleNode
 *
 */
AccessibleNode::AccessibleNode(long JavaVMID, AccessibleContext context,
                               AccessibleNode *parent, HWND hwnd, 
                               HTREEITEM parentTreeNodeItem) {
    vmID = JavaVMID;
    ac = context;
    parentNode = parent;
    baseHWND = hwnd;
    treeNodeParent = parentTreeNodeItem;

    // setting accessibleName and accessibleRole not done here,
    // in order to minimize calls to the AccessBridge
    // (since such a call is needed to enumerate children)
}

/**
 * Destroy an AccessibleNode
 *
 */
AccessibleNode::~AccessibleNode() {
    ReleaseJavaObject(vmID, ac);
}

/**
 * Set the accessibleName string
 *
 */
void AccessibleNode::setAccessibleName(char *name) {
    strncpy(accessibleName, name, MAX_STRING_SIZE);
}

/**
 * Set the accessibleRole string
 *
 */
void AccessibleNode::setAccessibleRole(char *role) {
    strncpy(accessibleRole, role, SHORT_STRING_SIZE);
}

std::shared_ptr<JabNodeInfo> AccessibleNode::GetNodeInfo()
{
    AccessibleContextInfo info;
    if (GetAccessibleContextInfo(vmID, ac, &info) == FALSE) {
        return nullptr;
    } else {
        return std::make_shared<JabNodeInfo>(info);
    }
}





/**
 * Create an API window to show off the info for this AccessibleContext
 */
BOOL AccessibleNode::displayAPIWindow() {

    HWND apiWindow = CreateWindow(theAccessInfoClassName,
                                  "Java Accessibility API view",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  600,
                                  750,
                                  HWND_DESKTOP,
                                  NULL,
                                  theInstance,
                                  (void *) NULL);

    if (!apiWindow) {
        printError("cannot create API window");
        return (FALSE);
    }

    char buffer[HUGE_BUFSIZE] = {0};
    try
    {
        getAccessibleInfo(vmID, ac, buffer, sizeof(buffer));
        displayAndLog(apiWindow, cAccessInfoText, logfile, buffer);		
    }
    catch(std::exception const & e)
    {
    }
    
    ShowWindow(apiWindow, SW_SHOWNORMAL);
    UpdateWindow(apiWindow); 

    return (TRUE);      
}



