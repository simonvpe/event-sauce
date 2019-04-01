#pragma once
#include "../commands.hpp"
#include "../common/qtree.hpp"
#include "../common/units.hpp"
#include "entity.hpp"
#include "time.hpp"
#include <immer/box.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>

struct Collider
{
  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////

  struct Create
  {
    CorrelationId correlation_id;
    EntityId entity_id;
    rectangle<meter_t> bounding_box;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////

  struct Created
  {
    CorrelationId correlation_id;
    EntityId entity_id;
    rectangle<meter_t> bounding_box;
  };

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct collider_t
  {
    EntityId entity_id;
    rectangle<meter_t> bounding_box;
    tensor<meter_t> position;
    radian_t rotation;
  };
  using state_type = quad_tree<collider_t>;

  //////////////////////////////////////////////////////////////////////////////
  // Behaviour
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Execute Create -> Created
  static Created execute(const state_type& state, const Create& command)
  {
    return { command.correlation_id, command.entity_id, command.bounding_box };
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Created
  static state_type apply(const state_type& state, const Created& event)
  {
    auto collider = collider_t{ event.entity_id, event.bounding_box, { 0_m, 0_m }, 0_rad };
    return insert(state, std::move(collider), { 0_m, 0_m });
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Entity::PositionChanged
  static state_type apply(const state_type& state, const Entity::PositionChanged& evt)
  {
    return move(
      state, evt.position, 10_m, [&evt](const auto& collider) { return evt.entity_id == collider.entity_id; });
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Entity::RotationChanged
  static state_type apply(const state_type& state, const Entity::RotationChanged& evt)
  {
    /*
    if (const auto *c = state.find(evt.entity_id)) {
      auto collider = *c;
      collider.rotation = evt.rotation;
      return state.set(evt.entity_id, collider);
    }
    */
    return state;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply TimeAdvanced
  static state_type apply(const state_type& state, const TimeAdvanced& event) { return state; }
};
