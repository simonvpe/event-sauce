#pragma once
#include "window.hpp"
#include <GL/glew.h>
#include <variant>

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

  static std::variant<Created, Deferred, Failed> execute(const state_type& state, const Create& evt);

  //////////////////////////////////////////////////////////////////////////////
  // APPLY
  //////////////////////////////////////////////////////////////////////////////

  static state_type apply(const state_type& state, const window::Created&);

  //////////////////////////////////////////////////////////////////////////////
  // PROCESS
  //////////////////////////////////////////////////////////////////////////////

  static Create process(const state_type& state, const Deferred& evt);
};
}
