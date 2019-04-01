#pragma once

#include "../commands.hpp"
#include "../common/units.hpp"

// Events

struct MapUpdated
{
  using Grid = std::array<std::array<bool, 100>, 100>;
  Grid grid;
};

struct Map
{
  struct state_type
  {
    CreateMap::Grid grid;
  };

  static MapUpdated execute(const state_type& state, const CreateMap& command) { return { command.grid }; }

  static state_type apply(const state_type& state, const MapUpdated& event) { return { event.grid }; }
};
