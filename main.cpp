#include "gui/entity_browser.hpp"
#include "opengl/opengl.hpp"
#include "physics/entity.hpp"
#include "pool_dispatcher.hpp"
#include "render-loop/startup.hpp"
#include "scheduler/fiber.hpp"
#include <event-sauce/event-sauce.hpp>
#include <iostream>

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
                                         physics::entity,
                                         mesh::cube,
                                         gui::entity_browser>();
    auto dispatch = event_sauce::dispatch(ctx, projector, scheduler);
    dispatch(render_loop::startup::initiate{});
    while (true) {
    }
  });

  fiber_worker(scheduler, 8).run();
  return 0;
}
