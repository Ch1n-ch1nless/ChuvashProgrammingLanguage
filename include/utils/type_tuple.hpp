#pragma once

#include <variant>

/*

A little piece of metaprogramming course. We use it to create type hierarchies with concepts.
If you wanna see more crazy stuff like that, check out Roman Sandu's course.
Task "type_lists" there is just about that kind of things.
https://github.com/Mrkol/metaprogramming-course

*/

template<typename... Ts>
struct TTuple {
};

template<typename T>
concept TypeTuple = requires(T t) {
  []<typename... Ts>(TTuple<Ts...>){}(t);
};

// suppouse to work:

// static_assert(TypeTuple<TTuple<char>>);

// static_assert(TypeTuple<TTuple<char, int, float, double>>);

template<TypeTuple Left, TypeTuple Right>
struct Concat;

template<typename... LeftTs, typename... RightTs>
struct Concat<TTuple<LeftTs...>, TTuple<RightTs...>> {
  using Result = TTuple<LeftTs..., RightTs...>;
};

// suppouse to work:

// static_assert(
//   std::same_as
//   < Concat
//     < TypeTuple
//       < int
//       , float
//       >
//     , TypeTuple
//       < double
//       , char
//       >
//     >
//   , TypeTuple<int, float, double, char>
//   >
// );

// static_assert(
//   std::same_as
//   < Concat
//     < TypeTuple<int, double>
//     , TypeTuple<>
//     >
//   , TypeTuple<int, double>
//   >
// );

// static_assert(
//   std::same_as
//   < Concat
//     < TypeTuple<>
//     , TypeTuple<float, char>
//     >
//   , TypeTuple<float, char>
//   >
// );

// static_assert(
//   std::same_as
//   < Concat
//     < TypeTuple<>
//     , TypeTuple<>
//     >
//   , TypeTuple<>
//   >
// );

template<TypeTuple Tuple, typename T>
struct Contains;

template<typename Head, typename... Tail, typename T>
struct Contains<TTuple<Head, Tail...>, T> {
  static constexpr bool value = Contains<TTuple<Tail...>, T>::value;
};

template<typename Head, typename... Tail>
struct Contains<TTuple<Head, Tail...>, Head> {
  static constexpr bool value = true;
};

template<typename T>
struct Contains<TTuple<>, T> {
  static constexpr bool value = false;
};

// suppouse to work:

// static_assert(Contains<TTuple<char>, char>::value == true);

// static_assert(Contains<TTuple<double, int, float, char>, char>::value == true);

// static_assert(Contains<TTuple<>, char>::value == false);

// static_assert(Contains<TTuple<double, int, float>, char>::value == false);

template<TypeTuple Tuple>
struct TupleToVariant;

template<typename... Ts>
struct TupleToVariant<TTuple<Ts...>> {
  using Result = std::variant<Ts...>;
};
