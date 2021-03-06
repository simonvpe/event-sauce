#pragma once
#include "../commands.hpp"
#include "../common/units.hpp"
#include "collider.hpp"
#include "entity.hpp"
#include "rigid_body.hpp"
#include <immer/map.hpp>
#include <tuple>
#include <variant>

/*******************************************************************************
 ** Player
 *******************************************************************************/
struct Player
{
public:
  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////

  struct Create
  {
    CorrelationId player_id;
  };

  struct ActivateThruster
  {
    CorrelationId player_id;
  };

  struct SetRotation
  {
    CorrelationId player_id;
    radian_t rotation;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////

  struct Created
  {
    CorrelationId player_id;
  };

  struct ThrusterActivated
  {
    CorrelationId player_id;
  };

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct player_t
  {
    newton_t thrust;
    EntityId root_entity_id;
    radian_t rotation;
  };

  struct state_type
  {
    immer::map<CorrelationId, player_t> players;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Creation Behaviour
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Execute Create -> Created
  static Created execute(const state_type& state, const Create& cmd) { return Created{ cmd.player_id }; }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Created
  static state_type apply(const state_type& state, const Created& evt)
  {
    state_type next = state;
    next.players = next.players.set(evt.player_id, {});
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process Created -> Entity::Create
  static Entity::Create process(const state_type& state, const Created& evt) { return { evt.player_id }; }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Entity::Created
  static state_type apply(const state_type& state, const Entity::Created& evt)
  {
    if (state.players.find(evt.correlation_id)) {
      auto player = player_t{ 10_N, evt.entity_id, 0_rad };
      state_type next = state;
      next.players = next.players.set(evt.correlation_id, player);
      return next;
    }
    return state;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process Entity::Created -> [RigidBody::Create | Collider::Create]
  static std::vector<std::variant<RigidBody::Create, Collider::Create>> process(const state_type& state,
                                                                                const Entity::Created& evt)
  {
    const auto player_id = evt.correlation_id;
    if (state.players.find(player_id)) {
      const auto entity_id = state.players[player_id].root_entity_id;
      auto mass = 1_kg;
      auto box = rectangle<meter_t>{ { 0_m, 0_m }, { 1_m, 1_m } };
      return { RigidBody::Create{ evt.correlation_id, entity_id, std::move(mass) },
               Collider::Create{ evt.correlation_id, entity_id, std::move(box) } };
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply RigidBodyCreated
  static state_type apply(const state_type& state, const RigidBody::Created& evt)
  {
    std::cout << "Player " << evt.correlation_id << " created!" << std::endl;
    return state;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Control Behaviour
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateThruster -> ThrusterActivated
  static ThrusterActivated execute(const state_type& state, const ActivateThruster& evt) { return { evt.player_id }; }

  //////////////////////////////////////////////////////////////////////////////
  // Execute ChangeRotation -> Entity::RotationChanged
  static std::variant<std::monostate, Entity::RotationChanged> execute(const state_type& state, const SetRotation& evt)
  {
    const auto player_id = evt.player_id;
    if (state.players.find(player_id)) {
      const auto entity_id = state.players[player_id].root_entity_id;
      return Entity::RotationChanged{ player_id, entity_id, evt.rotation };
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process ThrusterActivated -> RigidBody::ApplyForce
  static RigidBody::ApplyForce process(const state_type& state, const ThrusterActivated& evt)
  {
    using namespace units::math;
    const auto& player = state.players[evt.player_id];
    const auto rotation = player.rotation;
    const auto direction = tensor<float>{ cos(rotation), sin(rotation) };
    const auto thrust = direction * player.thrust;
    return { evt.player_id, player.root_entity_id, thrust };
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Entity::RotationChanged
  static state_type apply(const state_type& state, const Entity::RotationChanged& evt)
  {

    if (const auto* p = state.players.find(evt.correlation_id)) {
      player_t player = *p;
      player.rotation = evt.rotation;
      state_type next = state;
      next.players = next.players.set(evt.correlation_id, std::move(player));
      return next;
    }
    return state;
  }
};
