#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/post.hpp>
#include <boost/thread/thread.hpp>
#include <event-sauce/event-sauce.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <thread>

namespace opengl {
////////////////////////////////////////////////////////////////////////////////
// Common events
////////////////////////////////////////////////////////////////////////////////

struct Error
{
  std::string description;
};

namespace aggregate {
////////////////////////////////////////////////////////////////////////////////
// opengl::aggregate::Window
////////////////////////////////////////////////////////////////////////////////
struct window
{
  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////
  struct Create
  {
    int width;
    int height;
    std::string name;
  };

  struct Terminate
  {};

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////
  struct Created
  {
    GLFWwindow* window;
  };

  struct Terminated
  {};

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct state_type
  {
    GLFWwindow* window;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Execute
  //////////////////////////////////////////////////////////////////////////////

  static std::variant<Error, Created> execute(const state_type& state, const Create& cmd)
  {
    std::cout << "Creating window" << std::endl;
    if (state.window) {
      return Error{ "Window already created" };
    }

    if (glfwInit() != GLFW_TRUE) {
      return Error{ "Failed to initialize glfw" };
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto* window = glfwCreateWindow(cmd.width, cmd.height, cmd.name.c_str(), nullptr, nullptr);
    if (window == nullptr) {
      return Error{ "Failed to create window" };
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
      return Error{ "Failed to initialize glew" };
    }

    return Created{ window };
  }

  static Terminated execute(const state_type& state, const Terminate& cmd)
  {
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply
  //////////////////////////////////////////////////////////////////////////////

  static state_type apply(const state_type& state, const Created& evt)
  {
    return { evt.window };
  }

  static state_type apply(const state_type& state, const Terminated&)
  {
    glfwTerminate();
    return { nullptr };
  }

  static state_type apply(const state_type& state, const Error& evt)
  {
    std::cerr << "Error: " << evt.description << std::endl;
    glfwTerminate();
    return state;
  }
};

struct shader
{
  enum class type_id
  {
    VERTEX_SHADER
  };

  struct Create
  {
    type_id type;
    std::string source;
  };

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

  struct state_type
  {
    bool ready = false;
  };

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

  static state_type apply(const state_type& state, const aggregate::window::Created&)
  {
    return { true };
  }

  static Create process(const state_type& state, const Deferred& evt)
  {
    return evt.command;
  }
};
}

namespace process {
////////////////////////////////////////////////////////////////////////////////
// opengl::process::Render
////////////////////////////////////////////////////////////////////////////////
struct render
{
  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////

  struct Start
  {};

  struct Finish
  {};

  //////////////////////////////////////////////////////////////////////////////
  // Events
  //////////////////////////////////////////////////////////////////////////////

  struct Started
  {};

  struct Finished
  {};

  //////////////////////////////////////////////////////////////////////////////
  // State
  //////////////////////////////////////////////////////////////////////////////

  struct state_type
  {
    GLFWwindow* window = nullptr;
  };

  //////////////////////////////////////////////////////////////////////////////
  // Execute
  //////////////////////////////////////////////////////////////////////////////

  static std::variant<Error, Started> execute(const state_type& state, const Start& cmd)
  {
    if (state.window) {
      glClear(GL_COLOR_BUFFER_BIT);
      return Started{};
    }
    return Error{ "No window" };
  }

  static Finished execute(const state_type& state, const Finish& cmd)
  {
    if (state.window) {
      glfwSwapBuffers(state.window);
    }
    return {};
  }

  //////////////////////////////////////////////////////////////////////////////
  // Apply
  //////////////////////////////////////////////////////////////////////////////

  static state_type apply(const state_type& state, const aggregate::window::Created& evt)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    return { evt.window };
  }

  static state_type apply(const state_type& state, const aggregate::window::Terminated& evt)
  {
    return { nullptr };
  }

  //////////////////////////////////////////////////////////////////////////////
  // Process
  //////////////////////////////////////////////////////////////////////////////

  static Start process(const state_type& state, const aggregate::window::Created& evt)
  {
    return {};
  }

  static Finish process(const state_type& state, const Started& evt)
  {
    return {};
  }

  static std::optional<Start> process(const state_type& state, const Finished& evt)
  {
    if (state.window) {
      return { Start{} };
    }
    return {};
  }
};

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

  static state_type apply(const state_type& state, const aggregate::shader::Created& evt)
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
namespace aggregate {
////////////////////////////////////////////////////////////////////////////////
// opengl::aggregate::io
////////////////////////////////////////////////////////////////////////////////
struct io
{
  struct state_type
  {};

  static state_type apply(const state_type& state, const window::Created& evt)
  {
    glfwSetInputMode(evt.window, GLFW_STICKY_KEYS, GL_TRUE);
    return {};
  }

  static state_type apply(const state_type& state, const process::render::Finished& evt)
  {
    glfwPollEvents();
    return {};
  }
};
}
}

class pool_dispatcher
{
  boost::asio::io_context io_context;
  std::unique_ptr<boost::asio::io_context::work> work;
  boost::thread_group pool;
  boost::asio::io_context::strand serializer;

public:
  pool_dispatcher(int nthreads = std::thread::hardware_concurrency())
      : io_context{}
      , work{ std::make_unique<boost::asio::io_context::work>(io_context) }
      , pool{}
      , serializer{ io_context }
  {
    for (auto i = 0; i < nthreads; ++i) {
      pool.create_thread([this] { io_context.run(); });
    }
  }

  ~pool_dispatcher()
  {
    work.reset();
    pool.join_all();
  }

  auto serial()
  {
    return [this](auto&& fn) { boost::asio::post(serializer, std::forward<decltype(fn)>(fn)); };
  }

  auto concurrent()
  {
    return [this](auto&& fn) { boost::asio::post(io_context, std::forward<decltype(fn)>(fn)); };
  }
};

int
main()
{
  pool_dispatcher dispatcher{};

  int model;
  auto ctx = event_sauce::context<int,
                                  opengl::aggregate::window,
                                  opengl::aggregate::io,
                                  opengl::process::render,
                                  opengl::aggregate::shader,
                                  opengl::process::program>(model);
  ctx.dispatch(opengl::aggregate::window::Create{ 1024, 768, "My Window" }, dispatcher);

  ctx.dispatch(opengl::process::program::InitiateCreation{}, dispatcher);
  ctx.dispatch(opengl::aggregate::shader::Create{ opengl::aggregate::shader::type_id::VERTEX_SHADER,
                                                  "#version 420\n"
                                                  "in vec3 vertex_position;\n"
                                                  "void main() {\n"
                                                  "  gl_Position = vec4(vertex_position, 1.0);\n"
                                                  "}\n" },
               dispatcher);
  ctx.dispatch(opengl::process::program::CompleteCreation{}, dispatcher);

  std::this_thread::sleep_for(std::chrono::seconds{ 2 });
  ctx.dispatch(opengl::aggregate::window::Terminate{}, dispatcher);
  return 0;
}
