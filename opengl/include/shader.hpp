#pragma once
#include "window.hpp"
#include <GL/glew.h>
#include <string>
#include <variant>
#include <functional>

namespace opengl {
struct shader
{
  //////////////////////////////////////////////////////////////////////////////
  // DEFINITIONS
  //////////////////////////////////////////////////////////////////////////////

  enum class type_id
  {
    VERTEX_SHADER
  };

  //////////////////////////////////////////////////////////////////////////////
  // COMMANDS
  //////////////////////////////////////////////////////////////////////////////

  struct Create
  {
    type_id type;
    std::string source;
  };

  //////////////////////////////////////////////////////////////////////////////
  // EVENTS
  //////////////////////////////////////////////////////////////////////////////

  struct Created
  {
    GLuint shader;
  };

  struct Deferred
  {
    Create command;
  };

  struct Failed
  {
    std::string message;
  };

  //////////////////////////////////////////////////////////////////////////////
  // STATE
  //////////////////////////////////////////////////////////////////////////////

  struct state_type
  {
    bool ready = false;
  };

  //////////////////////////////////////////////////////////////////////////////
  // EXECUTE
  //////////////////////////////////////////////////////////////////////////////

  static std::variant<Created, Deferred, Failed> execute(const state_type& state, const Create& evt)
  {
    if (!state.ready) {
      return Deferred{ evt };
    }

    const GLuint shader = std::invoke([&evt] {
      const auto type = evt.type == type_id::VERTEX_SHADER ? GL_VERTEX_SHADER : -1;
      const auto handle = glCreateShader(GL_VERTEX_SHADER);
      const auto* source = evt.source.data();
      glShaderSource(handle, 1, &source, nullptr);
      glCompileShader(handle);
      return handle;
    });

    const auto compiled = std::invoke([shader] {
      GLint status = GL_FALSE;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
      return status == GL_TRUE;
    });

    if (!compiled) {
      auto message = std::invoke([shader] {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

        if (len > 1) {
          std::string buffer(len, '\0');
          glGetShaderInfoLog(shader, len, nullptr, buffer.data());
          return std::move(buffer);
        }
        return std::string{};
      });
      glDeleteShader(shader);
      return Failed{ std::move(message) };
    }
    return Created{ shader };
  }

  //////////////////////////////////////////////////////////////////////////////
  // APPLY
  //////////////////////////////////////////////////////////////////////////////

  static state_type apply(const state_type& state, const window::Created&)
  {
    return { true };
  }

  //////////////////////////////////////////////////////////////////////////////
  // PROCESS
  //////////////////////////////////////////////////////////////////////////////

  static Create process(const state_type& state, const Deferred& evt)
  {
    return evt.command;
  }
};
}
