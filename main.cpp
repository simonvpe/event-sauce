#include "opengl/opengl.hpp"
#include "physics/entity.hpp"
#include "pool_dispatcher.hpp"
#include "render-loop/startup.hpp"
#include "scheduler/fiber.hpp"
#include <event-sauce/event-sauce.hpp>
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
                                         test_gui,
                                         event_logger>();
    auto dispatch = event_sauce::dispatch(ctx, projector, scheduler);
    dispatch(render_loop::startup::initiate{});
    while (true) {
    }
  });

  fiber_worker(scheduler, 8).run();
  return 0;
}
