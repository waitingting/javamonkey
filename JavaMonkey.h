/*
 * Copyright 2005 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*
 * @(#)JavaMonkey.h	1.3 05/03/21
 */

#include <windows.h>   // includes basic windows functionality
#include <stdio.h>
#include <commctrl.h>
#include <jni.h>
#include "monkeyResource.h"
#include "AccessBridgeCalls.h"
#include "AccessBridgeCallbacks.h"
#include "AccessBridgeDebug.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <string>
#include <time.h>
#include <memory>

extern FILE *file;

#define null NULL
#define JAVA_MONKEY_LOG "JavaMonkey.log"

struct JabNodeInfo {
    std::wstring name;
    std::wstring description;

    std::wstring role;
    std::wstring role_en_US;
    std::wstring states;
    std::wstring states_en_US;

    int indexInParent;
    int childrenCount;

    int x;
    int y;
    int width;
    int height;

    bool accessibleComponent;
    bool accessibleAction;
    bool accessibleSelection;
    bool accessibleText;
    bool accessibleInterfaces;

    explicit JabNodeInfo(AccessibleContextInfo info) : name(std::wstring(info.name)),
        description(std::wstring(info.description)),
        role(std::wstring(info.role)),
        role_en_US(std::wstring(info.role_en_US)),
        states(std::wstring(info.states)),
        states_en_US(std::wstring(info.states_en_US)),
        indexInParent(int(info.indexInParent)),
        childrenCount(int(info.childrenCount)),
        x(int(info.x)),
        y(int(info.y)),
        width(int(info.width)),
        height(int(info.height)),
        accessibleComponent(info.accessibleComponent != 0),
        accessibleAction(info.accessibleAction != 0),
        accessibleSelection(info.accessibleSelection != 0),
        accessibleText(info.accessibleText != 0),
        accessibleInterfaces(info.accessibleInterfaces != 0) {
            if(accessibleSelection)
            {

            }
        }

    bool operator==(JabNodeInfo const& other) const {
        return name == other.name &&
            role_en_US == other.role_en_US &&
            indexInParent == other.indexInParent &&
            childrenCount == other.childrenCount &&
            x == other.x &&
            y == other.y &&
            width == other.width &&
            height == other.height;
    }

    std::string get(std::string key)
    {
        return "";
    }
};
/**
 * A node in the Monkey tree
 */
class AccessibleNode {

    HWND baseHWND;
    HTREEITEM treeNodeParent;
    long vmID;
    AccessibleContext ac;
    AccessibleNode *parentNode;
    char accessibleName[MAX_STRING_SIZE];
    char accessibleRole[SHORT_STRING_SIZE];

public:
    AccessibleNode(long vmID, AccessibleContext context, 
                   AccessibleNode *parent, HWND hWnd, 
                   HTREEITEM parentTreeNodeItem);
    ~AccessibleNode();
    void setAccessibleName(char *name);
    void setAccessibleRole(char *role);
    BOOL displayAPIWindow();	// bring up an Accessibility API detail window
    std::shared_ptr<JabNodeInfo> GetNodeInfo();
};


/**
 * The main application class
 */
class JavaMonkey {

public:
    JavaMonkey(int nCmdShow);
    BOOL InitWindow(int windowMode);
    char *getAccessibleInfo(long vmID, AccessibleContext ac, char *buffer, int bufsize);
    void exitJavaMonkey(HWND hWnd);
    void buildAccessibilityTree();
    void addComponentNodes(long vmID, AccessibleContext context, 
                           AccessibleNode *parent, HWND hWnd,
                           HTREEITEM treeNodeParent, HWND treeWnd);
};

char *getTimeAndDate();

void displayAndLogText(char *buffer, ...);

LRESULT CALLBACK WinProc (HWND, UINT, WPARAM, LPARAM);

void debugString(char *msg, ...);

LRESULT CALLBACK MonkeyWindowProc(HWND hDlg, UINT message, UINT wParam, LONG lParam);

BOOL CALLBACK EnumWndProc(HWND hWnd, LPARAM lParam);

HWND CreateATreeView(HWND hwndParent);

void AddTableRow(int rowIndex, const char* property, const wchar_t* value);
void AddTableRow(int rowIndex, const char* property, const char* value);
void UpdatePropertyTable(AccessibleNode* node);

HWND CreateAListView(HWND hwndParent);
void CreateToolbar(HWND hwndParent);

LRESULT CALLBACK AccessInfoWindowProc (HWND hWnd, UINT message, UINT wParam, LONG lParam);

char *getAccessibleInfo(long vmID, AccessibleContext ac, char *buffer, int bufsize);



