#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <variant>
#include <iostream>

namespace opengl {
////////////////////////////////////////////////////////////////////////////////
// opengl::window
////////////////////////////////////////////////////////////////////////////////
struct window
{
  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////
  struct Create
  {
    int width;
    int height;
    std::string name;
  };

  struct Terminate
  {};

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////
  struct Created
  {
    GLFWwindow* window;
  };

  struct Terminated
  {};

  struct Error
  {
    std::string description;
  };

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct state_type
  {
    GLFWwindow* window;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Execute
  //////////////////////////////////////////////////////////////////////////////

  static std::variant<Error, Created> execute(const state_type& state, const Create& cmd)
  {
    std::cout << "Creating window" << std::endl;
    if (state.window) {
      return Error{ "Window already created" };
    }

    if (glfwInit() != GLFW_TRUE) {
      return Error{ "Failed to initialize glfw" };
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto* window = glfwCreateWindow(cmd.width, cmd.height, cmd.name.c_str(), nullptr, nullptr);
    if (window == nullptr) {
      return Error{ "Failed to create window" };
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
      return Error{ "Failed to initialize glew" };
    }

    return Created{ window };
  }

  static Terminated execute(const state_type& state, const Terminate& cmd)
  {
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply
  //////////////////////////////////////////////////////////////////////////////

  static state_type apply(const state_type& state, const Created& evt)
  {
    return { evt.window };
  }

  static state_type apply(const state_type& state, const Terminated&)
  {
    glfwTerminate();
    return { nullptr };
  }

  static state_type apply(const state_type& state, const Error& evt)
  {
    std::cerr << "Error: " << evt.description << std::endl;
    glfwTerminate();
    return state;
  }
};
}
