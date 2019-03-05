#pragma once
#include "common/units.hpp"
#include <vector>

using EntityId = int;
using CorrelationId = int;

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

struct CreateEntity {
  CorrelationId correlation_id;
};

struct ActivateThruster {
  PlayerId player;
  static constexpr newton_t thrust = 10.0_N;
};

struct DeactivateThruster {
  PlayerId player;
};

struct CreateMap {
  using Grid = std::array<std::array<bool, 100>, 100>;
  static constexpr meter_t cell_width = 1_m;
  Grid grid;
};

struct Tick {
  CorrelationId correlation_id;
  second_t dt;
};

struct SetShipRotation {
  PlayerId player;
  radian_t angle;
};

struct MoveEntity {
  CorrelationId correlation_id;
  EntityId entity_id;
  tensor<meter_t> distance;
};

struct RotateEntity {
  CorrelationId correlation_id;
  EntityId entity_id;
  radian_t angle;
};

struct SetEntityRotation {
  CorrelationId correlation_id;
  EntityId entity_id;
  radian_t rotation;
};
