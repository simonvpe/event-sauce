#pragma once
#include "../commands.hpp"
#include "../common/units.hpp"
#include "entity.hpp"
#include "time.hpp"
#include <immer/map.hpp>

struct RigidBody {
  static constexpr auto project = [] {}; // Disabled

  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////

  struct Create {
    CorrelationId correlation_id;
    EntityId entity_id;
    kilogram_t mass;
  };

  struct ApplyForce {
    CorrelationId correlation_id;
    EntityId entity_id;
    tensor<newton_t> force;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////

  struct Created {
    CorrelationId correlation_id;
    EntityId entity_id;
    kilogram_t mass;
    tensor<mps_t> velocity;
  };

  struct ForceApplied {
    CorrelationId correlation_id;
    EntityId entity_id;
    tensor<newton_t> force;
  };

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct rigid_body_t {
    kilogram_t mass;
    tensor<mps_t> velocity;
    tensor<newton_t> force;
  };

  struct state_type {
    immer::map<EntityId, rigid_body_t> components;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Behaviour
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Execute Create -> Created
  static Created execute(const state_type &state, const Create &command) {
    return {command.correlation_id, command.entity_id, command.mass, 0_mps};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute ApplyForce -> ForceApplied
  static ForceApplied execute(const state_type &state,
                              const ApplyForce &command) {
    return {command.correlation_id, command.entity_id, command.force};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply ForceApplied
  static state_type apply(const state_type &state, const ForceApplied &event) {
    rigid_body_t rb = state.components[event.entity_id];
    rb.force += event.force;
    state_type next = state;
    next.components = next.components.set(event.entity_id, std::move(rb));
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Created
  static state_type apply(const state_type &state, const Created &event) {
    state_type next = state;
    auto rb = rigid_body_t{event.mass, event.velocity};
    next.components = next.components.set(event.entity_id, std::move(rb));
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply TimeAdvanced
  static state_type apply(const state_type &state, const TimeAdvanced &event) {
    state_type next = state;
    for (auto [entity_id, rb] : state.components) {
      rb.velocity += (rb.force / rb.mass) * event.dt;
      rb.force = {0_N, 0_N};
      next.components = next.components.set(entity_id, std::move(rb));
    }
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process TimeAdvanced -> [Entity::Move]
  static std::vector<Entity::Move> process(const state_type &state,
                                           const TimeAdvanced &event) {
    std::vector<Entity::Move> commands;
    for (auto [entity_id, rb] : state.components) {
      if (rb.velocity.x != 0_mps && rb.velocity.y != 0_mps) {
        auto position = rb.velocity * event.dt;
        commands.push_back({event.correlation_id, entity_id, position});
      }
    }
    return commands;
  }
};
