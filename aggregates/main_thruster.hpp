#pragma once

#include "../commands.hpp"
#include <tuple>
#include <variant>

// Events

struct ThrustChanged {
  PlayerId player;
  newton_t thrust;
  bool activated;
};

struct MainThruster {
  static constexpr auto project = [] {}; // disabled

  struct state_type {
    newton_t thrust;
    bool activated = false;
  };

  //////////////////////////////////////////////////////////////////////////////
  // ActivateMainThruster -> ThrustChanged
  static std::variant<std::monostate, ThrustChanged>
  execute(const state_type &state, const ActivateMainThruster &command) {
    if (!state.activated) {
      const auto thrust = state.thrust + ActivateMainThruster::thrust;
      return {ThrustChanged{command.player, thrust, true}};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // DeactivateMainThruster -> ThrustChanged
  static std::variant<std::monostate, ThrustChanged>
  execute(const state_type &state, const DeactivateMainThruster &command) {
    if (state.activated) {
      const auto thrust = state.thrust - ActivateMainThruster::thrust;
      return {ThrustChanged{command.player, thrust, false}};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply ThrustChanged
  static constexpr state_type apply(const state_type &state,
                                    const ThrustChanged &event) {
    state_type next = state;
    next.activated = event.activated;
    next.thrust = event.thrust;
    return next;
  }
};
