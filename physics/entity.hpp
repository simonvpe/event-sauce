#pragma once
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <immer/vector.hpp>
#include <optional>

namespace physics {
struct entity
{
  using id_type = int;

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // COMMANDS
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  struct create
  {
    id_type id;
  };

  struct transform
  {
    id_type id;
    glm::vec3 position;
    glm::quat orientation;
    ;
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // EVENTS
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  struct created
  {
    id_type id;
    glm::vec3 position;
    glm::quat orientation;
  };

  struct transform_changed
  {
    id_type id;
    glm::vec3 position;
    glm::quat orientation;
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // STATE
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  struct entity_type
  {
    id_type id;
    glm::vec3 position;
    glm::quat orientation;
  };

  using state_type = immer::vector<entity_type>;

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // EXECUTE
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  static std::optional<created> execute(const state_type& state, const create& cmd)
  {
    for (const auto& entity : state) {
      if (entity.id == cmd.id) {
        return std::nullopt;
      }
    }
    auto origo = glm::vec3{};
    auto unit_quaternion = glm::quat{};
    return { created{ cmd.id, origo, unit_quaternion } };
  }

  static auto execute(const state_type& state, const transform& cmd) -> std::optional<transform_changed>
  {
    for (const auto& entity : state) {
      if (entity.id == cmd.id) {
        return transform_changed{ cmd.id, cmd.position, cmd.orientation };
      }
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // APPLY
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  static state_type apply(const state_type& state, const created& evt)
  {
    return state.push_back({ evt.id, evt.position, evt.orientation });
  }

  static state_type apply(const state_type& state, const transform_changed& evt)
  {
    auto i = 0;
    for (const auto& entity : state) {
      if (entity.id == evt.id) {
        return state.update(i, [&](auto entity) {
          entity.position = evt.position;
          entity.orientation = evt.orientation;
          return entity;
        });
      }
      ++i;
    }
    return state;
  }
};
}
