#pragma once

#include <string>
#include <cassert>

namespace detail {

template<typename T>
constexpr auto getPrettyFunctionInfo() {
  return __PRETTY_FUNCTION__;
}

} // namespace detail

template<typename T>
constexpr std::string toString() {
  std::string info = detail::getPrettyFunctionInfo<T>();

  auto pos = info.find("[T = ");
  assert(pos != std::string::npos);

  auto end_pos = info.find("]");
  assert(end_pos != std::string::npos);

  return std::string{info.data() + pos + 5, info.data() + end_pos};
}

template<typename T>
constexpr std::string toStringUnqualified() {
  auto qualified = toString<T>();

  auto pos = qualified.find_last_of("::");

  return std::string{qualified.begin() + (pos != std::string::npos ? pos + 1 : 0), qualified.end()};
}
