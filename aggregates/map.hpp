#pragma once

#include "../commands.hpp"
#include "../common/units.hpp"

// Events

struct MapUpdated {
  std::vector<std::tuple<tensor<meter_t>, tensor<meter_t>>> edges;
};

struct Map {
  static constexpr auto project = [] {}; // disabled

  struct state_type {
    std::vector<std::tuple<tensor<meter_t>, tensor<meter_t>>> edges;
  };

  static MapUpdated execute(const state_type &state, const CreateMap &command) {
    return {command.edges};
  }

  static state_type apply(const state_type &state, const MapUpdated &event) {
    return {event.edges};
  }
};
