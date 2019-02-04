#pragma once
#include "common/units.hpp"
#include <vector>

template <int Id> struct ActivateMainThruster {
  static constexpr newton_t thrust = 10.0_N;
};

template <int Id> struct DeactivateMainThruster {};

template <int Id> struct ActivateLeftThruster {
  static constexpr newton_meter_t torque = 10.0_Nm;
};

template <int Id> struct DeactivateLeftThruster {};

template <int Id> struct ActivateRightThruster {
  static constexpr newton_meter_t torque = -10.0_Nm;
};

template <int Id> struct DeactivateRightThruster {};

struct CreateMap {
  std::vector<std::tuple<tensor<meter_t>, tensor<meter_t>>> edges;
};

struct Tick {
  second_t dt;
};
