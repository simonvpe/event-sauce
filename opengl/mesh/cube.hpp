#pragma once
#include "../utility/shader.hpp"
#include <GL/glew.h>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <tuple>

struct cube_data
{
  std::tuple<glm::vec3, glm::vec4> vertices[24] = {
    // Front
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

  GLuint indices[6][6] = {
    // Front
    { 0, 1, 2, 0, 2, 3 },
    // Right
    { 4, 5, 6, 4, 6, 7 },
    // Back
    { 8, 9, 10, 8, 10, 11 },
    // Left
    { 12, 13, 14, 12, 14, 15 },
    // Up
    { 16, 17, 18, 16, 18, 19 },
    // Down
    { 20, 21, 22, 20, 22, 23 }
  };
};

struct cube
{
  GLuint program;
  const cube_data data;
  GLuint vao, vbo, ibo, instance_vbo;
  std::vector<glm::mat4> transforms;

  void init()
  {
    //transforms = { 2, glm::mat4{ 1.0f } };
    program = LoadShaders("../opengl/shader/cube-vertex.glsl", "../opengl/shader/cube-fragment.glsl");
    //transforms[1] = glm::translate(transforms[1], glm::vec3{ 1.1f, 0.0f, 0.0f });

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data.vertices), data.vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data.indices), data.indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(data.vertices[0]), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(data.vertices[0]), (void*)(sizeof(glm::vec3)));

    glGenBuffers(1, &instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
    regenerate_buffers();

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);

    // Unbind
    glBindVertexArray(0);
  }

  void push()
  {
    transforms.push_back(glm::mat4{ 1.0f });
    regenerate_buffers();
  }

  void pop()
  {
    transforms.pop_back();
    regenerate_buffers();
  }

  void transform(std::size_t idx, const glm::mat4& tfm)
  {
    transforms[idx] = tfm;
    glBufferSubData(instance_vbo, idx * sizeof(glm::mat4), sizeof(glm::mat4), &tfm);
  }

  glm::mat4 transform(std::size_t idx) const
  {
    return transforms[idx];
  }

  void regenerate_buffers()
  {
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * transforms.size(), transforms.data(), GL_DYNAMIC_DRAW);
  }

  void render(const glm::mat4& projection, const glm::mat4& view)
  {
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "view_matrix"), 1, false, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection_matrix"), 1, false, &projection[0][0]);
    glBindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, transforms.size());
    glBindVertexArray(0);
  }
};
