#pragma once
#include "shader.hpp"
#include <GL/glew.h>
#include <iostream>
#include <string>
#include <variant>

namespace opengl {
struct program
{

  struct InitiateCreation
  {};

  struct CompleteCreation
  {};

  struct CreationInitiated
  {
    GLuint program;
  };

  struct CreationCompleted
  {
    GLuint program;
  };

  struct CreationFailed
  {
    std::string message;
  };

  struct state_type
  {
    GLuint current_program = {};
  };

  static std::variant<CreationFailed, CreationInitiated> execute(const state_type& state, const InitiateCreation& cmd)
  {
    if (state.current_program != GLuint{}) {
      return CreationFailed{ "Program creation already in progress" };
    }
    const auto program = glCreateProgram();
    return CreationInitiated{ program };
  }

  static std::variant<CreationFailed, CreationCompleted> execute(const state_type& state, const CompleteCreation& cmd)
  {
    if (state.current_program == GLuint{}) {
      return CreationFailed{ "Program creation not in progress" };
    }
    glLinkProgram(state.current_program);

    const auto linked = std::invoke([program = state.current_program] {
      GLint status = 0;
      glGetProgramiv(program, GL_LINK_STATUS, &status);
      return status == GL_TRUE;
    });

    if (!linked) {
      auto message = std::invoke([program = state.current_program] {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

        if (len > 1) {
          std::string buffer(len, '\0');
          glGetProgramInfoLog(program, len, nullptr, buffer.data());
          return std::move(buffer);
        }
        return std::string{};
      });

      glDeleteProgram(state.current_program);
      return CreationFailed{ std::move(message) };
    }

    return CreationCompleted{ state.current_program };
  }

  static state_type apply(const state_type& state, const CreationInitiated& evt)
  {
    return { evt.program };
  }

  static state_type apply(const state_type& state, const CreationCompleted& evt)
  {
    std::cout << "Created program " << evt.program << std::endl;
    return { {} };
  }

  static state_type apply(const state_type& state, const shader::Created& evt)
  {
    if (state.current_program != GLuint{}) {
      glAttachShader(state.current_program, evt.shader);
    }
    return state;
  }

  static state_type apply(const state_type& state, const CreationFailed& evt)
  {
    std::cerr << "Program creation failed" << std::endl << evt.message << std::endl;
    return state;
  }
};
}
