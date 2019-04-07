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

  int model;
  auto ctx =
    event_sauce::context<int, opengl::window, opengl::input_output, opengl::render, opengl::shader, opengl::program>(
      model);
  ctx.dispatch(opengl::window::Create{ 1024, 768, "My Window" }, dispatcher);

  ctx.dispatch(opengl::program::InitiateCreation{}, dispatcher);
  ctx.dispatch(opengl::shader::Create{ opengl::shader::type_id::VERTEX_SHADER,
                                       "#version 420\n"
                                       "in vec3 vertex_position;\n"
                                       "void main() {\n"
                                       "  gl_Position = vec4(vertex_position, 1.0);\n"
                                       "}\n" },
               dispatcher);
  ctx.dispatch(opengl::program::CompleteCreation{}, dispatcher);

  std::this_thread::sleep_for(std::chrono::seconds{ 2 });
  ctx.dispatch(opengl::window::Terminate{}, dispatcher);
  return 0;
}
