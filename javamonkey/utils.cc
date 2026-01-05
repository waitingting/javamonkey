#include "utils.h"
#include <string>
#include <Windows.h>

namespace rpa::java
{
  bool IsSunAwtClass(HWND hwnd)
  {
    wchar_t className[256];
    GetClassNameW(hwnd, className, sizeof(className) / sizeof(wchar_t));
    
    const wchar_t prefix[] = L"SunAwt";
    size_t prefixLen = wcslen(prefix);
    auto classCheck = (wcsncmp(className, prefix, prefixLen) == 0);
    if(!classCheck)
    {
      return false;
    }
    if (!::IsWindowVisible(hwnd)) {
      return false;
    }
    return true;
  }

  BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM arg)
  {
    std::vector<HWND>* hwnds = reinterpret_cast<std::vector<HWND>*>(arg);
    if (IsSunAwtClass(hwnd))
    {
      hwnds->push_back(hwnd);
      return FALSE;
    }
    return TRUE;
  }

  BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
  {
    std::vector<HWND>* hwnds = reinterpret_cast<std::vector<HWND>*>(lParam);

    if (IsSunAwtClass(hWnd))
    {
      hwnds->push_back(hWnd);
      return TRUE;
    }

    EnumChildWindows(hWnd, EnumChildWindowsProc, lParam);
    return TRUE;
  }

  std::vector<HWND> GetJavaWindows()
  {
    std::vector<HWND> foundWindows;
    ::EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&foundWindows));
    return foundWindows;
  }
}