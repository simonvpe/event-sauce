#pragma once
#include "../commands.hpp"
#include "../common/units.hpp"
#include "entity.hpp"
#include "time.hpp"
#include <immer/map.hpp>

struct Collider {
  static constexpr auto project = [] {};
  static constexpr auto process = [] {};

  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////

  struct Create {
    CorrelationId correlation_id;
    EntityId entity_id;
    rectangle<meter_t> bounding_box;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////

  struct Created {
    CorrelationId correlation_id;
    EntityId entity_id;
    rectangle<meter_t> bounding_box;
  };

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct collider_t {
    rectangle<meter_t> bounding_box;
    tensor<meter_t> position;
    radian_t rotation;
  };
  using state_type = immer::map<EntityId, collider_t>;

  //////////////////////////////////////////////////////////////////////////////
  // Behaviour
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Execute Create -> Created
  static Created execute(const state_type &state, const Create &command) {
    return {command.correlation_id, command.entity_id, command.bounding_box};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Created
  static state_type apply(const state_type &state, const Created &event) {
    auto collider = collider_t{event.bounding_box, {0_m, 0_m}, 0_rad};
    return state.set(event.entity_id, std::move(collider));
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Entity::PositionChanged
  static state_type apply(const state_type &state,
                          const Entity::PositionChanged &evt) {
    if (const auto *c = state.find(evt.entity_id)) {
      auto collider = *c;
      collider.position = evt.position;
      return state.set(evt.entity_id, collider);
    }
    return state;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Entity::RotationChanged
  static state_type apply(const state_type &state,
                          const Entity::RotationChanged &evt) {
    if (const auto *c = state.find(evt.entity_id)) {
      auto collider = *c;
      collider.rotation = evt.rotation;
      return state.set(evt.entity_id, collider);
    }
    return state;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply TimeAdvanced
  static state_type apply(const state_type &state, const TimeAdvanced &event) {
    return state;
  }
};
