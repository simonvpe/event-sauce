#pragma once
#include "../commands.hpp"
#include "../common/qtree.hpp"
#include "../common/units.hpp"
#include "entity.hpp"
#include "time.hpp"
#include <immer/box.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>

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
    auto qtree = quad_tree<EntityId>(0_m, 0_m, 100_m, 4);
    auto next = insert(qtree, 0, {1_m, 1_m});
    next = insert(next, 1, {2_m, 2_m});
    next = insert(next, 2, {3_m, 3_m});
    next = insert(next, 3, {4_m, 4_m});
    next = insert(next, 4, {5_m, 5_m});
    next = insert(next, 5, {5_m, 5_m});
    next = insert(next, 6, {5_m, 5_m});
    next = insert(next, 7, {5_m, 5_m});
    next = insert(next, 8, {5_m, 5_m});
    next = insert(next, 9, {5_m, 5_m});
    next = insert(next, 10, {5.5_m, 5_m});
    {
      auto points = query(next, {3_m, 3_m, 3_m});
      std::cout << points.size() << std::endl;
      for (const auto &point : points) {
        std::cout << "Found " << point.payload << " @ " << point.position
                  << std::endl;
      }
    }
    for (auto i = 0; i < 11; ++i) {
      next = move(next, {0_m, 0_m}, 10_m, [](int i) { return i == 5; });
    }
    // next = move(next, 4, {0_m, 0_m}, 10_m);
    {
      auto points = query(next, {3_m, 3_m, 3_m});
      std::cout << points.size() << std::endl;
      for (const auto &point : points) {
        std::cout << "Found " << point.payload << " @ " << point.position
                  << std::endl;
      }
    }
    std::cout << "southwest=" << (bool)next->southwest << std::endl;

    /*
    auto q = query(next, {3_m, 3_m, 1_m},
                   [](const auto &entity_id) { return entity_id == 2; });
    if (q) {
      std::cout << "Queried " << std::get<EntityId>(*q) << std::endl;
    }
    */
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
