#pragma once

#include <tuple>
#include <variant>

namespace event_sauce {
/*******************************************************************************
 ** Router
 **
 ** The router routes all the published events out to the different subscribers
 *******************************************************************************/
namespace router_detail {

template <typename... Ts> struct RouterImpl {
private:
  std::tuple<Ts...> subscribers;

  struct Visitor {
    Visitor(RouterImpl &router) : router{router} {}
    RouterImpl &router;

    template <typename T> void operator()(const T &value) {
      router.publish(value);
    }
  };

public:
  RouterImpl(Ts &&... ts) : subscribers{std::move(ts)...} {}

  template <typename S> RouterImpl<Ts..., S> subscribe(S &&sub) {
    return {std::move(std::get<Ts>(subscribers))..., std::move(sub)};
  }

  template <typename T> void publish(const T &evt) {
    auto wrap = [&evt](auto &sub) {
      sub(evt);
      return 0;
    };

    std::apply([&wrap](auto &&... xs) { return std::make_tuple(wrap(xs)...); },
               subscribers);
  }

  template <typename... Us> void publish(std::variant<Us...> &&v) {
    std::visit(Visitor(*this), v);
  }

  template <typename... Us> void publish(std::tuple<Us...> &&v) {
    auto wrap = [this](auto &&evt) {
      this->publish(evt);
      return 0;
    };
    std::apply([&wrap](auto &&... xs) { return std::make_tuple(wrap(xs)...); },
               v);
  }
};
} // namespace router_detail

struct Router : public router_detail::RouterImpl<> {
  Router() {}
};

inline Router make_router() { return {}; }

/*******************************************************************************
 ** Context
 *******************************************************************************/

// wrapper for an aggregate used for selecting the correct apply() function
template <typename Aggregate> struct AggregateApply {
  using state_type = typename Aggregate::state_type;
  state_type &state;
  AggregateApply(state_type &state) : state{state} {}

  template <typename Event> auto operator()(const Event &evt) {
    return (*this)(evt, 0);
  }

  template <typename Event>
  auto operator()(const Event &evt, int)
      -> decltype(Aggregate::apply(state, evt)) {
    state = Aggregate::apply(static_cast<const state_type &>(state), evt);
    return state;
  }

  template <typename Event> void operator()(const Event &evt, long) {
    /* Do nothing */
  }
};

// wrapper for an aggregate used for selecting the correct execute() function
template <typename Aggregate> struct AggregateExecute {
  using state_type = typename Aggregate::state_type;

  state_type &state;
  AggregateExecute(state_type &state) : state{state} {}

  template <typename Command> auto operator()(const Command &cmd) const {
    return (*this)(cmd, 0);
  }

  template <typename Command>
  auto operator()(const Command &cmd, int) const
      -> decltype(Aggregate::execute(state, cmd)) {
    return Aggregate::execute(state, cmd);
  }

  template <typename Command> auto operator()(const Command &evt, long) const {
    return std::variant<std::monostate>{};
  }
};

template <typename Aggregate, typename ReadModel> struct AggregateProject {
  ReadModel &read_model;

  AggregateProject(ReadModel &read_model) : read_model{read_model} {}

  template <typename Event> void operator()(const Event &evt) {
    (*this)(evt, 0);
  }

  template <typename Event>
  auto operator()(const Event &evt, int)
      -> decltype(Aggregate::project(read_model, evt)) {
    return Aggregate::project(read_model, evt);
  }

  template <typename Event> void operator()(const Event &evt, long) {
    /* Do nothing */
  }
};

// utility function to accumulate over a tuple
template <typename Op, typename Val, typename T, typename... Ts>
inline auto tuple_accumulate(Op op, Val val, std::tuple<T, Ts...> &&tuple) {
  const auto next_val = op(val, std::move(std::get<0>(tuple)));
  if constexpr (sizeof...(Ts) == 0) {
    return next_val;
  } else {
    auto tail = std::make_tuple(std::get<Ts>(tuple)...);
    return tuple_accumulate(op, next_val, std::move(tail));
  }
}

// binds aggregates and their states and recursively creates a router
template <typename State, typename ReadModel, typename Aggregate,
          typename... Aggregates>
inline auto bind(State &state, ReadModel &read_model, Aggregate &&agg,
                 Aggregates &&... aggs) {

  auto bind_aggregate = [&state, &read_model](auto &&aggregate) {
    return
        [&state, &read_model, aggregate = std::move(aggregate)](auto &&router) {
          auto &s = std::get<typename decltype(aggregate)::state_type>(state);
          return router.subscribe(AggregateApply<decltype(aggregate)>{s})
              .subscribe(
                  AggregateProject<decltype(aggregate), ReadModel>{read_model});
        };
  };
  auto initial = bind_aggregate(std::move(agg))(make_router());
  if constexpr (sizeof...(Aggregates) > 0) {
    auto binders = std::make_tuple(bind_aggregate(std::move(aggs))...);
    auto op = [](auto acc, auto binder) { return binder(acc); };
    return tuple_accumulate(op, initial, std::move(binders));
  } else {
    return initial;
  }
}

// the context is the api endpoint that the user of the library interacts with
template <typename ReadModel, typename... Aggregates> struct Context {
  using read_model_type = ReadModel;
  using state_type = std::tuple<typename Aggregates::state_type...>;
  using router_type =
      decltype(bind(std::declval<state_type &>(), std::declval<ReadModel &>(),
                    std::declval<Aggregates>()...));

  read_model_type &read_model;
  state_type state;
  router_type router;

  Context(read_model_type &read_model)
      : read_model{read_model}, state{}, router{bind(state, read_model,
                                                     Aggregates{}...)} {}

  template <typename Command> void dispatch(const Command &cmd) {
    auto events = std::make_tuple(AggregateExecute<Aggregates>{
        std::get<typename Aggregates::state_type>(state)}(cmd)...);

    // Publish events
    auto wrap = [this](auto &&evt) {
      this->router.publish(std::move(evt));
      return 0;
    };

    std::apply(
        [&wrap](auto &&... xs) {
          return std::make_tuple(wrap(std::move(xs))...);
        },
        events);
  }

  template <typename Aggregate> auto inspect() const {
    return std::get<typename Aggregate::state_type>(state);
  }
};

template <typename... Aggregates, typename ReadModel>
auto make_context(ReadModel &read_model) {
  return Context<ReadModel, Aggregates...>{read_model};
}

inline static const auto disabled = [] {};
} // namespace event_sauce