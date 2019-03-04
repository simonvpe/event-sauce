#pragma once
#include "common/units.hpp"
#include <vector>

using PlayerId = int;

struct ActivateMainThruster {
  PlayerId player;
  static constexpr newton_t thrust = 10.0_N;
};

struct DeactivateMainThruster {
  PlayerId player;
};

template <int Id> struct ActivateLeftThruster {
  static constexpr newton_meter_t torque = 10.0_Nm;
};

template <int Id> struct DeactivateLeftThruster {};

template <int Id> struct ActivateRightThruster {
  static constexpr newton_meter_t torque = -10.0_Nm;
};

template <int Id> struct DeactivateRightThruster {};

struct CreateMap {
  using Grid = std::array<std::array<bool, 100>, 100>;
  static constexpr meter_t cell_width = 1_m;
  Grid grid;
};

struct Tick {
  second_t dt;
};
