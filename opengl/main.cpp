#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

using namespace glm;

GLFWwindow* window;

class WindowContext
{
  GLFWwindow* window;
  std::string window_name;

public:
  WindowContext(int width, int height, const std::string& name)
  {
    { // GLFW
      const auto err = glfwInit();
      if (err != GLFW_TRUE) {
        throw std::runtime_error{ "Failed to initialize glfw" };
      }

      glfwWindowHint(GLFW_SAMPLES, 4);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

      window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
      if (window == nullptr) {
        glfwTerminate();
        throw std::runtime_error{ "Failed to create window" };
      }
      glfwMakeContextCurrent(window);
    }
    {
      const auto err = glewInit();
      if (err != GLEW_OK) {
        glfwTerminate();
        auto error_string = std::string(reinterpret_cast<const char*>(glewGetErrorString(err)));
        throw std::runtime_error{ "Failed to initialize glew " + error_string };
      }
    }
  }

  ~WindowContext() { glfwTerminate(); }

  template<typename F>
  void run_event_loop(F&& callback)
  {
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    do {
      glClear(GL_COLOR_BUFFER_BIT);
      callback();
      glfwSwapBuffers(window);
      glfwPollEvents();
    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
  }

private:
};

int
main()
{
  WindowContext window(1024, 768, "My Window");

  window.run_event_loop([] { std::cout << "Event loop" << std::endl; });
  return 0;
}
