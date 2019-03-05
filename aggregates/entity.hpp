#pragma once
#include "../commands.hpp"
#include <immer/map.hpp>

using Active = bool;

// Events

struct EntityCreated {
  CorrelationId correlation_id;
  EntityId entity_id;
};

struct EntityPositionChanged {
  CorrelationId correlation_id;
  EntityId entity_id;
  tensor<meter_t> position;
};

struct EntityRotationChanged {
  CorrelationId correlation_id;
  EntityId entity_id;
  radian_t rotation;
};

struct Entity {
public:
  static constexpr auto project = [] {}; // Disabled
  static constexpr auto process = [] {}; // Disabled

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
  // Execute CreateEntity -> EntityCreated
  static EntityCreated execute(const state_type &state,
                               const CreateEntity &command) {
    return {command.correlation_id, state.next_unused_entity_id};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute MoveEntity -> EntityMoved
  static EntityPositionChanged execute(const state_type &state,
                                       const MoveEntity &command) {
    auto position = state.entities[command.entity_id].position;
    position += command.distance;
    return {command.correlation_id, command.entity_id, position};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute RotateEntity -> EntityRotationChanged
  static EntityRotationChanged execute(const state_type &state,
                                       const RotateEntity &command) {
    auto rotation = state.entities[command.entity_id].rotation;
    rotation += command.angle;
    return {command.correlation_id, command.entity_id, rotation};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Execute SetEntityRotation -> EntityRotationChanged
  static EntityRotationChanged execute(const state_type &state,
                                       const SetEntityRotation &command) {
    return {command.correlation_id, command.entity_id, command.rotation};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply EntityCreated
  static state_type apply(const state_type &state, const EntityCreated &event) {
    auto entity = entity_t{true, {0_m, 0_m}, 0_rad};
    state_type next = state;
    next.entities = next.entities.set(event.entity_id, std::move(entity));
    next.next_unused_entity_id = state.next_unused_entity_id + 1;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply EntityMoved
  static state_type apply(const state_type &state,
                          const EntityPositionChanged &event) {
    auto entity = state.entities[event.entity_id];
    entity.position = event.position;
    state_type next = state;
    next.entities = next.entities.set(event.entity_id, std::move(entity));
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply EntityRotated
  static state_type apply(const state_type &state,
                          const EntityRotationChanged &event) {
    auto entity = state.entities[event.entity_id];
    entity.rotation = event.rotation;
    state_type next = state;
    next.entities = next.entities.set(event.entity_id, std::move(entity));
    return next;
  }
};
