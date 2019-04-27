#pragma once

#include "../render-loop/input.hpp"
#include "../render-loop/physics.hpp"
#include "../render-loop/rendering.hpp"
#include "utility/shader.hpp"
#include <GL/glew.h>
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
  GLuint vertex_array_id;
  GLuint vertexbuffer;
  GLuint programId;
  GLfloat vertex_buffer_data[9] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

public:
  void operator()(const render_loop::startup::initiated& evt)
  {
    window = create_window(1024, 768, "My Window");
    imgui_configure(window);
    ///
    glGenVertexArrays(1, &vertex_array_id);
    glBindVertexArray(vertex_array_id);
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);
    programId = LoadShaders("../opengl/shader/tutorial-vertex.glsl", "../opengl/shader/tutorial-fragment.glsl");
    ///
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
  {

  }

  void operator()(const render_loop::rendering::stopped& evt)
  {
    ImGui::Render();
    int width, height;
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ///
    glUseProgram(programId);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
                          3,        // size
                          GL_FLOAT, // type
                          GL_FALSE, // normalized?
                          0,        // stride
                          (void*)0  // array buffer offset
                          );
    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDisableVertexAttribArray(0);
    ///
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
  }

  template<typename Event>
  void operator()(const Event& evt)
  {}
};
