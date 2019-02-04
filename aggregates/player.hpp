#pragma once
#include "../commands.hpp"
#include "../common/units.hpp"
#include <tuple>
#include <variant>

// Events

template <int Id> struct ThrustChanged {
  bool main_thruster_activated;
  newton_t thrust;
};

template <int Id> struct TorqueChanged {
  bool left_thruster_activated;
  bool right_thruster_activated;
  newton_meter_t torque;
};

template <int Id> struct VelocityChanged {
  tensor<mps_t> velocity;
  radians_per_second_t angular_velocity;
};

template <int Id> struct PositionChanged {
  tensor<meter_t> position;
  radian_t rotation;
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
    kilogram_meters_squared_t inertia = initial_inertia;
    radian_t rotation = 0_rad;
    tensor<mps_t> velocity = {0_mps, 0_mps};
    radians_per_second_t angular_velocity = 0_rad_per_s;
    tensor<meter_t> position = {1.0_m, 1.0_m};
    tensor<meter_t> size = {1.0_m, 1.0_m};
    newton_t thrust = 0_N;
    newton_meter_t torque = 0_Nm;
    bool left_thruster_activated = false;
    bool right_thruster_activated = false;
    bool main_thruster_activated = false;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateMainThruster -> (ThrustChanged)
  static constexpr std::variant<std::monostate, ThrustChanged<Id>>
  execute(const state_type &state, const ActivateMainThruster<Id> &event) {
    if (!state.main_thruster_activated) {
      const auto thrust = state.thrust + ActivateMainThruster<Id>::thrust;
      return ThrustChanged<Id>{true, thrust};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute DeactivateMainThruster -> (ThrustChanged)
  static constexpr std::variant<std::monostate, ThrustChanged<Id>>
  execute(const state_type &state, const DeactivateMainThruster<Id> &event) {
    if (state.main_thruster_activated) {
      const auto thrust = state.thrust - ActivateMainThruster<Id>::thrust;
      return ThrustChanged<Id>{false, thrust};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateLeftThruster -> (TorqueChanged)
  static constexpr std::variant<std::monostate, TorqueChanged<Id>>
  execute(const state_type &state, const ActivateLeftThruster<Id> &event) {
    if (!state.left_thruster_activated) {
      const auto right = state.right_thruster_activated;
      const auto torque = state.torque + ActivateLeftThruster<Id>::torque;
      return TorqueChanged<Id>{true, right, torque};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute DeactivateLeftThruster -> (TorqueChanged)
  static constexpr std::variant<std::monostate, TorqueChanged<Id>>
  execute(const state_type &state, const DeactivateLeftThruster<Id> &event) {
    if (state.left_thruster_activated) {
      const auto right = state.right_thruster_activated;
      const auto torque = state.torque - ActivateLeftThruster<Id>::torque;
      return TorqueChanged<Id>{false, right, torque};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateRightThruster -> (TorqueChanged)
  static constexpr std::variant<std::monostate, TorqueChanged<Id>>
  execute(const state_type &state, const ActivateRightThruster<Id> &event) {
    if (!state.right_thruster_activated) {
      const auto left = state.left_thruster_activated;
      const auto torque = state.torque + ActivateRightThruster<Id>::torque;
      return TorqueChanged<Id>{left, true, torque};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute DeactivateRightThruster -> (TorqueChanged)
  static constexpr std::variant<std::monostate, TorqueChanged<Id>>
  execute(const state_type &state, const DeactivateRightThruster<Id> &event) {
    if (state.right_thruster_activated) {
      const auto left = state.left_thruster_activated;
      const auto torque = state.torque - ActivateRightThruster<Id>::torque;
      return TorqueChanged<Id>{left, false, torque};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute Tick -> VelocityChanged & PositionChanged
  static constexpr std::tuple<VelocityChanged<Id>, PositionChanged<Id>>
  execute(const state_type &state, const Tick &command) {
    const auto velocity = update_velocity(state, command);
    const auto angular_velocity = update_angular_velocity(state, command);
    const auto position = update_position(state, command);
    const auto rotation = update_rotation(state, command);
    return {{velocity, angular_velocity}, {position, rotation, state.size}};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply ThrustChanged
  static constexpr state_type apply(const state_type &state,
                                    const ThrustChanged<Id> &event) {
    state_type next = state;
    next.main_thruster_activated = event.main_thruster_activated;
    next.thrust = event.thrust;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply TorqueChanged
  static constexpr state_type apply(const state_type &state,
                                    const TorqueChanged<Id> &event) {
    state_type next = state;
    next.left_thruster_activated = event.left_thruster_activated;
    next.right_thruster_activated = event.right_thruster_activated;
    next.torque = event.torque;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply VelocityChanged
  static constexpr state_type apply(const state_type &state,
                                    const VelocityChanged<Id> &event) {
    state_type next = state;
    next.velocity = event.velocity;
    next.angular_velocity = event.angular_velocity;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply PositionChanged
  static constexpr state_type apply(const state_type &state,
                                    const PositionChanged<Id> &event) {
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
    const float v = state.torque / state.inertia;
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
