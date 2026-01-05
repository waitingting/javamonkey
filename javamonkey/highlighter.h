#pragma once

#include <Windows.h>
#include <thread>
#include <future>
#include <mutex>
#include "misc.h"

namespace uia::testing {

struct WinHwnd {
  HWND main;
  HWND child;
  HWND title;
  std::string text;
  int width;
};

class WindowsHighlighter {
 public:
  WindowsHighlighter(bool penetration = true);
  ~WindowsHighlighter();

 public:
  void Highlight(rpad::automation::Rectangle, std::string title = "描述");
  bool Clicked();

 private:
  void ThreadProc(bool penetration, std::promise<WinHwnd> &promise);
  static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  void WriteToClipboard(const std::wstring& text);
  WinHwnd hwnd_;
  std::thread thread_;
  std::mutex clipboardMutex;
  rpad::automation::Rectangle lastRect;
  std::string lastTitle;
};

}  // namespace rpad::automation::driver
