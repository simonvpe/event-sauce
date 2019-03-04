#pragma once

#include "../commands.hpp"
#include <tuple>
#include <variant>

// Events
using PlayerId = int;

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

  static constexpr bool validate_player_id(const state_type &state,
                                           PlayerId player) {
    if (player != 0) {
      std::cerr << "Invalid player id " << player << std::endl;
      return false;
    }
    return true;
  }

  //////////////////////////////////////////////////////////////////////////////
  // ActivateMainThruster -> ThrustChanged
  static std::variant<std::monostate, ThrustChanged>
  execute(const state_type &state, const ActivateMainThruster &command) {
    if (!validate_player_id(state, command.player)) {
      return {};
    }
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
    if (!validate_player_id(state, command.player)) {
      return {};
    }
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
    if (!validate_player_id(state, event.player)) {
      return state;
    }
    state_type next = state;
    next.activated = event.activated;
    next.thrust = event.thrust;
    return next;
  }
};
