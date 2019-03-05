#pragma once
#include "../commands.hpp"
#include "../common/units.hpp"
#include "entity.hpp"
#include "main_thruster.hpp"
#include "mouse_aim.hpp"
#include "rigid_body.hpp"
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

  struct state_type {
    newton_t thrust = 10_N;
    EntityId root_entity_id;
    CorrelationId player_id;
    radian_t rotation;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Creation Behaviour
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Execute Created -> Created
  static constexpr Created execute(const state_type &state, const Create &cmd) {
    return {cmd.player_id};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply Created
  static constexpr state_type apply(const state_type &state,
                                    const Created &evt) {
    state_type next = state;
    next.player_id = evt.player_id;
    return next;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process Created -> CreateEntity
  static constexpr CreateEntity process(const state_type &state,
                                        const Created &evt) {
    return {state.player_id};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply EntityCreated
  static constexpr state_type apply(const state_type &state,
                                    const EntityCreated &evt) {
    if (evt.correlation_id == state.player_id) {
      state_type next = state;
      next.root_entity_id = evt.entity_id;
      return next;
    }
    return state;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process EntityCreated -> [CreateRigidBody]
  static std::vector<CreateRigidBody> process(const state_type &state,
                                              const EntityCreated &evt) {
    if (evt.correlation_id == state.player_id) {
      return {{evt.correlation_id, state.root_entity_id, 1_kg}};
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
    const auto rotation = state.rotation;
    const auto direction = tensor<float>{cos(rotation), sin(rotation)};
    const auto thrust = direction * state.thrust;
    return {state.player_id, state.root_entity_id, thrust};
  }

  static state_type apply(const state_type &state,
                          const EntityRotationChanged &evt) {
    state_type next = state;
    next.rotation = evt.rotation;
    return next;
  }
};
