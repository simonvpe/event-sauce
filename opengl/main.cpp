#include "include/dispatch.hpp"
#include "include/input_output.hpp"
#include "include/program.hpp"
#include "include/render.hpp"
#include "include/shader.hpp"
#include "include/window.hpp"
#include <event-sauce/event-sauce.hpp>

int
main()
{
  pool_dispatcher dispatcher{};

  auto ctx =
    event_sauce::make_context<opengl::window, opengl::input_output, opengl::render, opengl::shader, opengl::program>();
  auto projector = [](const auto&) {};
  auto dispatch = event_sauce::dispatch(ctx, projector, dispatcher);

  dispatch(opengl::window::Create{ 1024, 768, "My Window" });
  /*
  dispatch(opengl::program::InitiateCreation{});
  dispatch(opengl::shader::Create{ opengl::shader::type_id::VERTEX_SHADER,
                                   "#version 420\n"
                                   "in vec3 vertex_position;\n"
                                   "void main() {\n"
                                   "  gl_Position = vec4(vertex_position, 1.0);\n"
                                   "}\n" });
  dispatch(opengl::program::CompleteCreation{});
  */
  std::this_thread::sleep_for(std::chrono::seconds{ 2 });
  dispatch(opengl::window::Terminate{});
  return 0;
}
