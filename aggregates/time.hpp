#pragma once
#include "../commands.hpp"

struct TimeAdvanced
{
  CorrelationId correlation_id;
  second_t dt;
};

struct Time
{
  struct state_type
  {
    second_t time;
  };

  static TimeAdvanced execute(const state_type& state, const Tick& command)
  {
    return { command.correlation_id, command.dt };
  }

  static state_type apply(const state_type& state, const TimeAdvanced& event)
  {
    state_type next = state;
    next.time += event.dt;
    return next;
  }
};
