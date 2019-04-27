#include "opengl/opengl.hpp"
#include "physics/entity.hpp"
#include "pool_dispatcher.hpp"
#include "render-loop/startup.hpp"
#include "scheduler/fiber.hpp"
#include <event-sauce/event-sauce.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

struct test_gui
{

  struct draw
  {};

  struct checkbox_toggled
  {
    bool value;
  };

  struct button_clicked
  {};

  struct state_type
  {
    bool checkbox = false;
  };

  static std::tuple<std::optional<checkbox_toggled>, std::optional<button_clicked>> execute(const state_type& state,
                                                                                            const draw&)
  {
    ImGui::Begin("Checkbox control panel");

    std::optional<checkbox_toggled> checkbox_toggled_evt;
    std::optional<button_clicked> button_clicked_evt;

    auto checkbox = state.checkbox;
    ImGui::Checkbox("Check me!", &checkbox);
    if (checkbox != state.checkbox) {
      checkbox_toggled_evt = checkbox_toggled{ checkbox };
    }

    if (ImGui::Button("Quit!")) {
      button_clicked_evt = button_clicked{};
    }

    ImGui::End();
    return { checkbox_toggled_evt, button_clicked_evt };
  }

  static state_type apply(const state_type& state, const checkbox_toggled& evt)
  {
    std::cout << "CHECKBOX TOGGLED " << evt.value << std::endl;
    return state_type{ evt.value };
  }

  static draw process(const state_type& state, const render_loop::rendering::started&)
  {
    return draw{};
  }

  static render_loop::input::terminate process(const state_type& state, const button_clicked&)
  {
    return render_loop::input::terminate{};
  }
};

struct event_logger
{
  struct state_type
  {};

  template<typename T>
  static state_type apply(const state_type& state, const T& evt)
  {
    std::cout << "EVENT: " << typeid(evt).name() << std::endl;
    return state;
  }
};

struct entity_gui
{
  struct draw
  {};

  struct create_entity_requested
  {};

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

  using state_type = immer::vector<entity_type>;

  static auto execute(const state_type& state, const draw& cmd)
    -> std::tuple<std::optional<create_entity_requested>, std::optional<transform_entity_requested>>
  {
    std::optional<create_entity_requested> create_entity_evt;
    std::optional<transform_entity_requested> transform_entity_evt;
    ImGui::Begin("Entity Editor");

    if (ImGui::Button("Create")) {
      create_entity_evt = create_entity_requested{};
    }

    for (const auto& entity : state) {
      ImGui::PushID(&entity);
      if (ImGui::TreeNode("Entity")) {
        auto changed = false;
        glm::vec3 position = entity.position;
        glm::vec3 orientation = entity.orientation;

        ImGui::PushID("Position");
        ImGui::Text("Position");
        changed |= ImGui::InputFloat("x", &position.x, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat("y", &position.y, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat("z", &position.z, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Separator();
        ImGui::PopID();

        ImGui::PushID("Orientation");
        ImGui::Text("Orientation");
        changed |= ImGui::InputFloat("pitch", &orientation.x, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat("yaw", &orientation.y, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        changed |= ImGui::InputFloat("roll", &orientation.z, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Separator();
        ImGui::PopID();

        ImGui::TreePop();

        if (changed) {
          transform_entity_evt = { entity.id, position, orientation };
        }
      }
      ImGui::PopID();
    }
    ImGui::End();
    return { create_entity_evt, transform_entity_evt };
  }

  static auto apply(const state_type& state, const physics::entity::created& evt) -> state_type
  {
    auto orientation = glm::eulerAngles(evt.orientation) * 180.0f / 2.0f / glm::pi<float>();
    return state.push_back({ evt.id, evt.position, std::move(orientation) });
  }

  static state_type apply(const state_type& state, const physics::entity::transform_changed& evt)
  {
    auto i = 0;
    for (const auto& entity : state) {
      if (entity.id == evt.id) {
        return state.update(i, [&](auto entity) {
          entity.position = evt.position;
          entity.orientation = glm::eulerAngles(evt.orientation) * 180.0f / 2.0f / glm::pi<float>();
          return entity;
        });
      }
      ++i;
    }
    return state;
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

  static auto process(const state_type& state, const transform_entity_requested& evt) -> physics::entity::transform
  {
    return { evt.id, evt.position, evt.orientation * 2.0f * glm::pi<float>() / 180.0f };
  }
};

int
main()
{
  fiber_scheduler scheduler{ 64, 64 };
  auto main_thread = std::async(std::launch::async, [&scheduler] {
    auto projector = opengl{};
    auto ctx = event_sauce::make_context<render_loop::startup,
                                         render_loop::input,
                                         render_loop::physics,
                                         render_loop::rendering,
                                         physics::entity,
                                         entity_gui>();
    auto dispatch = event_sauce::dispatch(ctx, projector, scheduler);
    dispatch(render_loop::startup::initiate{});
    while (true) {
    }
  });

  fiber_worker(scheduler, 8).run();
  return 0;
}
