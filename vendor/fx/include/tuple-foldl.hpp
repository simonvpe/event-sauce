#pragma once
#include <tuple>

/*
template<typename Op, typename Val, typename T, typename... Ts>
inline auto
tuple_foldl(Op op, Val val, const std::tuple<T, Ts...>& tuple)
{
  const auto next_val = op(val, std::get<0>(tuple));
  if constexpr (sizeof...(Ts) == 0) {
    return next_val;
  } else {
    auto tail = std::make_tuple(std::get<Ts>(tuple)...);
    return tuple_foldl(op, next_val, std::move(tail));
  }
}
*/

template<typename Op, typename Acc, typename Tuple, std::size_t Head, std::size_t... Tail>
inline auto
tuple_foldl(Op op, Acc acc, Tuple tuple, std::index_sequence<Head, Tail...>)
{
  const auto next_acc = op(acc, std::get<Head>(tuple));
  if constexpr (sizeof...(Tail) == 0) {
    return next_acc;
  } else {
    return tuple_foldl(op, next_acc, tuple, std::index_sequence<Tail...>{});
  }
}

template<typename Op, typename Acc, typename... Ts>
inline auto
tuple_foldl(Op op, Acc acc, std::tuple<Ts...> tuple)
{
  return tuple_foldl(op, acc, tuple, std::index_sequence_for<Ts...>{});
}
