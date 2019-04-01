#pragma once
#include <event-sauce/fx/tuple-map.hpp>

template<typename T, typename F>
void
tuple_execute(T&& target, F&& fn)
{
  auto wrapper = [&fn](auto&& x) {
    fn(std::forward<decltype(x)>(x));
    return 0;
  };
  tuple_map(std::forward<T>(target), std::move(wrapper));
}
