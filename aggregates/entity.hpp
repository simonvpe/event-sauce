#pragma once
#include "../commands.hpp"
#include <immer/map.hpp>

using Active = bool;

struct Entity {
public:
  static constexpr auto project = [] {}; // Disabled
  static constexpr auto process = [] {}; // Disabled

  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////

  struct Created {
    CorrelationId correlation_id;
    EntityId entity_id;
  };

  struct PositionChanged {
    CorrelationId correlation_id;
    EntityId entity_id;
    tensor<meter_t> position;
  };

  struct RotationChanged {
    CorrelationId correlation_id;
    EntityId entity_id;
    radian_t rotation;
  };

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct entity_t {
    bool active;
    tensor<meter_t> position;
    radian_t rotation;
  };

  struct state_type {
    immer::map<EntityId, entity_t> entities;
    EntityId next_unused_entity_id = 0;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Execute CreateEntity -> Created
  static Created execute(const state_type &state, const CreateEntity &command) {
    return {command.correlation_id, state.next_unused_entity_id};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute MoveEntity -> EntityMoved
  static PositionChanged execute(const state_type &state,
                                 const MoveEntity &command) {
    auto position = state.entities[command.entity_id].position;
    position += command.distance;
    return {command.correlation_id, command.entity_id, position};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute RotateEntity -> RotationChanged
  static RotationChanged execute(const state_type &state,
                                 const RotateEntity &command) {
    auto rotation = state.entities[command.entity_id].rotation;
    rotation += command.angle;
    return {command.correlation_id, command.entity_id, rotation};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute SetEntityRotation -> RotationChanged
  static RotationChanged execute(const state_type &state,
                                 const SetEntityRotation &command) {
    return {command.correlation_id, command.entity_id, command.rotation};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Created
  static state_type apply(const state_type &state, const Created &event) {
    auto entity = entity_t{true, {0_m, 0_m}, 0_rad};
    state_type next = state;
    next.entities = next.entities.set(event.entity_id, std::move(entity));
    next.next_unused_entity_id = state.next_unused_entity_id + 1;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply EntityMoved
  static state_type apply(const state_type &state,
                          const PositionChanged &event) {
    auto entity = state.entities[event.entity_id];
    entity.position = event.position;
    state_type next = state;
    next.entities = next.entities.set(event.entity_id, std::move(entity));
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply EntityRotated
  static state_type apply(const state_type &state,
                          const RotationChanged &event) {
    auto entity = state.entities[event.entity_id];
    entity.rotation = event.rotation;
    state_type next = state;
    next.entities = next.entities.set(event.entity_id, std::move(entity));
    return next;
  }
};
