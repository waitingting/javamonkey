#include "misc.h"
#include "string.h"
#include <filesystem>
#include <cwctype>
#include <shlobj_core.h>
#include <psapi.h>
#include <wtsapi32.h>
#include <versionhelpers.h>
#pragma comment(lib, "Wtsapi32.lib")

namespace rpad::util::misc
{
  std::filesystem::path GetUserDir()
  {
    wchar_t result[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, result)) && result[0])
    {
      return std::filesystem::path(result);
    }
    return std::filesystem::path();
  }

  std::string GetImagePathByPid(int pid)
  {
    return "";
  }

  std::string GetProcessImageFileNameByPid(int pid)
  {
    wchar_t image_name[256];
    auto proc_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
    if (proc_handle == NULL)
    {
      return "";
    }
    auto size = GetProcessImageFileNameW(proc_handle, image_name, 255);
    CloseHandle(proc_handle);

    if (size > 0)
    {
      std::wstring full_name(image_name);
      auto pos = full_name.rfind('\\');
      if (pos != std::string::npos)
      {
        full_name = full_name.substr(pos + 1, full_name.length() - pos - 1);
      }
      // if (boost::algorithm::ends_with(full_name, L".exe")) {
      //     full_name = full_name.substr(0, full_name.length() - 4);
      // }
      std::transform(full_name.begin(), full_name.end(), full_name.begin(), std::towlower);

      return ::util::string::ToString(full_name);
    }
    return "";
  }

  automation::Point GetCursorPos()
  {
    POINT p;
    GetPhysicalCursorPos(&p);
    return automation::Point{p.x, p.y};
  }
}