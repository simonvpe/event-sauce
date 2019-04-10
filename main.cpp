#include "pool_dispatcher.hpp"
#include "processes/input.hpp"
#include "processes/physics.hpp"
#include "processes/rendering.hpp"
#include "processes/startup.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <event-sauce/event-sauce.hpp>
#include <iostream>

GLFWwindow*
create_window(int width, int height, std::string name)
{
  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error{ "Falied to initialize GLFW" };
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  auto* window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
  if (window == nullptr) {
    throw std::runtime_error{ "Failed to create window" };
  }

  glfwMakeContextCurrent(window);

  if (glewInit() != GLEW_OK) {
    glfwTerminate();
    throw std::runtime_error{ "Failed to initialize glew" };
  }

  return window;
}

void
destroy_window(GLFWwindow* window)
{
  glfwDestroyWindow(window);
  glfwTerminate();
}

class opengl
{
  GLFWwindow* window;

public:
  void operator()(const startup::initiated& evt)
  {
    window = create_window(1024, 768, "My Window");
    log(evt);
  }

  void operator()(const input::terminated& evt)
  {
    log(evt);
    destroy_window(window);
    window = nullptr;
  }

  void operator()(const startup::completed& evt)
  {
    log(evt);
  }

  void operator()(const input::collected& evt)
  {
    glfwPollEvents();
    log(evt);
  }

  void operator()(const rendering::started& evt)
  {
    log(evt);
  }

  void operator()(const rendering::stopped& evt)
  {
    int width, height;
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
    log(evt);
  }

  template<typename Event>
  void operator()(const Event& evt)
  {
    log(evt);
  }

  template<typename Event>
  void log(const Event& evt)
  {
    std::cerr << typeid(evt).name() << std::endl;
  }
};

int
main()
{
  pool_dispatcher dispatcher{};
  auto projector = opengl{};
  auto ctx = event_sauce::make_context<startup, input, physics, rendering>();
  auto dispatch = event_sauce::dispatch(ctx, projector, dispatcher);

  dispatcher.install_signal_handler({ SIGINT, SIGTERM }, [&] { dispatch(input::terminate{}); });

  dispatch(startup::initiate{});

  // dispatcher.join();
  return 0;
}
