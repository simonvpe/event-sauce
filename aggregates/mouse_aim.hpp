#pragma once
#include "../common/units.hpp"
#include <tuple>
#include <variant>

// Events

struct ShipRotationChanged {
  PlayerId player;
  radian_t angle;
};

struct MouseAim {
public:
  static constexpr auto project = [] {}; // disabled

  struct state_type {
    radian_t angle;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Execute SetShipRotation -> ShipRotationChanged
  static constexpr ShipRotationChanged execute(const state_type &state,
                                               const SetShipRotation &command) {
    return {command.player, command.angle};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply ShipRotationChanged
  static constexpr state_type apply(const state_type &state,
                                    const ShipRotationChanged &event) {
    state_type next = state;
    next.angle = event.angle;
    return next;
  }
};
