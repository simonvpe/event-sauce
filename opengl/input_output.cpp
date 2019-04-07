#include "include/input_output.hpp"

namespace opengl {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// APPLY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

input_output::state_type
input_output::apply(const state_type& state, const window::Created& evt)
{
  glfwSetInputMode(evt.window, GLFW_STICKY_KEYS, GL_TRUE);
  return {};
}

input_output::state_type
input_output::apply(const state_type& state, const render::Finished& evt)
{
  glfwPollEvents();
  return {};
}
}
