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

template<typename Aggregate>
using substate_type = typename Aggregate::state_type;

template<typename Aggregate>
using substate_or_monostate_type = typename detected_or<std::monostate, substate_type, Aggregate>::type;

template<typename Aggregate>
static constexpr auto has_substate = is_detected<substate_type, Aggregate>::value;
    
template<typename... Aggregates>
using state_type = std::tuple<substate_or_monostate_type<Aggregates>...>;

template<typename Aggregate>
constexpr void assert_has_substate(const Aggregate&) 
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
    auto events =  tuple_invoke(std::move(functions), state, cmd);
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
  return [](state_type<Aggregates...>& state, const auto& evt) {
    // functions :: [state -> evt -> command | monostate]
    auto functions = std::make_tuple([agg = Aggregates{}](state_type<Aggregates...>& state, const auto& evt) {
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
// PROJECT
//////////////////////////////////////////////////////////////////////////////
template<typename Aggregate, typename Event>
using project_result_type = decltype(Aggregate::project(std::declval<Event>()));

template<typename Aggregate, typename Event>
constexpr auto can_project = is_detected<project_result_type, Aggregate, Event>::value;

// project :: () -> read_model -> event -> IO(read_model)
template<typename Dispatcher, typename... Aggregates>
constexpr auto
project(Dispatcher&& dispatcher)
{
  return [](auto& read_model, auto& evt) {
    // projectors :: evt -> [read_model -> IO(read_model)]
    auto projectors = [](const auto& evt) {
      // functions :: [evt -> (read_model -> IO(read_model))]
      auto functions = std::make_tuple([agg = Aggregates{}](const auto& evt) {
        if constexpr (can_project<decltype(agg), decltype(evt)>) {
          return decltype(agg)::project(evt);
        } else {
          return [](auto&) { return 0; };
        }
      }...);
      return tuple_invoke(std::move(functions), evt);
    };
    tuple_invoke(projectors(evt), read_model);
  };
}
}

//////////////////////////////////////////////////////////////////////////////
// CONTEXT
//////////////////////////////////////////////////////////////////////////////    
template<typename ReadModel, typename... Aggregates>
class context
{
public:
  //////////////////////////////////////////////////////////////////////////////
  // DEFINITIONS
  //////////////////////////////////////////////////////////////////////////////
  using read_model_type = ReadModel;
  using state_type = detail::state_type<Aggregates...>;

  //////////////////////////////////////////////////////////////////////////////
  // STATE
  //////////////////////////////////////////////////////////////////////////////
  read_model_type& read_model;
  state_type state;
  unsigned long long last_event_id;

  //////////////////////////////////////////////////////////////////////////////
  // CONSTRUCTORS
  //////////////////////////////////////////////////////////////////////////////
  context(read_model_type& read_model)
      : read_model{ read_model }
      , state{}
      , last_event_id{ 0 }
  {}

  //////////////////////////////////////////////////////////////////////////////
  // DISPATCH
  //////////////////////////////////////////////////////////////////////////////
  template<typename Dispatcher = detail::default_dispatcher_type>
  void dispatch(const std::monostate&, Dispatcher&& = detail::default_dispatcher_type{})
  {}

  template<typename... Ts, typename Dispatcher = detail::default_dispatcher_type>
  void dispatch(const std::variant<Ts...>& cmd, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    auto fn = [this, &dispatcher](const auto& cmd) { this->dispatch(cmd, std::forward<Dispatcher>(dispatcher)); };
    std::visit(std::move(fn), cmd);
  }

  template<typename... Ts, typename Dispatcher = detail::default_dispatcher_type>
  void dispatch(const std::tuple<Ts...>& cmds, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    auto fn = [this, &dispatcher](const auto& cmd) { this->dispatch(cmd, std::forward<Dispatcher>(dispatcher)); };
    tuple_execute(cmds, std::move(fn));
  }

  template<typename T, typename Dispatcher = detail::default_dispatcher_type>
  void dispatch(const std::vector<T>& cmds, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    auto fn = [this, &dispatcher](const auto& cmd) { this->dispatch(cmd, std::forward<Dispatcher>(dispatcher)); };
    std::for_each(cbegin(cmds), cend(cmds), std::move(fn));
  }

  template<typename T, typename Dispatcher = detail::default_dispatcher_type>
  void dispatch(const std::optional<T>& cmd, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    if (cmd) {
      dispatch(*cmd, std::forward<Dispatcher>(dispatcher));
    }
  }

  template<typename Command, typename Dispatcher = detail::default_dispatcher_type>
  void dispatch(const Command& cmd, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    dispatcher.serial()([this, cmd, &dispatcher]() mutable {
      const auto execute = detail::execute<Dispatcher, Aggregates...>(std::forward<Dispatcher>(dispatcher));
      const auto events = execute(state, cmd);
      constexpr auto nof_events = detail::event_count(events);
      static_assert(nof_events > 0, "Unhandled command");
      static_assert(nof_events < 2, "Command handled more than once");
      publish(events, std::forward<Dispatcher>(dispatcher));
    });
  }

  //////////////////////////////////////////////////////////////////////////////
  // PUBLISH
  //////////////////////////////////////////////////////////////////////////////
  template<typename Dispatcher>
  void publish(const std::monostate&, Dispatcher&& = detail::default_dispatcher_type{})
  {}

  template<typename... Ts, typename Dispatcher>
  void publish(const std::variant<Ts...>& evt, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    auto fn = [this, &dispatcher](const auto& evt) { this->publish(evt, std::forward<Dispatcher>(dispatcher)); };
    std::visit(std::move(fn), evt);
  }

  template<typename... Ts, typename Dispatcher>
  void publish(const std::tuple<Ts...>& evts, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    auto fn = [this, &dispatcher](const auto& evt) { this->publish(evt, std::forward<Dispatcher>(dispatcher)); };
    tuple_execute(evts, std::move(fn));
  }

  template<typename T, typename Dispatcher>
  void publish(const std::vector<T>& evts, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    auto fn = [this, &dispatcher](const auto& evt) { this->publish(evt, std::forward<Dispatcher>(dispatcher)); };
    std::for_each(cbegin(evts), cend(evts), std::move(fn));
  }

  template<typename T, typename Dispatcher>
  void publish(const std::optional<T>& evt, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    if (evt) {
      publish(*evt, std::forward<Dispatcher>(dispatcher));
    }
  }

  template<typename Event, typename Dispatcher>
  void publish(const Event& evt, Dispatcher&& dispatcher = detail::default_dispatcher_type{})
  {
    const auto apply = detail::apply<Dispatcher, Aggregates...>(std::forward<Dispatcher>(dispatcher));
    const auto project = detail::project<Dispatcher, Aggregates...>(std::forward<Dispatcher>(dispatcher));
    const auto process = detail::process<Dispatcher, Aggregates...>(std::forward<Dispatcher>(dispatcher));

    state = apply(state, evt);
    ++last_event_id;
    project(read_model, evt);
    const auto commands = process(state, evt);
    dispatch(commands, std::forward<Dispatcher>(dispatcher));
  }

//////////////////////////////////////////////////////////////////////////////
// DEBUGGING
//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
  template<typename Aggregate>
  detail::substate_type<Aggregate> inspect() const
  {
    return std::get<detail::substate_type<Aggregate>>(state);
  }
#endif
};
} // namespace event_sauce
