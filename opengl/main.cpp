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
      return Started{};
    }
    return Error{ "No window" };
  }

  static Finished execute(const state_type& state, const Finish& cmd)
  {
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

  static state_type apply(const state_type& state, const Started& evt)
  {
    glClear(GL_COLOR_BUFFER_BIT);
    return state;
  }

  static state_type apply(const state_type& state, const Finished& evt)
  {
    if (state.window) {
      glfwSwapBuffers(state.window);
    }
    return state;
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
    std::cout << "Rendering finished" << std::endl;
    return {};
  }

  static std::optional<Start> process(const state_type& state, const Finished& evt)
  {
    if (!state.window) {
      std::cout << "Rendering aborted because there is no window to render to" << std::endl;
      return {};
    }
    std::cout << "Rendering completed" << std::endl;
    return { Start{} };
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
  auto ctx =
    event_sauce::context<int, opengl::aggregate::window, opengl::aggregate::io, opengl::process::render>(model);
  ctx.dispatch(opengl::aggregate::window::Create{ 1024, 768, "My Window" }, dispatcher);

  std::this_thread::sleep_for(std::chrono::seconds{ 2 });
  ctx.dispatch(opengl::aggregate::window::Terminate{}, dispatcher);

  return 0;
}
