#pragma once
#include "shader.hpp"
#include <GL/glew.h>
#include <functional>
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

  //////////////////////////////////////////////////////////////////////////////
  // EXECUTE
  //////////////////////////////////////////////////////////////////////////////

  static std::variant<CreationFailed, CreationInitiated> execute(const state_type& state, const InitiateCreation& cmd);

  static std::variant<CreationFailed, CreationCompleted> execute(const state_type& state, const CompleteCreation& cmd);

  //////////////////////////////////////////////////////////////////////////////
  // APPLY
  //////////////////////////////////////////////////////////////////////////////

  static state_type apply(const state_type& state, const CreationInitiated& evt);

  static state_type apply(const state_type& state, const CreationCompleted& evt);

  static state_type apply(const state_type& state, const shader::Created& evt);

  static state_type apply(const state_type& state, const CreationFailed& evt);
};
}
