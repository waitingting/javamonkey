#include "string.h"

#include <Windows.h>

namespace util::string {

std::string ConvertWCSToMBS(const wchar_t *pstr, int wslen) {
 int len = ::WideCharToMultiByte(CP_UTF8, 0, pstr, wslen, NULL, 0, NULL, NULL);
 std::string dblstr(len, '\0');
 len = ::WideCharToMultiByte(CP_UTF8, 0, pstr, wslen, &dblstr[0], len, NULL, NULL);
 return dblstr;
}

std::string ToString(const std::wstring &s) {
  return ConvertWCSToMBS(s.c_str(), static_cast<int>(s.size()));
}

std::wstring ToWString(const std::string &s) {
 int wslen = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), NULL, 0);
 std::wstring wsdata;
 wsdata.resize(wslen);
 ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), wsdata.data(), wslen);
 return wsdata;
}

}  // namespace util::string