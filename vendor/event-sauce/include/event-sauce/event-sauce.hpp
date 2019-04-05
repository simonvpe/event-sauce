#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <event-sauce/fx/tuple-execute.hpp>
#include <event-sauce/fx/tuple-foldl.hpp>
#include <event-sauce/fx/tuple-invoke.hpp>
#include <event-sauce/misc/type_traits.hpp>
#include <tuple>
#include <variant>

namespace event_sauce {

namespace detail {

template<typename... Aggregates>
using state_type = std::tuple<typename Aggregates::state_type...>;

//////////////////////////////////////////////////////////////////////////////
// EXECUTE
//////////////////////////////////////////////////////////////////////////////
template<typename Aggregate, typename Command, typename State = typename Aggregate::state_type>
using execute_result_type = decltype(Aggregate::execute(std::declval<State>(), std::declval<Command>()));

template<typename Aggregate, typename Command>
constexpr auto can_execute = is_detected<execute_result_type, Aggregate, Command>::value;

// execute :: () -> state -> command -> [event | monostate]
template<typename... Aggregates>
constexpr auto
execute()
{
  return [](const state_type<Aggregates...>& state, const auto& cmd) {
    // functions :: [state -> cmd -> event | monostate]
    auto functions = std::make_tuple([agg = Aggregates{}](const state_type<Aggregates...>& state, const auto& cmd) {
      if constexpr (can_execute<decltype(agg), decltype(cmd)>) {
        const auto& substate = std::get<typename decltype(agg)::state_type>(state);
        return decltype(agg)::execute(substate, cmd);
      } else {
        return std::monostate{};
      }
    }...);
    return tuple_invoke(std::move(functions), state, cmd);
  };
}

template<typename... Ts>
constexpr auto
event_count(const std::tuple<Ts...>& events)
{
  return (0 + ... + (std::is_same_v<Ts, std::monostate> ? 0 : 1));
}

//////////////////////////////////////////////////////////////////////////////
// APPLY
//////////////////////////////////////////////////////////////////////////////
template<typename Aggregate, typename Event, typename State = typename Aggregate::state_type>
using apply_result_type = decltype(Aggregate::apply(std::declval<State>(), std::declval<Event>()));

template<typename Aggregate, typename Event, typename State = typename Aggregate::state_type>
constexpr auto can_apply = is_detected_convertible<State, apply_result_type, Aggregate, Event>::value;

// apply :: () -> state -> event -> state
template<typename... Aggregates>
constexpr auto
apply()
{
  return [](const state_type<Aggregates...>& state, const auto& evt) {
    // functions :: [state -> evt -> substate]
    auto functions = std::make_tuple([agg = Aggregates{}](const state_type<Aggregates...>& state, const auto& evt) {
      const auto& substate = std::get<typename decltype(agg)::state_type>(state);
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
template<typename Aggregate, typename Event, typename State = typename Aggregate::state_type>
using process_result_type = decltype(Aggregate::process(std::declval<State>(), std::declval<Event>()));

template<typename Aggregate, typename Event>
constexpr auto can_process = is_detected<process_result_type, Aggregate, Event>::value;

// process :: () -> state -> event -> [command | monostate]
template<typename... Aggregates>
constexpr auto
process()
{
  return [](state_type<Aggregates...>& state, const auto& evt) {
    // functions :: [state -> evt -> command | monostate]
    auto functions = std::make_tuple([agg = Aggregates{}](state_type<Aggregates...>& state, const auto& evt) {
      if constexpr (can_process<decltype(agg), decltype(evt)>) {
        const auto& substate = std::get<typename decltype(agg)::state_type>(state);
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
template<typename... Aggregates>
constexpr auto
project()
{
  return [=](auto& read_model, auto& evt) {
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

// the context is the api endpoint that the user of the library interacts with
template<typename ReadModel, typename... Aggregates>
class context
{
  static constexpr auto execute = ::event_sauce::detail::execute<Aggregates...>();
  static constexpr auto apply = ::event_sauce::detail::apply<Aggregates...>();
  static constexpr auto process = ::event_sauce::detail::process<Aggregates...>();
  static constexpr auto project = ::event_sauce::detail::project<Aggregates...>();

public:
  //////////////////////////////////////////////////////////////////////////////
  // DEFINITIONS
  //////////////////////////////////////////////////////////////////////////////
  using read_model_type = ReadModel;
  using state_type = ::event_sauce::detail::state_type<Aggregates...>;
  static constexpr auto default_dispatcher = [](auto&& fn) { fn(); };
  using default_dispatcher_type = decltype(default_dispatcher);

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
  template<typename Dispatcher = default_dispatcher_type>
  void dispatch(const std::monostate&, Dispatcher = default_dispatcher)
  {}

  template<typename... Ts, typename Dispatcher = default_dispatcher_type>
  void dispatch(const std::variant<Ts...>& cmd, Dispatcher dispatcher = default_dispatcher)
  {
    std::visit([this, dispatcher](const auto& cmd) { this->dispatch(cmd, dispatcher); }, cmd);
  }

  template<typename... Ts, typename Dispatcher = default_dispatcher_type>
  void dispatch(const std::tuple<Ts...>& cmds, Dispatcher dispatcher = default_dispatcher)
  {
    tuple_execute(cmds, [this, dispatcher](const auto& cmd) { this->dispatch(cmd, dispatcher); });
  }

  template<typename T, typename Dispatcher = default_dispatcher_type>
  void dispatch(const std::vector<T>& cmds, Dispatcher dispatcher = default_dispatcher)
  {
    std::for_each(cbegin(cmds), cend(cmds), [this, dispatcher](const auto& cmd) { this->dispatch(cmd, dispatcher); });
  }

  template<typename T, typename Dispatcher = default_dispatcher_type>
  void dispatch(const std::optional<T>& cmd, Dispatcher dispatcher = default_dispatcher)
  {
    if (cmd) {
      dispatch(*cmd, dispatcher);
    }
  }

  template<typename Command, typename Dispatcher = default_dispatcher_type>
  void dispatch(const Command& cmd, Dispatcher dispatcher = default_dispatcher)
  {
    dispatcher([this, cmd, dispatcher = std::forward<decltype(dispatcher)>(dispatcher)] {
      const auto events = execute(state, cmd);
      constexpr auto nof_events = detail::event_count(events);
      static_assert(nof_events > 0, "Unhandled command");
      static_assert(nof_events < 2, "Command handled more than once");
      publish(events, dispatcher);
    });
  }

  //////////////////////////////////////////////////////////////////////////////
  // PUBLISH
  //////////////////////////////////////////////////////////////////////////////
  template<typename Dispatcher>
  void publish(const std::monostate&, Dispatcher = default_dispatcher)
  {}

  template<typename... Ts, typename Dispatcher>
  void publish(const std::variant<Ts...>& evt, Dispatcher dispatcher = default_dispatcher)
  {
    std::visit([this, dispatcher](const auto& evt) { this->publish(evt, dispatcher); }, evt);
  }

  template<typename... Ts, typename Dispatcher>
  void publish(const std::tuple<Ts...>& evts, Dispatcher dispatcher = default_dispatcher)
  {
    tuple_execute(evts, [this, dispatcher](const auto& evt) { this->publish(evt, dispatcher); });
  }

  template<typename T, typename Dispatcher>
  void publish(const std::vector<T>& evts, Dispatcher dispatcher = default_dispatcher)
  {
    std::for_each(cbegin(evts), cend(evts), [this, dispatcher](const auto& evt) { this->publish(evt, dispatcher); });
  }

  template<typename T, typename Dispatcher>
  void publish(const std::optional<T>& evt, Dispatcher dispatcher = default_dispatcher)
  {
    if (evt) {
      publish(*evt, dispatcher);
    }
  }

  template<typename Event, typename Dispatcher>
  void publish(const Event& evt, Dispatcher dispatcher = default_dispatcher)
  {
    state = apply(state, evt);
    ++last_event_id;
    project(read_model, evt);
    const auto commands = process(state, evt);
    dispatch(commands, dispatcher);
  }

//////////////////////////////////////////////////////////////////////////////
// DEBUGGING
//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
  template<typename Aggregate>
  typename Aggregate::state_type inspect() const
  {
    return std::get<typename Aggregate::state_type>(state);
  }
#endif
};
} // namespace event_sauce
