#pragma once
#include <tuple>

template<typename T, typename F>
auto tuple_map(T&& target, F&& fn) {
  return std::apply([&fn](auto && ... x) {
      return std::make_tuple(fn(std::forward<decltype(x)>(x))...);
    }, std::forward<T>(target));
}
