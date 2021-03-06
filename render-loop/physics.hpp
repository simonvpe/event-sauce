#pragma once
#include "../common/units.hpp"
#include "input.hpp"
#include <chrono>

namespace render_loop {

struct physics
{
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // COMMANDS
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  struct start
  {};

  struct stop
  {};

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // EVENTS
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  struct started
  {
    std::chrono::high_resolution_clock::time_point time;
  };

  struct stopped
  {};

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // STATE
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  struct state_type
  {
    std::chrono::high_resolution_clock::time_point time;
    second_t delta_time = 0_s;
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // EXECUTE
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  static started execute(const state_type&, const start&)
  {
    return started{ std::chrono::high_resolution_clock::now() };
  }

  static stopped execute(const state_type&, const stop&)
  {
    return stopped{};
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // APPLY
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  static state_type apply(const state_type& state, const started& evt)
  {
    return { evt.time, evt.time - state.time };
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // PROCESSES
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  static start process(const state_type&, const input::collected&)
  {
    return start{};
  }

  static stop process(const state_type&, const started&)
  {
    return stop{};
  }
};

}
