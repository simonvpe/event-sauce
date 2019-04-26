#pragma once

#include <event-sauce/fx/tuple-execute.hpp>
#include <event-sauce/fx/tuple-foldl.hpp>
#include <event-sauce/fx/tuple-invoke.hpp>
#include <event-sauce/misc/type_traits.hpp>
#include <tuple>
#include <variant>

namespace event_sauce {

namespace detail {

struct default_dispatcher_type
{
  auto serial()
  {
    return [](auto&& fn) { fn(); };
  }

  auto concurrent()
  {
    // ?????????????????????????????????
  }
};

struct default_projector_type
{
  template<typename Event>
  void operator()(const Event&)
  {}
};

template<typename Aggregate>
using substate_type = typename Aggregate::state_type;

template<typename Aggregate>
using substate_or_monostate_type = typename detected_or<std::monostate, substate_type, Aggregate>::type;

template<typename Aggregate>
static constexpr auto has_substate = is_detected<substate_type, Aggregate>::value;

template<typename... Aggregates>
using state_type = std::tuple<substate_or_monostate_type<Aggregates>...>;

template<typename Aggregate>
constexpr void
assert_has_substate(const Aggregate&)
{
  static_assert(has_substate<Aggregate>, "Aggregate does not define a 'substate_type' type.");
}

//////////////////////////////////////////////////////////////////////////////
// EXECUTE
//////////////////////////////////////////////////////////////////////////////
template<typename... Ts>
constexpr auto
event_count(const std::tuple<Ts...>& events)
{
  return (0 + ... + (std::is_same_v<Ts, std::monostate> ? 0 : 1));
}

template<typename Aggregate, typename Command, typename State = substate_type<Aggregate>>
using execute_result_type = decltype(Aggregate::execute(std::declval<State>(), std::declval<Command>()));

template<typename Aggregate, typename Command>
constexpr auto can_execute = is_detected<execute_result_type, Aggregate, Command>::value;

// execute :: () -> state -> command -> [event | monostate]
template<typename Dispatcher, typename... Aggregates>
constexpr auto
execute(Dispatcher&& dispatcher)
{
  return [&dispatcher](const state_type<Aggregates...>& state, const auto& cmd) {
    // functions :: [state -> cmd -> event | monostate]
    auto functions = std::make_tuple([agg = Aggregates{}](const state_type<Aggregates...>& state, const auto& cmd) {
      assert_has_substate(agg);
      if constexpr (can_execute<decltype(agg), decltype(cmd)>) {
        const auto& substate = std::get<substate_type<decltype(agg)>>(state);
        return decltype(agg)::execute(substate, cmd);
      } else {
        return std::monostate{};
      }
    }...);
    auto events = tuple_invoke(std::move(functions), state, cmd);
    constexpr auto nof_events = detail::event_count(events);
    static_assert(nof_events > 0, "Unhandled command");
    static_assert(nof_events < 2, "Command handled more than once");
    return std::move(events);
  };
}

//////////////////////////////////////////////////////////////////////////////
// APPLY
//////////////////////////////////////////////////////////////////////////////
template<typename Aggregate, typename Event, typename State = substate_type<Aggregate>>
using apply_result_type = decltype(Aggregate::apply(std::declval<State>(), std::declval<Event>()));

template<typename Aggregate, typename Event, typename State = substate_type<Aggregate>>
constexpr auto can_apply = is_detected_convertible<State, apply_result_type, Aggregate, Event>::value;

// apply :: () -> state -> event -> state
template<typename Dispatcher, typename... Aggregates>
constexpr auto
apply(Dispatcher&& dispatcher)
{
  return [](const state_type<Aggregates...>& state, const auto& evt) {
    // functions :: [state -> evt -> substate]
    auto functions = std::make_tuple([agg = Aggregates{}](const state_type<Aggregates...>& state, const auto& evt) {
      assert_has_substate(agg);
      const auto& substate = std::get<substate_type<decltype(agg)>>(state);
      if constexpr (can_apply<decltype(agg), decltype(evt)>) {
        return decltype(agg)::apply(substate, evt);
      } else {
        return substate;
      }
    }...);
    return tuple_invoke(std::move(functions), state, evt);
  };
}

//////////////////////////////////////////////////////////////////////////////
// PROCESS
//////////////////////////////////////////////////////////////////////////////
template<typename Aggregate, typename Event, typename State = substate_type<Aggregate>>
using process_result_type = decltype(Aggregate::process(std::declval<State>(), std::declval<Event>()));

template<typename Aggregate, typename Event>
constexpr auto can_process = is_detected<process_result_type, Aggregate, Event>::value;

// process :: () -> state -> event -> [command | monostate]
template<typename Dispatcher, typename... Aggregates>
constexpr auto
process(Dispatcher&& dispatcher)
{
  return [](const state_type<Aggregates...>& state, const auto& evt) {
    // functions :: [state -> evt -> command | monostate]
    auto functions = std::make_tuple([agg = Aggregates{}](const state_type<Aggregates...>& state, const auto& evt) {
      assert_has_substate(agg);
      if constexpr (can_process<decltype(agg), decltype(evt)>) {
        const auto& substate = std::get<substate_type<decltype(agg)>>(state);
        return decltype(agg)::process(substate, evt);
      } else {
        return std::monostate{};
      }
    }...);
    return tuple_invoke(std::move(functions), state, evt);
  };
}

//////////////////////////////////////////////////////////////////////////////
// UTIL
//////////////////////////////////////////////////////////////////////////////
template<typename T>
const T&
const_ref(T& ref)
{
  return const_cast<const T&>(ref);
}
} // namespace event_sauce::detail

// TODO: For some reason unwrap() can't be in the 'detail' namespace
template<typename Fn>
void
unwrap(const std::monostate&, Fn&&)
{}

template<typename Fn, typename... Ts>
void
unwrap(const std::variant<Ts...>& x, Fn&& fn)
{
  std::visit([&](const auto& x) { return unwrap(x, std::forward<Fn>(fn)); }, x);
}

template<typename Fn, typename T>
void
unwrap(const std::vector<T>& xs, Fn&& fn)
{
  for (const auto& x : xs) {
    unwrap(x, std::forward<Fn>(fn));
  }
}

template<typename Fn, typename T>
void
unwrap(const std::optional<T>& x, Fn&& fn)
{
  if (x) {
    unwrap(*x, std::forward<Fn>(fn));
  }
}

template<typename Fn, typename... Ts>
void
unwrap(const std::tuple<Ts...>& xs, Fn&& fn)
{
  tuple_execute(xs, [&](const auto& x) { unwrap(x, std::forward<Fn>(fn)); });
}

template<typename Fn, typename T>
void
unwrap(const T& x, Fn&& fn)
{
  fn(x);
}

template<typename Fn>
auto
unwrapper(Fn&& fn)
{
  return [fn = std::forward<Fn>(fn)](const auto& x) mutable { unwrap(x, std::forward<Fn>(fn)); };
}

template<typename... Aggregates>
struct context_type
{
  std::tuple<typename Aggregates::state_type...> state;

#ifndef NDEBUG
  template<typename Aggregate>
  auto inspect() const
  {
    return std::get<typename Aggregate::state_type>(state);
  }
#endif
};

template<typename... Aggregates>
auto
make_context()
{
  return context_type<Aggregates...>{};
}

template<typename... Aggregates,
         typename Projector = detail::default_projector_type,
         typename Dispatcher = detail::default_dispatcher_type>
auto
publish(context_type<Aggregates...>& ctx,
        Projector&& projector = detail::default_projector_type{},
        Dispatcher&& dispatcher = detail::default_dispatcher_type{})
{
  using namespace detail;
  return unwrapper([&](const auto& evt) mutable {
    ctx.state = apply<Dispatcher, Aggregates...>(std::forward<Dispatcher>(dispatcher))(ctx.state, evt);
    projector(evt);
    const auto commands =
      process<Dispatcher, Aggregates...>(std::forward<Dispatcher>(dispatcher))(const_ref(ctx).state, evt);
    dispatch(ctx, std::forward<Projector>(projector), std::forward<Dispatcher>(dispatcher))(commands);
  });
}

template<typename... Aggregates,
         typename Projector = detail::default_projector_type,
         typename Dispatcher = detail::default_dispatcher_type>
auto
dispatch(context_type<Aggregates...>& ctx,
         Projector&& projector = detail::default_projector_type{},
         Dispatcher&& dispatcher = detail::default_dispatcher_type{})
{
  using namespace detail;
  return unwrapper([&](const auto& cmd) mutable {
    dispatcher.serial()([&, cmd] {
      const auto events =
        execute<Dispatcher, Aggregates...>(std::forward<Dispatcher>(dispatcher))(const_ref(ctx).state, cmd);
      publish(ctx, std::forward<Projector>(projector), std::forward<Dispatcher>(dispatcher))(events);
    });
  });
};
} // namespace event_sauce
