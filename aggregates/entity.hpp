#pragma once
#include "../commands.hpp"
#include <immer/map.hpp>

using Active = bool;

// Events

struct EntityCreated {
  CorrelationId correlation_id;
  EntityId entity_id;
};

struct EntityMoved {
  CorrelationId correlation_id;
  EntityId entity_id;
  tensor<meter_t> distance;
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
  static EntityMoved execute(const state_type &state,
                             const MoveEntity &command) {
    return {command.correlation_id, command.entity_id, command.distance};
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
  static state_type apply(const state_type &state, const EntityMoved &event) {
    auto entity = state.entities[event.entity_id];
    entity.position += event.distance;
    state_type next = state;
    next.entities = next.entities.set(event.entity_id, std::move(entity));
    return next;
  }
};
