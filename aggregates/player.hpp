#pragma once
#include "../commands.hpp"
#include "../common/units.hpp"
#include "main_thruster.hpp"
#include "mouse_aim.hpp"
#include <tuple>
#include <variant>

// Events

template <int Id> struct TorqueChanged {
  bool left_thruster_activated;
  bool right_thruster_activated;
  newton_meter_t torque;
};

template <int Id> struct VelocityChanged { tensor<mps_t> velocity; };

template <int Id> struct PositionChanged {
  tensor<meter_t> position;
  tensor<meter_t> size;
};

/*******************************************************************************
 ** Player
 *******************************************************************************/
template <int Id = 0> struct Player {
public:
  static constexpr auto project = [] {}; // disabled

  // State
  struct state_type {
    static constexpr auto initial_inertia = kilogram_meters_squared_t{1.0};
    kilogram_t mass = 1.0_kg;
    radian_t rotation = 0_rad;
    tensor<mps_t> velocity = {0_mps, 0_mps};
    tensor<meter_t> position = {1.0_m, 1.0_m};
    tensor<meter_t> size = {1.0_m, 1.0_m};
    newton_t thrust = 0_N;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Execute Tick -> VelocityChanged & PositionChanged
  static constexpr std::tuple<VelocityChanged<Id>, PositionChanged<Id>>
  execute(const state_type &state, const Tick &command) {
    const auto velocity = update_velocity(state, command.dt);
    const auto position = update_position(state, command.dt);
    return {{velocity}, {position, state.size}};
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
  // Apply VelocityChanged
  static constexpr state_type apply(const state_type &state,
                                    const VelocityChanged<Id> &event) {
    state_type next = state;
    next.velocity = event.velocity;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static constexpr state_type apply(const state_type &state,
                                    const PositionChanged<Id> &event) {
    state_type next = state;
    next.position = event.position;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply ShipRotationChanged
  static constexpr state_type apply(const state_type &state,
                                    const ShipRotationChanged &event) {
    state_type next = state;
    next.rotation = event.angle;
    return next;
  }

private:
  //////////////////////////////////////////////////////////////////////////////
  // update_velocity
  static constexpr tensor<mps_t> update_velocity(const state_type &state,
                                                 const second_t &dt) {
    const mps_t velocity_change_abs = (state.thrust / state.mass) * dt;
    return {state.velocity.x + cos(state.rotation) * velocity_change_abs,
            state.velocity.y + sin(state.rotation) * velocity_change_abs};
  }

  //////////////////////////////////////////////////////////////////////////////
  // update_position
  static constexpr tensor<meter_t> update_position(const state_type &state,
                                                   const second_t &dt) {
    return {state.position.x + state.velocity.x * dt,
            state.position.y + state.velocity.y * dt};
  }
};
