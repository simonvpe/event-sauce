#pragma once
#include "common/units.hpp"
#include <vector>

class PlayerId {
public:
  constexpr PlayerId(int value) : value{value} {
    if (value != 0) {
      throw std::runtime_error{"Invalid player id" + std::to_string(value)};
    }
  }

  operator int() { return value; }

private:
  int value;
};

struct ActivateMainThruster {
  PlayerId player;
  static constexpr newton_t thrust = 10.0_N;
};

struct DeactivateMainThruster {
  PlayerId player;
};

struct CreateMap {
  using Grid = std::array<std::array<bool, 100>, 100>;
  static constexpr meter_t cell_width = 1_m;
  Grid grid;
};

struct Tick {
  second_t dt;
};

struct SetShipRotation {
  PlayerId player;
  radian_t angle;
};
