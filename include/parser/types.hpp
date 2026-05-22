// types.hpp

#pragma once

#include <string>
#include <variant>

namespace parser {

// ---------------------------------< Types >----------------------------------
struct BuiltinType {
  enum class Kind { 
    kInt, 
    kFloat, 
    kString, 
    kBool,
    kUnit,
  } kind;
  bool operator==(const BuiltinType&) const = default;
};

struct UserType {
  std::string name;
  bool operator==(const UserType&) const = default;
};

using TypeVariant = std::variant<BuiltinType, UserType>;

namespace concepts {
template <typename T>
concept IsType = 
    std::is_same_v<T, BuiltinType> || 
    std::is_same_v<T, UserType>;
}  // namespace concepts  


} // namespace parser