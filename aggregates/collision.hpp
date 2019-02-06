#pragma once
#include "map.hpp"
#include "player.hpp"

// Events

struct PlayerCollided {
  tensor<meter_t> position;
};

struct Collision {

  static constexpr auto project = [] {};

  struct state_type {
    MapUpdated::Grid grid;
    tensor<meter_t> player_position;
  };

  static std::variant<std::monostate, PlayerCollided>
  execute(const state_type &state, const Tick &event) {
    const auto _px = state.player_position.x;
    const auto _py = state.player_position.y;
    for (auto y = 0; y < state.grid.size(); ++y) {
      const auto &row = state.grid[y];
      for (auto x = 0; x < row.size(); ++x) {
        const auto &cell = row[x];
        const auto x0 = CreateMap::cell_width * x;
        const auto x1 = x0 + CreateMap::cell_width;
        const auto y0 = CreateMap::cell_width * y;
        const auto y1 = y0 + CreateMap::cell_width;
        if (cell && _px < x1 && _px > x0 && _py < y1 && _py > y0) {
          std::cout << "COLLISION" << std::endl;
          return {PlayerCollided{state.player_position}};
        }
      }
    }
    return {};
  }

  template <int Id>
  static constexpr state_type apply(const state_type &state,
                                    const PositionChanged<Id> &event) {
    return {state.grid, event.position};
  }

  static constexpr state_type apply(const state_type &state,
                                    const MapUpdated &event) {
    return {event.grid};
  }
};
