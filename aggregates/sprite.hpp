#pragma once
#include "../commands.hpp"
#include "entity.hpp"
#include <memory>

template<typename Texture>
struct Sprite
{
  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////

  struct Create
  {
    CorrelationId correlation_id;
    EntityId entity_id;
    std::shared_ptr<Texture> texture;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////

  struct Created
  {
    CorrelationId correlation_id;
    EntityId entity_id;
    std::shared_ptr<Texture> texture;
  };

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct sprite_t
  {
    std::shared_ptr<Texture> texture;
    tensor<meter_t> position;
    tensor<meter_t> rotation;
  };

  using state_type = immer::map<EntityId, sprite_t>;

  //////////////////////////////////////////////////////////////////////////////
  // Behaviour
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Execute Create -> Created
  static Created execute(const state_type& state, const Create& command)
  {
    return { command.correlation_id, command.entity_id, command.texture };
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Created
  static state_type apply(const state_type& state, const Created& event)
  {
    auto sprite = sprite_t{ event.texture, { 0_m, 0_m }, 0_rad };
    return state.set(event.entity_id, std::move(sprite));
  }
};
