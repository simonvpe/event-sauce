#pragma once
#include <tuple>

template <typename Op, typename Val, typename T, typename... Ts>
inline auto tuple_foldl(Op op, Val val, std::tuple<T, Ts...> &&tuple) {
  const auto next_val = op(val, std::move(std::get<0>(tuple)));
  if constexpr (sizeof...(Ts) == 0) {
    return next_val;
  } else {
    auto tail = std::make_tuple(std::get<Ts>(tuple)...);
    return tuple_foldl(op, next_val, std::move(tail));
  }
}
