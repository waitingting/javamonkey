#pragma once

#include <string>

namespace util::string {

std::string ToString(const std::wstring &);
std::wstring ToWString(const std::string &);

}  // namespace util::string