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
#include "units.h"
#include <chrono>
#include <cmath>

using namespace units::literals;
using namespace units::angle;
using namespace units::angular_velocity;
using namespace units::acceleration;
using namespace units::force;
using namespace units::time;
using namespace units::math;
using namespace units::mass;
using namespace units::velocity;
using namespace units::length;
using namespace units::torque;

using mps_sq_t = meters_per_second_squared_t;
using mps_t = meters_per_second_t;
using kilogram_meters_squared_t = decltype(1.0_kg * 1.0_sq_m);
using radians_per_second_squared_t = decltype(1.0_rad / 1.0_s / 1.0_s);

template <typename T> struct tensor {
  T x, y;
  tensor operator+(const tensor &other) { return {x + other.x, y + other.y}; }
};
// Commands
struct ActivateMainThruster {
  static constexpr newton_t thrust = 1.0_N;
};

struct DeactivateMainThruster {
  static constexpr newton_t thrust = -ActivateMainThruster::thrust;
};

struct ActivateLeftThruster {
  static constexpr newton_meter_t torque = 1.0_Nm;
};

struct DeactivateLeftThruster {
  static constexpr newton_meter_t torque = -ActivateLeftThruster::torque;
};

struct ActivateRightThruster {
  static constexpr newton_meter_t torque = -1.0_Nm;
};

struct DeactivateRightThruster {
  static constexpr newton_meter_t torque = -ActivateRightThruster::torque;
};

struct Tick {
  second_t dt;
};

// Events

struct ThrustChanged {
  newton_t thrust;
};

struct TorqueChanged {
  newton_meter_t torque;
};

struct VelocityChanged {
  tensor<mps_t> velocity;
  radians_per_second_t angular_velocity;
};

struct PositionChanged {
  tensor<meter_t> position;
  radian_t rotation;
};

/*******************************************************************************
 ** Player
 *******************************************************************************/
struct Player {
public:
  // State
  struct state_type {
    static constexpr auto initial_inertia = kilogram_meters_squared_t{1.0};
    kilogram_t mass = 1.0_kg;
    kilogram_meters_squared_t inertia = initial_inertia;
    newton_meter_t torque = 0_Nm;
    newton_t thrust = 0.0_N;
    radian_t rotation = 0_rad;
    tensor<mps_t> velocity = {0_mps, 0_mps};
    radians_per_second_t angular_velocity = 0_rad_per_s;
    tensor<meter_t> position = {0.0_m, 0.0_m};
  };

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateMainThruster -> ThrustChanged
  static constexpr ThrustChanged execute(const state_type &state,
                                         const ActivateMainThruster &event) {
    return {state.thrust + ActivateMainThruster::thrust};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute DeactivateMainThruster -> ThrustChanged
  static constexpr ThrustChanged execute(const state_type &state,
                                         const DeactivateMainThruster &event) {
    return {state.thrust + DeactivateMainThruster::thrust};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateLeftThruster -> TorqueChanged
  static constexpr TorqueChanged execute(const state_type &state,
                                         const ActivateLeftThruster &event) {
    return {state.torque + ActivateLeftThruster::torque};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute DeactivateLeftThruster -> TorqueChanged
  static constexpr TorqueChanged execute(const state_type &state,
                                         const DeactivateLeftThruster &event) {
    return {state.torque + DeactivateLeftThruster::torque};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateRightThruster -> TorqueChanged
  static constexpr TorqueChanged execute(const state_type &state,
                                         const ActivateRightThruster &event) {
    return {state.torque + ActivateRightThruster::torque};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute DeactivateRightThruster -> TorqueChanged
  static constexpr TorqueChanged execute(const state_type &state,
                                         const DeactivateRightThruster &event) {
    return {state.torque + DeactivateRightThruster::torque};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute Tick -> VelocityChanged & PositionChanged
  static constexpr std::tuple<VelocityChanged, PositionChanged>
  execute(const state_type &state, const Tick &event) {
    const auto velocity = update_velocity(state, event);
    const auto angular_velocity = update_angular_velocity(state, event);
    const auto position = update_position(state, event);
    const auto rotation = update_rotation(state, event);
    return {{velocity, angular_velocity}, {position, rotation}};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply ThrustChanged
  static constexpr state_type apply(const state_type &state,
                                    const ThrustChanged &event) {
    state_type next = state;
    next.thrust = event.thrust;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply TorqueChanged
  static constexpr state_type apply(const state_type &state,
                                    const TorqueChanged &event) {
    state_type next = state;
    next.torque = event.torque;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply VelocityChanged
  static constexpr state_type apply(const state_type &state,
                                    const VelocityChanged &event) {
    state_type next = state;
    next.velocity = event.velocity;
    next.angular_velocity = event.angular_velocity;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static constexpr state_type apply(const state_type &state,
                                    const PositionChanged &event) {
    state_type next = state;
    next.position = event.position;
    next.rotation = event.rotation;
    return next;
  }

private:
  //////////////////////////////////////////////////////////////////////////////
  // update_velocity
  static constexpr tensor<mps_t> update_velocity(const state_type &state,
                                                 const Tick &event) {
    const mps_t velocity_change_abs = (state.thrust / state.mass) * event.dt;
    return {state.velocity.x + cos(state.rotation) * velocity_change_abs,
            state.velocity.y + sin(state.rotation) * velocity_change_abs};
  }

  //////////////////////////////////////////////////////////////////////////////
  // update_angular_velocity
  static constexpr radians_per_second_t
  update_angular_velocity(const state_type &state, const Tick &event) {
    // TODO: This should be type safe, but idk how to make units understand how
    // to convert from [N/kg/m] to [rad/s^2]
    const auto v = (state.torque / state.inertia).to<float>();
    const radians_per_second_squared_t angular_acc{v};
    return state.angular_velocity + angular_acc * event.dt;
  }

  //////////////////////////////////////////////////////////////////////////////
  // update_position
  static constexpr tensor<meter_t> update_position(const state_type &state,
                                                   const Tick &event) {
    return {state.position.x + state.velocity.x * event.dt,
            state.position.y + state.velocity.y * event.dt};
  }

  //////////////////////////////////////////////////////////////////////////////
  // update_rotation
  static constexpr radian_t update_rotation(const state_type &state,
                                            const Tick &event) {
    return state.rotation + state.angular_velocity * event.dt;
  }
};

/*******************************************************************************
 ** PlayerProjection
 *******************************************************************************/
struct PlayerProjection {
  constexpr static auto execute = event_sauce::disabled;

  struct state_type {
    float x, y;
    float rotation;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static state_type apply(const state_type &state,
                          const PositionChanged &event) {
    state_type next = state;
    next.x = event.position.x.to<float>();
    next.y = event.position.y.to<float>();
    next.rotation = event.rotation.to<float>();

    std::cout << "x=" << event.position.x << " y=" << event.position.y
              << " angle=" << event.rotation << std::endl;
    return next;
  }
};

int main() {
  using namespace std::chrono_literals;
  auto ctx = event_sauce::make_context<Player, PlayerProjection>();
  ctx.dispatch(ActivateMainThruster{});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(ActivateLeftThruster{});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(DeactivateMainThruster{});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(ActivateMainThruster{});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});
  ctx.dispatch(Tick{100ms});

  return 0;
}
