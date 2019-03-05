#pragma once
#include "../commands.hpp"
#include "../common/units.hpp"
#include "entity.hpp"
#include "main_thruster.hpp"
#include "mouse_aim.hpp"
#include "rigid_body.hpp"
#include <immer/map.hpp>
#include <tuple>
#include <variant>

/*******************************************************************************
 ** Player
 *******************************************************************************/
struct Player {
public:
  static constexpr auto project = [] {}; // Disabled

  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////

  struct Create {
    CorrelationId player_id;
  };

  struct ActivateThruster {
    CorrelationId player_id;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////

  struct Created {
    CorrelationId player_id;
  };

  struct ThrusterActivated {
    CorrelationId player_id;
  };

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct player_t {
    newton_t thrust;
    EntityId root_entity_id;
    radian_t rotation;
  };

  struct state_type {
    immer::map<CorrelationId, player_t> players;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Creation Behaviour
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Execute Create -> Created
  static Created execute(const state_type &state, const Create &cmd) {
    return Created{cmd.player_id};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Created
  static state_type apply(const state_type &state, const Created &evt) {
    state_type next = state;
    next.players = next.players.set(evt.player_id, {});
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process Created -> CreateEntity
  static CreateEntity process(const state_type &state, const Created &evt) {
    return {evt.player_id};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply EntityCreated
  static state_type apply(const state_type &state, const EntityCreated &evt) {
    if (state.players.find(evt.correlation_id)) {
      auto player = player_t{10_N, evt.entity_id, 0_rad};
      state_type next = state;
      next.players = next.players.set(evt.correlation_id, player);
      return next;
    }
    return state;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process EntityCreated -> [CreateRigidBody]
  static std::vector<CreateRigidBody> process(const state_type &state,
                                              const EntityCreated &evt) {
    const auto player_id = evt.correlation_id;
    if (state.players.find(player_id)) {
      const auto entity_id = state.players[player_id].root_entity_id;
      return {{evt.correlation_id, entity_id, 1_kg}};
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply RigidBodyCreated
  static state_type apply(const state_type &state,
                          const RigidBodyCreated &evt) {
    std::cout << "Player " << evt.correlation_id << " created!" << std::endl;
    return state;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Control Behaviour
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Execute ActivateThruster -> ThrusterActivated
  static ThrusterActivated execute(const state_type &state,
                                   const ActivateThruster &evt) {
    return {evt.player_id};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process ThrusterActivated -> RigidBody::ApplyForce
  static ApplyForce process(const state_type &state,
                            const ThrusterActivated &evt) {
    using namespace units::math;
    const auto &player = state.players[evt.player_id];
    const auto rotation = player.rotation;
    const auto direction = tensor<float>{cos(rotation), sin(rotation)};
    const auto thrust = direction * player.thrust;
    return {evt.player_id, player.root_entity_id, thrust};
  }

  static state_type apply(const state_type &state,
                          const EntityRotationChanged &evt) {
    if (const auto *p = state.players.find(evt.correlation_id)) {
      player_t player = *p;
      player.rotation = evt.rotation;
      state_type next = state;
      next.players = next.players.set(evt.correlation_id, std::move(player));
      return next;
    }
    return state;
  }
};
