/// @file Helper.h 
/// @brief Utility helper functions.
#pragma once

#include <algorithm>
#include <string>

class Helper {
  public:

  /// Convert string to lowercase
  static std::string stringToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    return s;
  }
};
