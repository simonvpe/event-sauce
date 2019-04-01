#pragma once

#include "tuple-invoke.hpp"
#include "type_traits.hpp"
#include <tuple-execute.hpp>
#include <tuple-foldl.hpp>
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
  void dispatch(const std::monostate&) {}

  template<typename... Ts>
  void dispatch(const std::variant<Ts...>& cmd)
  {
    std::visit([this](const auto& cmd) { this->dispatch(cmd); }, cmd);
  }

  template<typename... Ts>
  void dispatch(const std::tuple<Ts...>& cmds)
  {
    tuple_execute(cmds, [this](const auto& cmd) { this->dispatch(cmd); });
  }

  template<typename T>
  void dispatch(const std::vector<T>& cmds)
  {
    std::for_each(cbegin(cmds), cend(cmds), [this](const auto& cmd) { this->dispatch(cmd); });
  }

  template<typename T>
  void dispatch(const std::optional<T>& cmd)
  {
    if (cmd) {
      publish(*cmd);
    }
  }

  template<typename Command>
  void dispatch(const Command& cmd)
  {
    const auto events = execute(state, cmd);
    static_assert(detail::event_count(events) > 0, "Unhandled command");
    publish(events);
  }

  //////////////////////////////////////////////////////////////////////////////
  // PUBLISH
  //////////////////////////////////////////////////////////////////////////////
  void publish(const std::monostate&) {}

  template<typename... Ts>
  void publish(const std::variant<Ts...>& evt)
  {
    std::visit([this](const auto& evt) { this->publish(evt); }, evt);
  }

  template<typename... Ts>
  void publish(const std::tuple<Ts...>& evts)
  {
    tuple_execute(evts, [this](const auto& evt) { this->publish(evt); });
  }

  template<typename T>
  void publish(const std::vector<T>& evts)
  {
    std::for_each(cbegin(evts), cend(evts), [this](const auto& evt) { this->publish(evt); });
  }

  template<typename T>
  void publish(const std::optional<T>& evt)
  {
    if (evt) {
      publish(*evt);
    }
  }

  template<typename Event>
  void publish(const Event& evt)
  {
    state = apply(state, evt);
    ++last_event_id;
    project(read_model, evt);
    const auto commands = process(state, evt);
    dispatch(commands);
  }
};
} // namespace event_sauce
