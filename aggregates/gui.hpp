#pragma once
#include <functional>

namespace engine {
struct Gui {
  static constexpr auto project = [] {};

  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////

  struct RequestRendering {
    CorrelationId correlation_id;
    std::function<void()> on_begin;
    std::function<void()> on_end;
  };

  struct StartRendering {
    CorrelationId correlation_id;
  };

  struct StopRendering {
    CorrelationId correlation_id;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////

  struct RenderingRequested {
    CorrelationId correlation_id;
    std::function<void()> on_begin;
    std::function<void()> on_end;
  };

  struct RenderingStarted {
    CorrelationId correlation_id;
  };

  struct RenderingStopped {
    CorrelationId correlation_id;
  };

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct state_type {
    std::function<void()> on_begin;
    std::function<void()> on_end;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Behaviour
  //////////////////////////////////////////////////////////////////////////////

  static RenderingRequested execute(const state_type &,
                                    const RequestRendering &cmd) {
    return {cmd.correlation_id, cmd.on_begin, cmd.on_end};
  }

  static RenderingStarted execute(const state_type &,
                                  const StartRendering &cmd) {
    return {cmd.correlation_id};
  }

  static RenderingStopped execute(const state_type &,
                                  const StopRendering &cmd) {
    return {cmd.correlation_id};
  }

  static state_type apply(const state_type &state,
                          const RenderingRequested &evt) {
    evt.on_begin();
    return {evt.on_begin, state.on_end};
  }

  static state_type apply(const state_type &state, const RenderingStopped &) {
    state.on_end();
    return state;
  }

  static StartRendering processs(const state_type &,
                                 const RenderingRequested &cmd) {
    return {cmd.correlation_id};
  }

  static StopRendering process(const state_type &,
                               const RenderingStarted &cmd) {
    return {cmd.correlation_id};
  }
};
} // namespace engine
