#pragma once

#include <string>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <filesystem>

namespace rpad::automation {
  struct Point {
    int x;
    int y;
  };

  struct Rectangle {
    int left;
    int top;
    int right;
    int bottom;

    bool contains(Point pt) const {
      if (left <= pt.x && pt.x <= right)
        if (top <= pt.y && pt.y <= bottom)
          return true;
      return false;
    }

    std::size_t width() {
      return right - left;
    }

    std::size_t height() {
      return bottom - top;
    }

    std::size_t area() {
      return width() * height();
    }

    bool operator==(const Rectangle b) const{
      return this->left == b.left && this->top == b.top && this->right == b.right && this->bottom == b.bottom;
    }
  };
}

namespace rpad::util::misc {

std::string GetProgramLocation();
std::filesystem::path GetUserDir();

uint64_t GetTimePoint();
bool IsSpeedTime(uint64_t start, uint64_t end, uint64_t speed_time);

std::string GetImagePathByPid(int pid);
std::string GetProcessImageFileNameByPid(int pid);

automation::Point GetCursorPos();

} // namespace rpad::util::misc