#include "pool_dispatcher.hpp"
#include "processes/input.hpp"
#include "processes/physics.hpp"
#include "processes/rendering.hpp"
#include "processes/startup.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <event-sauce/event-sauce.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

static void
glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

GLFWwindow*
create_window(int width, int height, std::string name)
{
  glfwSetErrorCallback(glfw_error_callback);
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
  glfwSwapInterval(1);

  if (glewInit() != GLEW_OK) {
    glfwTerminate();
    throw std::runtime_error{ "Failed to initialize glew" };
  }

  return window;
}

void
imgui_configure(GLFWwindow* window)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 150");
}

void
imgui_new_frame()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
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
    imgui_configure(window);
  }

  void operator()(const input::terminated& evt)
  {
    std::cout << "terminating" << std::endl;
    destroy_window(window);
    window = nullptr;
  }

  void operator()(const startup::completed& evt)
  {}

  void operator()(const input::collected& evt)
  {
    glfwPollEvents();
    imgui_new_frame();
  }

  void operator()(const rendering::started& evt)
  {}

  void operator()(const rendering::stopped& evt)
  {
    ImGui::Render();
    int width, height;
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
  }

  template<typename Event>
  void operator()(const Event& evt)
  {}
};

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

  static draw process(const state_type& state, const rendering::started&)
  {
    return draw{};
  }

  static input::terminate process(const state_type& state, const button_clicked&)
  {
    return input::terminate{};
  }
};

struct event_logger
{
  struct state_type {};

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
  pool_dispatcher dispatcher{};
  auto projector = opengl{};
  auto ctx = event_sauce::make_context<startup, input, physics, rendering, test_gui, event_logger>();
  auto dispatch = event_sauce::dispatch(ctx, projector, dispatcher);

  dispatcher.install_signal_handler({ SIGINT, SIGTERM }, [&] { dispatch(input::terminate{}); });

  dispatch(startup::initiate{});

  // dispatcher.join();
  return 0;
}
