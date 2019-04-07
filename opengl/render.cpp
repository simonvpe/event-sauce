#include "include/render.hpp"

namespace opengl {
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXECUTE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::variant<render::Error, render::Started>
render::execute(const state_type& state, const Start& cmd)
{
  if (state.window) {
    glClear(GL_COLOR_BUFFER_BIT);
    return Started{};
  }
  return Error{ "No window" };
}

render::Finished
render::execute(const state_type& state, const Finish& cmd)
{
  if (state.window) {
    glfwSwapBuffers(state.window);
  }
  return {};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// APPLY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

render::state_type
render::apply(const state_type& state, const window::Created& evt)
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  return { evt.window };
}

render::state_type
render::apply(const state_type& state, const window::Terminated& evt)
{
  return { nullptr };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROCESS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

render::Start
render::process(const state_type& state, const window::Created& evt)
{
  return {};
}

render::Finish
render::process(const state_type& state, const Started& evt)
{
  return {};
}

std::optional<render::Start>
render::process(const state_type& state, const Finished& evt)
{
  if (state.window) {
    return { Start{} };
  }
  return {};
}
}
