#pragma once
#include "../utility/shader.hpp"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <tuple>
#include <vector>

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
