#include "../opengl/include/dispatch.hpp"
#include "../opengl/include/input_output.hpp"
#include "../opengl/include/program.hpp"
#include "../opengl/include/render.hpp"
#include "../opengl/include/shader.hpp"
#include "../opengl/include/window.hpp"
#include <event-sauce/event-sauce.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace engine::imgui {
struct context
{

  ////////////////////////////////////////////////////////////////////////////////
  // COMMANDS
  ////////////////////////////////////////////////////////////////////////////////
  struct Create
  {};

  struct StartFrame
  {};

  ////////////////////////////////////////////////////////////////////////////////
  // EVENTS
  ////////////////////////////////////////////////////////////////////////////////
  struct Created
  {};

  struct Deferred
  {};

  struct FrameStarted
  {};

  ////////////////////////////////////////////////////////////////////////////////
  // STATE
  ////////////////////////////////////////////////////////////////////////////////
  struct state_type
  {
    GLFWwindow* window = nullptr;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // EXECUTE
  ////////////////////////////////////////////////////////////////////////////////
  static std::variant<Deferred, Created> execute(const state_type& state, const Create& cmd)
  {
    if (state.window) {
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO();
      // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls
      // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable gamepad controls

      ImGui::StyleColorsDark();
      const char* glsl_version = "#version 420";
      ImGui_ImplGlfw_InitForOpenGL(state.window, true);
      ImGui_ImplOpenGL3_Init(glsl_version);
      return Created{};
    }
    return Deferred{};
  }

  static FrameStarted execute(const state_type& state, const StartFrame& cmd)
  {
    return {};
  }

  ////////////////////////////////////////////////////////////////////////////////
  // APPLY
  ////////////////////////////////////////////////////////////////////////////////

  static state_type apply(const state_type& state, const opengl::window::Created& evt)
  {
    std::cerr << "Imgui context created" << std::endl;
    return { evt.window };
  }

  static state_type apply(const state_type& state, const opengl::render::EndOfFrameRequested& evt)
  {
    if (state.window) {
      ImGui::Render();
    }
    return state;
  }

  static state_type apply(const state_type& state, const opengl::render::Finished& evt)
  {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return state;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // PROCESS
  ////////////////////////////////////////////////////////////////////////////////
  static Create process(const state_type& state, const Deferred& evt)
  {
    std::cerr << "Imgui context creation deferred" << std::endl;
    return {};
  }

  static StartFrame process(const state_type& state, const opengl::render::Started& evt)
  {
    if (state.window) {
      glfwPollEvents();
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ImGui::Begin("Hello, World!");
      ImGui::End();
    }
    return {};
  }
};
}

int
main()
{
  pool_dispatcher dispatcher;
  int model;

  auto ctx = event_sauce::context<int,
                                  opengl::window,
                                  opengl::input_output,
                                  opengl::render,
                                  opengl::shader,
                                  opengl::program>(model);

  ctx.dispatch(opengl::window::Create{ 1024, 768, "Imgui test" }, dispatcher);
  //ctx.dispatch(engine::imgui::context::Create{}, dispatcher);
  std::this_thread::sleep_for(std::chrono::seconds{ 2 });
  ctx.dispatch(opengl::window::Terminate{}, dispatcher);

  return 0;
}
