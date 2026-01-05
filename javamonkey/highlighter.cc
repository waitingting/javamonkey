#include "highlighter.h"
#include "string.h"
#include "misc.h"
#include <Windows.h>
#include <iostream>
namespace uia::testing
{
  bool WindowsHighlighter::Clicked()
  {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
    {
        return true;
    }
    auto isWindow = ::IsWindow(hwnd_.main);
    if(isWindow){
      auto pt = rpad::util::misc::GetCursorPos();
      RECT rect;
      GetWindowRect(hwnd_.main, &rect);
      // int circleRadius = 5;
      int clientWidth = rect.right - rect.left;
      int clientHeight = rect.bottom - rect.top;
      int circleCenterX = rect.left + clientWidth / 2;
      int circleCenterY = rect.top + clientHeight / 2;
      // if(abs(circleCenterX - pt.x) < 5 && abs(circleCenterY - pt.y) < 5){
      //   return true;
      // }
      return false;
    }
    return true;
  }
  void WindowsHighlighter::WriteToClipboard(const std::wstring &text)
  {
    std::lock_guard<std::mutex> lock(clipboardMutex);

    if (!OpenClipboard(nullptr))
    {
      std::cout << "Open clipboard error" << std::endl;
      return;
    }
    EmptyClipboard();
    auto ctext = text.c_str();
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (wcslen(ctext) + 1) * sizeof(wchar_t));
    if (hGlobal != NULL)
    {
      wchar_t *buffer = static_cast<wchar_t *>(GlobalLock(hGlobal));
      wcscpy_s(buffer, wcslen(ctext) + 1, ctext);
      GlobalUnlock(hGlobal);
      SetClipboardData(CF_UNICODETEXT, hGlobal);
    }
    CloseClipboard();
  }

  LRESULT CALLBACK WindowsHighlighter::StaticWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
    WindowsHighlighter *pEngine = reinterpret_cast<WindowsHighlighter *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (pEngine != nullptr)
    {
      return pEngine->WindowProc(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
  }

  LRESULT CALLBACK WindowsHighlighter::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
    switch (msg)
    {
    // case WM_CREATE:
    // {
    //   SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    //   COLORREF clTransparent = RGB(0x98, 0xFB, 0x98);
    //   SetLayeredWindowAttributes(hWnd, clTransparent, 0, LWA_COLORKEY);
    //   break;
    // }
    case WM_CLOSE:
    {
      DestroyWindow(hWnd);
      break;
    }
    case WM_LBUTTONDOWN:
    {
      if (hWnd == hwnd_.title)
      {
        RECT rect;
        if (GetWindowRect(hWnd, &rect))
        {
          int width = rect.right - rect.left;
          int height = rect.bottom - rect.top;
          std::string text = "Window Position: (" + std::to_string(rect.left) + ", " + std::to_string(rect.top) + ")\n" + "Window Size: " + std::to_string(width) + " x " + std::to_string(height) + "\n" + "Window Description: " + hwnd_.text;
          std::wstring utext = util::string::ToWString(text);
          WriteToClipboard(utext);
        }
      }
    }
    case WM_PAINT:
    {
      if (hWnd == hwnd_.main)
      {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Draw the rectangle border
        RECT rect;
        GetClientRect(hWnd, &rect);

        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
        HGDIOBJ hOldPen = SelectObject(hdc, hPen);
        HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

        // Draw the circle in the client area
        // int circleRadius = clientWidth < clientHeight ? clientWidth / 4 : clientHeight / 4;
        int circleRadius = 5;
        int clientWidth = rect.right - rect.left;
        int clientHeight = rect.bottom - rect.top;
        int circleCenterX = clientWidth / 2;
        int circleCenterY = clientHeight / 2;
        HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
        SelectObject(hdc, redBrush);
        Ellipse(hdc, circleCenterX - circleRadius, circleCenterY - circleRadius, circleCenterX + circleRadius, circleCenterY + circleRadius);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);
        DeleteObject(redBrush);

        EndPaint(hWnd, &ps);
        break;
      }
      else
      {
        return DefWindowProc(hWnd, msg, wParam, lParam);
      }
    }
    case WM_DESTROY:
    case WM_RBUTTONDOWN:
    {
      PostQuitMessage(0);
      break;
    }
    default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
  }

  void WindowsHighlighter::ThreadProc(bool, std::promise<WinHwnd> &promise)
  {
    auto hInstance = GetModuleHandle(nullptr);
    // Register the window class.
    LPCSTR className = "DGH_HighlighterWindowClass";
    LPCSTR classNameChild = "DGH_HighlighterWindowClassChild";
    HBRUSH brush = CreateSolidBrush(RGB(0x98, 0xFB, 0x98));
    // HBRUSH childBrush = CreateSolidBrush(RGB(255, 0, 0));
    WNDCLASSEX wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = StaticWindowProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = className;
    wcex.hbrBackground = brush;
    wcex.cbWndExtra = sizeof(WindowsHighlighter *);
    RegisterClassEx(&wcex);

    WNDCLASSEX wcexChild{};
    wcexChild.cbSize = sizeof(WNDCLASSEX);
    wcexChild.lpfnWndProc = StaticWindowProc;
    wcexChild.hInstance = hInstance;
    wcexChild.lpszClassName = classNameChild;
    wcexChild.hbrBackground = brush;
    wcexChild.cbWndExtra = sizeof(WindowsHighlighter *);
    RegisterClassEx(&wcexChild);

    // Create the window.
    auto hWndMain = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE |
                                       WS_EX_LAYERED, // WS_EX_TRANSPARENT | WS_EX_LAYERED,
                                   className, "DataGrand WindowsHighlighter", WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT,
                                   CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, this);

    auto hWndChild = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
                                        WS_EX_LAYERED, // WS_EX_TRANSPARENT | WS_EX_LAYERED,
                                    classNameChild, "DataGrand WindowsHighlighter", WS_VISIBLE | WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT, hWndMain, NULL, hInstance, this);

    // auto hWndTitle = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
    //                                     WS_EX_LAYERED, // WS_EX_TRANSPARENT | WS_EX_LAYERED,
    //                                 classNameChild, L"DataGrand WindowsHighlighter", WS_VISIBLE | WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
    //                                 CW_USEDEFAULT, CW_USEDEFAULT, hWndMain, NULL, hInstance, this);
    auto hWndTitle = CreateWindowEx(0, "STATIC", "DataGrand WindowsHighlighter", WS_CHILD | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWndChild, NULL, hInstance, this);

    if (hWndMain != NULL && hWndChild != NULL && hWndTitle != NULL)
    {
      SetWindowLongPtr(hWndMain, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
      SetWindowLongPtr(hWndChild, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
      SetWindowLongPtr(hWndTitle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
      SetLayeredWindowAttributes(hWndMain, RGB(0, 0, 0), 100, LWA_ALPHA);
      SetLayeredWindowAttributes(hWndChild, RGB(0x98, 0xFB, 0x98), 100, LWA_COLORKEY);
      // SetLayeredWindowAttributes(hWndTitle, RGB(0x98, 0xFB, 0x98), 100, LWA_COLORKEY);
      // SetLayeredWindowAttributes(hWndTitle, RGB(0, 0, 0), 100, LWA_ALPHA);

      ShowWindow(hWndMain, SW_HIDE);
      ShowWindow(hWndChild, SW_HIDE);
      ShowWindow(hWndTitle, SW_SHOWNOACTIVATE);
    }
    WinHwnd winHwnd = {hWndMain, hWndChild, hWndTitle, "", {0}};
    promise.set_value(winHwnd);
    MSG msg;
    BOOL value;

    while ((value = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
      if (value == -1)
      {
        // TODO: handle error here
      }
      else
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    UnregisterClass(className, hInstance);
    UnregisterClass(classNameChild, hInstance);
  }

  WindowsHighlighter::WindowsHighlighter(bool penetration)
  {
    std::promise<WinHwnd> promise;
    std::future<WinHwnd> future = promise.get_future();
    thread_ = std::thread(&WindowsHighlighter::ThreadProc, this, penetration, std::ref(promise));
    hwnd_ = future.get();
  }

  WindowsHighlighter::~WindowsHighlighter()
  {
    if (hwnd_.main != NULL)
    {
      ::SendMessage(hwnd_.main, WM_CLOSE, NULL, NULL);
    }
    if (hwnd_.child != NULL)
    {
      ::SendMessage(hwnd_.child, WM_CLOSE, NULL, NULL);
    }
    thread_.join();
  }

  void WindowsHighlighter::Highlight(rpad::automation::Rectangle rect, std::string title)
  {
    if (lastRect == rect && lastTitle == title)
    {
      return;
    }
    hwnd_.text = title;

    HDC hdc = GetDC(hwnd_.child); 
    RECT rectx = {0, 0, 0, 0};
    HFONT hFont = (HFONT)SendMessage(hwnd_.title, WM_GETFONT, 0, 0);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    auto ctext = title.c_str();
    DrawText(hdc, ctext, -1, &rectx, DT_CALCRECT);
    int titleWidth = rectx.right - rectx.left /*+ GetSystemMetrics(SM_CXFRAME) * 2*/;
    int titleHeight = rectx.bottom - rectx.top;
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd_.child, hdc);

    hwnd_.width = titleWidth;
    SetWindowText(hwnd_.title, ctext);

    SetWindowPos(hwnd_.title, HWND_TOP, 0, 0, titleWidth, titleHeight, SWP_SHOWWINDOW);
    SetWindowPos(hwnd_.child, HWND_TOPMOST, rect.left, rect.top- titleHeight - 2, titleWidth, titleHeight, SWP_NOACTIVATE);
    SetWindowPos(hwnd_.main, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE);

    ShowWindow(hwnd_.main, SW_SHOWNOACTIVATE);
    ShowWindow(hwnd_.child, SW_SHOWNOACTIVATE);

    lastRect = rect;
    lastTitle = title;
  }

} // namespace rpad::automation::driver
