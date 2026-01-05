#pragma once
#include <memory>
#include <combaseapi.h>
#include <atlbase.h>
#include <vector>

namespace rpa::java
{
  std::vector<HWND> GetJavaWindows();
}