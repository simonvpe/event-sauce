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

/*******************************************************************************
 ** Example
 *******************************************************************************/
#include "units.h"
#include <chrono>
#include <cmath>
#include <functional>

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

struct DeactivateMainThruster {};

struct ActivateLeftThruster {
  static constexpr newton_meter_t torque = 1.0_Nm;
};

struct DeactivateLeftThruster {};

struct ActivateRightThruster {
  static constexpr newton_meter_t torque = -1.0_Nm;
};

struct DeactivateRightThruster {};

struct Tick {
  second_t dt;
};

// Events

struct ThrustChanged {
  bool main_thruster_activated;
  newton_t thrust;
};

struct TorqueChanged {
  bool left_thruster_activated;
  bool right_thruster_activated;
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
  static constexpr auto project = event_sauce::disabled;
  // State
  struct state_type {
    static constexpr auto initial_inertia = kilogram_meters_squared_t{1.0};
    kilogram_t mass = 1.0_kg;
    kilogram_meters_squared_t inertia = initial_inertia;
    radian_t rotation = 0_rad;
    tensor<mps_t> velocity = {0_mps, 0_mps};
    radians_per_second_t angular_velocity = 0_rad_per_s;
    tensor<meter_t> position = {0.0_m, 0.0_m};
    newton_t thrust = 0_N;
    newton_meter_t torque = 0_Nm;
    bool left_thruster_activated = false;
    bool right_thruster_activated = false;
    bool main_thruster_activated = false;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateMainThruster -> (ThrustChanged)
  static constexpr std::variant<std::monostate, ThrustChanged>
  execute(const state_type &state, const ActivateMainThruster &event) {
    if (!state.main_thruster_activated) {
      return ThrustChanged{true, state.thrust + ActivateMainThruster::thrust};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute DeactivateMainThruster -> (ThrustChanged)
  static constexpr std::variant<std::monostate, ThrustChanged>
  execute(const state_type &state, const DeactivateMainThruster &event) {
    if (state.main_thruster_activated) {
      return ThrustChanged{false, state.thrust - ActivateMainThruster::thrust};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateLeftThruster -> (TorqueChanged)
  static constexpr std::variant<std::monostate, TorqueChanged>
  execute(const state_type &state, const ActivateLeftThruster &event) {
    if (!state.left_thruster_activated) {
      const auto right = state.right_thruster_activated;
      return TorqueChanged{true, right,
                           state.torque + ActivateLeftThruster::torque};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute DeactivateLeftThruster -> (TorqueChanged)
  static constexpr std::variant<std::monostate, TorqueChanged>
  execute(const state_type &state, const DeactivateLeftThruster &event) {
    if (state.left_thruster_activated) {
      const auto right = state.right_thruster_activated;
      return TorqueChanged{false, right,
                           state.torque - ActivateLeftThruster::torque};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateRightThruster -> (TorqueChanged)
  static constexpr std::variant<std::monostate, TorqueChanged>
  execute(const state_type &state, const ActivateRightThruster &event) {
    if (!state.right_thruster_activated) {
      const auto left = state.left_thruster_activated;
      return TorqueChanged{left, true,
                           state.torque + ActivateRightThruster::torque};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute DeactivateRightThruster -> (TorqueChanged)
  static constexpr std::variant<std::monostate, TorqueChanged>
  execute(const state_type &state, const DeactivateRightThruster &event) {
    if (state.right_thruster_activated) {
      const auto left = state.left_thruster_activated;
      return TorqueChanged{left, false,
                           state.torque - ActivateRightThruster::torque};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute Tick -> VelocityChanged & PositionChanged
  static constexpr std::tuple<VelocityChanged, PositionChanged>
  execute(const state_type &state, const Tick &command) {
    const auto velocity = update_velocity(state, command);
    const auto angular_velocity = update_angular_velocity(state, command);
    const auto position = update_position(state, command);
    const auto rotation = update_rotation(state, command);
    return {{velocity, angular_velocity}, {position, rotation}};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply ThrustChanged
  static constexpr state_type apply(const state_type &state,
                                    const ThrustChanged &event) {
    state_type next = state;
    next.main_thruster_activated = event.main_thruster_activated;
    next.thrust = event.thrust;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply TorqueChanged
  static constexpr state_type apply(const state_type &state,
                                    const TorqueChanged &event) {
    state_type next = state;
    next.left_thruster_activated = event.left_thruster_activated;
    next.right_thruster_activated = event.right_thruster_activated;
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
                                                 const Tick &command) {
    const mps_t velocity_change_abs = (state.thrust / state.mass) * command.dt;
    return {state.velocity.x + cos(state.rotation) * velocity_change_abs,
            state.velocity.y + sin(state.rotation) * velocity_change_abs};
  }

  //////////////////////////////////////////////////////////////////////////////
  // update_angular_velocity
  static constexpr radians_per_second_t
  update_angular_velocity(const state_type &state, const Tick &command) {
    // TODO: This should be type safe, but idk how to make units understand
    // how to convert from [N/kg/m] to [rad/s^2]
    const auto v = (state.torque / state.inertia).to<float>();
    const radians_per_second_squared_t angular_acc{v};
    return state.angular_velocity + angular_acc * command.dt;
  }

  //////////////////////////////////////////////////////////////////////////////
  // update_position
  static constexpr tensor<meter_t> update_position(const state_type &state,
                                                   const Tick &command) {
    return {state.position.x + state.velocity.x * command.dt,
            state.position.y + state.velocity.y * command.dt};
  }

  //////////////////////////////////////////////////////////////////////////////
  // update_rotation
  static constexpr radian_t update_rotation(const state_type &state,
                                            const Tick &command) {
    return state.rotation + state.angular_velocity * command.dt;
  }
};

#include <SFML/Graphics.hpp>
#include <memory>
#include <sstream>

/*******************************************************************************
 ** PlayerProjection
 *******************************************************************************/
template <typename Gui> struct PlayerProjection {
  constexpr static auto execute = event_sauce::disabled;
  constexpr static auto apply = event_sauce::disabled;
  struct state_type {};

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static void project(Gui &gui, const PositionChanged &event) {
    static constexpr auto scale_factor = 100.0f;
    auto ship = sf::CircleShape{80.f, 3};
    ship.setOrigin(80.f, 80.f);
    degree_t rotation = event.rotation;
    ship.setRotation(rotation.to<float>() + 90.0f);
    ship.move({event.position.x.to<float>() * scale_factor,
               event.position.y.to<float>() * scale_factor});
    gui.window.draw(ship);
  }
};

/*******************************************************************************
 ** main
 *******************************************************************************/
struct Gui {
  sf::RenderWindow window;
  sf::Font font;
  Gui() : window{sf::VideoMode(800, 600), "My window"} {
    if (!font.loadFromFile("/usr/share/doc/dbus-python/_static/fonts/"
                           "RobotoSlab/roboto-slab-v7-regular.ttf")) {
      throw std::runtime_error{"Failed to load fonts!"};
    }
  }
};

int main() {
  using namespace std::chrono_literals;
  Gui gui;
  auto ctx = event_sauce::make_context<Player, PlayerProjection<Gui>>(gui);

  auto t = std::chrono::high_resolution_clock::now();
  while (gui.window.isOpen()) {
    sf::Event event;
    while (gui.window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        gui.window.close();
      }
      if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Up) {
          ctx.dispatch(ActivateMainThruster{});
        }
        if (event.key.code == sf::Keyboard::Left) {
          ctx.dispatch(ActivateRightThruster{});
        }
        if (event.key.code == sf::Keyboard::Right) {
          ctx.dispatch(ActivateLeftThruster{});
        }
      }
      if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::Up) {
          ctx.dispatch(DeactivateMainThruster{});
        }
        if (event.key.code == sf::Keyboard::Left) {
          ctx.dispatch(DeactivateRightThruster{});
        }
        if (event.key.code == sf::Keyboard::Right) {
          ctx.dispatch(DeactivateLeftThruster{});
        }
      }
    }

    gui.window.clear(sf::Color::Black);
    const auto now = std::chrono::high_resolution_clock::now();
    ctx.dispatch(Tick{now - t});
    t = now;
    gui.window.display();
  }

  return 0;
}
