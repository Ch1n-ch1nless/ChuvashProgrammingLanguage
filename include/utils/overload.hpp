#pragma once

/*

Usefull struct to create overloaded lambdas, needed to handle different types
inside std::variant. Taken from here:
https://dev.to/tmr232/that-overloaded-trick-overloading-lambdas-in-c17

*/

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
