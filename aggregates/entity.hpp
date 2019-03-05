#pragma once
#include "../commands.hpp"
#include <memory>
#include <unordered_map>

using EntityId = int;
using Active = bool;

// Events

struct EntityCreated {
  CorrelationId correlationId;
  EntityId entityId;
};

struct Entity {
public:
  static constexpr auto project = [] {}; // Disabled

  struct state_type {
    std::unordered_map<EntityId, Active> active;
    EntityId next_unused_entity_id = 0;
  };

  static EntityCreated(const state_type &state, const CreateEntity &command) {
    return {command.correlationId, state.next_unused_entity_id};
  }

  static state_type apply(const state_type &state, const EntityCreated &event) {
    state_type next = state;
    next.active[state.next_unused_entity_id] = true;
    next.next_unused_entity_id = state.next_unused_entity_id + 1;
    return next;
  }
};
