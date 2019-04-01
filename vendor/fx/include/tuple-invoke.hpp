#pragma once
#include "tuple-map.hpp"

template<typename T, typename... Args>
auto
tuple_invoke(T&& target, Args&&... args)
{
  return std::apply(
    [&](auto&&... fn) {
      return std::make_tuple(std::invoke(std::forward<decltype(fn)>(fn), std::forward<Args>(args)...)...);
    },
    std::forward<T>(target));
}
