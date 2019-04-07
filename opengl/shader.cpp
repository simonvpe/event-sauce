#include "include/shader.hpp"
#include <functional>
#include <string>

namespace opengl {
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXECUTE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::variant<shader::Created, shader::Deferred, shader::Failed>
shader::execute(const state_type& state, const Create& evt)
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// APPLY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

shader::state_type
shader::apply(const state_type& state, const window::Created&)
{
  return { true };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROCESS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
shader::Create
shader::process(const state_type& state, const Deferred& evt)
{
  return evt.command;
}

}
