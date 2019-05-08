#pragma once

#include "../mesh/cube.hpp"
#include "../render-loop/input.hpp"
#include "../render-loop/physics.hpp"
#include "../render-loop/rendering.hpp"
#include "mesh/cube.hpp"
#include "utility/shader.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

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
  cube m_cube;

public:
  void operator()(const render_loop::startup::initiated& evt)
  {
    window = create_window(1024, 768, "My Window");
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    imgui_configure(window);
    m_cube.init();
    m_cube.push();
    m_cube.transform(0, glm::translate(m_cube.transform(0), { 1.1f, 0.0f, 0.0f }));
    m_cube.push();
    m_cube.transform(1, glm::translate(m_cube.transform(1), { 0.0f, 1.1f, 0.0f }));
    m_cube.push();
  }

  void operator()(const render_loop::input::terminated& evt)
  {
    std::cout << "terminating" << std::endl;
    destroy_window(window);
    window = nullptr;
  }

  void operator()(const render_loop::startup::completed& evt)
  {}

  void operator()(const render_loop::input::collected& evt)
  {
    glfwPollEvents();
    imgui_new_frame();
  }

  void operator()(const render_loop::rendering::started& evt)
  {}

  void operator()(const render_loop::rendering::stopped& evt)
  {
    ImGui::Render();
    int width, height;
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto perspective = (float)width / (float)height;
    const auto projection = glm::perspective(glm::radians(45.0f), perspective, 0.1f, 100.0f);
    const auto view = glm::lookAt(glm::vec3(4, 3, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    m_cube.render(projection, view);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
  }

  template<typename Event>
  void operator()(const Event& evt)
  {}
};
