#pragma once

#include "../mesh/cube.hpp"
#include "../render-loop/input.hpp"
#include "../render-loop/physics.hpp"
#include "../render-loop/rendering.hpp"
#include "utility/shader.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
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

struct cube
{
  using vertex_type = glm::vec3;
  using color_type = glm::vec4;
  using T = std::tuple<vertex_type, color_type>;

  GLuint vao, vbo, ibo, program;

  std::vector<T> vertices{ // Front
                           { { -1.0, -1.0, +1.0 }, { 0, 0, 1, 1 } },
                           { { +1.0, -1.0, +1.0 }, { 1, 0, 1, 1 } },
                           { { +1.0, +1.0, +1.0 }, { 1, 1, 1, 1 } },
                           { { -1.0, +1.0, +1.0 }, { 0, 1, 1, 1 } },
                           // Right
                           { { +1.0, +1.0, +1.0 }, { 1, 1, 1, 1 } },
                           { { +1.0, +1.0, -1.0 }, { 1, 1, 0, 1 } },
                           { { +1.0, -1.0, -1.0 }, { 1, 0, 0, 1 } },
                           { { -1.0, -1.0, +1.0 }, { 1, 0, 1, 1 } },
                           // Back
                           { { -1.0, -1.0, -1.0 }, { 0, 0, 0, 1 } },
                           { { +1.0, -1.0, -1.0 }, { 1, 0, 0, 1 } },
                           { { +1.0, +1.0, -1.0 }, { 1, 1, 0, 1 } },
                           { { -1.0, +1.0, -1.0 }, { 0, 1, 0, 1 } },
                           // Left
                           { { -1.0, -1.0, -1.0 }, { 0, 0, 0, 1 } },
                           { { -1.0, -1.0, +1.0 }, { 0, 0, 1, 1 } },
                           { { -1.0, +1.0, +1.0 }, { 0, 1, 1, 1 } },
                           { { -1.0, +1.0, -1.0 }, { 0, 1, 0, 1 } },
                           // Up
                           { { +1.0, +1.0, +1.0 }, { 1, 1, 1, 1 } },
                           { { -1.0, +1.0, +1.0 }, { 0, 1, 1, 1 } },
                           { { -1.0, +1.0, -1.0 }, { 0, 1, 0, 1 } },
                           { { +1.0, +1.0, -1.0 }, { 1, 1, 0, 1 } },
                           // Down
                           { { -1.0, -1.0, -1.0 }, { 0, 0, 0, 1 } },
                           { { +1.0, -1.0, -1.0 }, { 1, 0, 0, 1 } },
                           { { +1.0, -1.0, 11.0 }, { 1, 0, 1, 1 } },
                           { { -1.0, -1.0, +1.0 }, { 0, 0, 1, 1 } }
  };

  std::vector<GLuint> indices{ // Front
                               0,
                               1,
                               2,
                               0,
                               2,
                               3,
                               // Right
                               4,
                               5,
                               6,
                               4,
                               6,
                               7,
                               // Back
                               8,
                               9,
                               10,
                               8,
                               10,
                               11,
                               // Left
                               12,
                               13,
                               14,
                               12,
                               14,
                               15,
                               // Up
                               16,
                               17,
                               18,
                               16,
                               18,
                               19,
                               // Down
                               20,
                               21,
                               22,
                               20,
                               22,
                               23
  };

  glm::vec3 rotation;

  void render()
  {
    const auto projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 100.0f);
    const auto view = glm::lookAt(glm::vec3(4, 3, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    rotation = rotation + glm::vec3(0.1f, 0.0f, 0.0f);
    glm::vec3 rotation_sin = glm::vec3(rotation.x * glm::pi<float>() / 180.0,
                                       rotation.y * glm::pi<float>() / 180.0,
                                       rotation.z * glm::pi<float>() / 180.0);
    glUseProgram(program);
    glUniform3f(glGetUniformLocation(program, "rotation"), rotation_sin.x, rotation_sin.y, rotation_sin.z);
    glUniformMatrix4fv(glGetUniformLocation(program, "view_matrix"), 1, false, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection_matrix"), 1, false, &projection[0][0]);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
  }

  void init()
  {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(T), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(T), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(T), (void*)(sizeof(glm::vec3)));
    glBindVertexArray(0);

    program = LoadShaders("../opengl/shader/cube-vertex.glsl", "../opengl/shader/cube-fragment.glsl");
  }
};

class opengl
{
  GLFWwindow* window;
  cube m_cube;

public:
  void operator()(const render_loop::startup::initiated& evt)
  {
    window = create_window(1024, 768, "My Window");
    imgui_configure(window);
    m_cube.init();
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
    glClear(GL_COLOR_BUFFER_BIT);

    m_cube.render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
  }

  template<typename Event>
  void operator()(const Event& evt)
  {}
};
