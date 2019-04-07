#pragma once
#include "render.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>

namespace opengl {
struct input_output
{
  struct state_type
  {};

  static state_type apply(const state_type& state, const window::Created& evt)
  {
    glfwSetInputMode(evt.window, GLFW_STICKY_KEYS, GL_TRUE);
    return {};
  }

  static state_type apply(const state_type& state, const render::Finished& evt)
  {
    glfwPollEvents();
    return {};
  }
};

}
