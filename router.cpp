#include <iostream>
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

// utility function to accumulate over a tuple
template <typename Op, typename Val, typename T, typename... Ts>
inline auto tuple_accumulate(Op op, Val val, std::tuple<T, Ts...> &&tuple) {
  const auto next_val = op(val, std::move(std::get<0>(tuple)));
  if constexpr (sizeof...(Ts) == 0) {
    return next_val;
  } else {
    auto tail = std::make_tuple(std::get<Ts>(tuple)...);
    return tuple_accumulate(op, next_val, tail);
  }
}

// binds aggregates and their states and recursively creates a router
template <typename State, typename Aggregate, typename... Aggregates>
inline auto bind(State &state, Aggregate &&agg, Aggregates &&... aggs) {

  auto bind_aggregate = [&state](auto &&aggregate) {
    return [&state, aggregate = std::move(aggregate)](auto &&router) {
      auto &s = std::get<typename decltype(aggregate)::state_type>(state);
      return router.subscribe(AggregateApply<decltype(aggregate)>{s});
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
template <typename... Aggregates> struct Context {
  using state_type = std::tuple<typename Aggregates::state_type...>;
  using router_type = decltype(
      bind(std::declval<state_type &>(), std::declval<Aggregates>()...));

  state_type state;
  router_type router;

  Context() : state{}, router{bind(state, Aggregates{}...)} {}

  template <typename Command> void dispatch(const Command &cmd) {
    auto events = std::make_tuple(AggregateExecute<Aggregates>{
        std::get<typename Aggregates::state_type>(state)}(cmd)...);

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

template <typename... Aggregates> auto make_context() {
  return Context<Aggregates...>{};
}

inline static const auto disabled = [] {};
} // namespace event_sauce

/*******************************************************************************
 ** Example
 *******************************************************************************/
#include <chrono>
#include <cmath>

// Commands

struct PressRightKey {
  static constexpr auto acceleration = 1.0f;
};

struct ReleaseRightKey {
  static constexpr auto acceleration = -PressRightKey::acceleration;
};

struct PressLeftKey {
  static constexpr auto acceleration = -PressRightKey::acceleration;
};

struct ReleaseLeftKey {
  static constexpr auto acceleration = -PressLeftKey::acceleration;
};

struct Tick {
  std::chrono::milliseconds dt;
};

// Events

struct PlayerAccelerationChanged {
  float acc_x, acc_y;
};

struct PlayerMoved {
  long pos_x, pos_y;
  float vel_x, vel_y;
};

struct PlayerPositionChanged {
  long pos_x, pos_y;
};

// The state of the player
struct Player {

  struct state_type {
    long pos_x = 10, pos_y = 10;
    float vel_x = 0.0f, vel_y = 0.0f;
    float acc_x = 0.0f, acc_y = 0.0f;
  };

  // execute(PressRightKey)
  static constexpr PlayerAccelerationChanged execute(const state_type &state,
                                                     const PressRightKey &) {
    return {state.acc_x + PressRightKey::acceleration, state.acc_y};
  }

  // execute(ReleaseRightKey)
  static constexpr PlayerAccelerationChanged execute(const state_type &state,
                                                     const ReleaseRightKey &) {
    return {state.acc_x + ReleaseRightKey::acceleration, state.acc_y};
  }

  // execute(PressLeftKey)
  static constexpr PlayerAccelerationChanged execute(const state_type &state,
                                                     const PressLeftKey &) {
    return {state.acc_x + PressLeftKey::acceleration, state.acc_y};
  }

  // execute(ReleaseLeftKey)
  static constexpr PlayerAccelerationChanged execute(const state_type &state,
                                                     const ReleaseLeftKey &) {
    return {state.acc_x + ReleaseLeftKey::acceleration, state.acc_y};
  }

  // execute(Tick)
  static constexpr PlayerMoved execute(const state_type &state,
                                       const Tick &event) {
    const auto dt = event.dt.count();
    const auto pos_x = state.pos_x + lround(state.vel_x * dt);
    const auto pos_y = state.pos_y + lround(state.vel_y * dt);
    const auto vel_x = state.vel_x + state.acc_x * dt;
    const auto vel_y = state.vel_y + state.acc_y * dt;
    return {pos_x, pos_y, vel_x, vel_y};
  }

  // apply(PlayerAccelerationChanged)
  static constexpr state_type apply(const state_type &state,
                                    const PlayerAccelerationChanged &event) {
    return {state.pos_x, state.pos_y, state.vel_x,
            state.vel_y, event.acc_x, event.acc_y};
  }

  // apply(PlayerMoved)
  static constexpr state_type apply(const state_type &state,
                                    const PlayerMoved &event) {
    return {event.pos_x, event.pos_y, event.vel_x,
            event.vel_y, state.acc_x, state.acc_y};
  }
};

// The state of the map
struct Map {
  static constexpr auto execute = event_sauce::disabled;

  struct state_type {
    long scroll = 0;
  };

  // apply(PlayerMoved)
  static constexpr state_type apply(const state_type &state,
                                    const PlayerMoved &event) {
    return {event.pos_x};
  }
};

int main() {
  using namespace std::chrono_literals;
  auto ctx = event_sauce::make_context<Player, Map>();
  ctx.dispatch(PressRightKey{});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(ReleaseRightKey{});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(PressLeftKey{});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});

  std::cout << "Player [x=" << ctx.inspect<Player>().pos_x
            << ", y=" << ctx.inspect<Player>().pos_y << "]\n";
  std::cout << "Map    [scroll=" << ctx.inspect<Map>().scroll << "]\n";

  return 0;
}
