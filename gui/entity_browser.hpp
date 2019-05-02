#pragma once

#include "../mesh/cube.hpp"
#include "../physics/entity.hpp"
#include "../render-loop/rendering.hpp"
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>
#include <optional>
#include <tuple>

namespace gui {
struct entity_browser
{
  struct draw
  {};

  struct create_entity_requested
  {};

  struct create_cube_requested
  {
    physics::entity::id_type entity;
  };

  struct transform_entity_requested
  {
    physics::entity::id_type id;
    glm::vec3 position;
    glm::vec3 orientation;
  };

  struct entity_type
  {
    physics::entity::id_type id;
    glm::vec3 position;
    glm::vec3 orientation; // Pitch, yaw, roll
  };

  struct cube_type
  {
    mesh::cube::id_type id;
    physics::entity::id_type entity;
    glm::vec3 size;
  };

  struct state_type
  {
    immer::vector<entity_type> entities;
    immer::vector<cube_type> cubes;
  };

  static auto execute(const state_type& state, const draw& cmd) -> std::tuple<std::optional<create_entity_requested>,
                                                                              std::optional<create_cube_requested>,
                                                                              std::optional<transform_entity_requested>>
  {
    std::optional<create_entity_requested> create_entity_evt;
    std::optional<create_cube_requested> create_cube_evt;
    std::optional<transform_entity_requested> transform_entity_evt;
    ImGui::Begin("Entity Editor");

    if (ImGui::Button("Create")) {
      create_entity_evt = create_entity_requested{};
    }

    for (const auto& entity : state.entities) {
      ImGui::PushID(&entity);
      if (ImGui::TreeNode("Entity")) {
        auto changed = false;
        glm::vec3 position = entity.position;
        glm::vec3 orientation = entity.orientation;

        changed |= ImGui::InputFloat("pos x", &position.x, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat("pos y", &position.y, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat("pos z", &position.z, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);

        changed |= ImGui::InputFloat("pitch", &orientation.x, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat("yaw", &orientation.y, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat("roll", &orientation.z, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);

        const auto cube = std::find_if(
          state.cubes.begin(), state.cubes.end(), [&](const auto& cube) { return cube.entity == entity.id; });

        if (cube != state.cubes.end()) {
          ImGui::PushID("Cube");
          ImGui::Text("Cube");
          glm::vec3 size = cube->size;
          ImGui::InputFloat("size x", &size.x, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
          ImGui::InputFloat("size y", &size.y, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
          ImGui::InputFloat("size z", &size.z, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
          ImGui::PopID();
        } else {
          if (ImGui::Button("Create Cube")) {
            create_cube_evt = create_cube_requested{ entity.id };
          }
        }
        ImGui::Separator();
        ImGui::TreePop();

        if (changed) {
          transform_entity_evt = { entity.id, position, orientation };
        }
      }
      ImGui::PopID();
    }
    ImGui::End();
    return { create_entity_evt, create_cube_evt, transform_entity_evt };
  }

  static auto apply(const state_type& state, const physics::entity::created& evt) -> state_type
  {
    auto orientation = glm::eulerAngles(evt.orientation) * 180.0f / 2.0f / glm::pi<float>();
    auto entities = state.entities.push_back({ evt.id, evt.position, std::move(orientation) });
    return state_type{ entities, state.cubes };
  }

  static state_type apply(const state_type& state, const physics::entity::transform_changed& evt)
  {
    auto i = 0;
    for (const auto& entity : state.entities) {
      if (entity.id == evt.id) {
        auto entities = state.entities.update(i, [&](auto entity) {
          entity.position = evt.position;
          entity.orientation = glm::eulerAngles(evt.orientation) * 180.0f / 2.0f / glm::pi<float>();
          return entity;
        });
        return state_type{ entities, state.cubes };
      }
      ++i;
    }
    return state;
  }

  static state_type apply(const state_type& state, const mesh::cube::created& evt)
  {
    return state_type{ state.entities, state.cubes.push_back({ evt.id, evt.entity, evt.size }) };
  }

  static auto process(const state_type& state, const render_loop::rendering::started&) -> draw
  {
    return draw{};
  }

  static auto process(const state_type& state, const create_entity_requested&) -> physics::entity::create
  {
    static physics::entity::id_type next_id = 0;
    return { next_id++ };
  }

  static auto process(const state_type& state, const create_cube_requested& evt) -> mesh::cube::create
  {
    static mesh::cube::id_type next_id = 0;
    return { next_id++, evt.entity, { 1.0f, 1.0f, 1.0f } };
  }

  static auto process(const state_type& state, const transform_entity_requested& evt) -> physics::entity::transform
  {
    return { evt.id, evt.position, evt.orientation * 2.0f * glm::pi<float>() / 180.0f };
  }
};
}
