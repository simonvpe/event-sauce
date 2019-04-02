#define GLFW_INCLUDE_VULKAN
#undef VULKAN_HPP_DISABLE_ENHANCED_MODE
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkano.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>

class HelloTriangleApplication
{
  GLFWwindow* window;
  vk::Instance instance;
  vk::PhysicalDevice physical_device;

  const std::vector<const char*> validation_layers = { "VK_LAYER_LUNARG_standard_validation" };

#ifdef NDEBUG
  static constexpr bool enable_validation_layers = false;
#else
  static constexpr bool enable_validation_layers = true;
#endif

public:
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  void initWindow()
  {
    constexpr auto width = 800;
    constexpr auto height = 600;
    constexpr auto name = "Vulkan";

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, name, nullptr, nullptr);
  }

  void initVulkan()
  {
    createInstance();
    setupDebugMessenger();
    pick_physical_device();
  }

  void createInstance()
  {
    constexpr auto app_name = "My Application";
    constexpr auto app_version = VK_MAKE_VERSION(1, 0, 0);
    constexpr auto engine_name = "No Engine";
    constexpr auto engine_version = VK_MAKE_VERSION(1, 0, 0);
    constexpr auto api_version = VK_API_VERSION_1_0;

    const auto app_info = vk::ApplicationInfo{ app_name, app_version, engine_name, engine_version, api_version };

    if (enable_validation_layers) {
      check_validation_layer_support();
    }
    const auto enabled_layer_count = validation_layers.size();
    const auto layer_names = validation_layers.data();
    const auto create_info =
      vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &app_info, enabled_layer_count, layer_names);

    if (vk::createInstance(&create_info, nullptr, &instance) != vk::Result::eSuccess) {
      throw std::runtime_error{ "Failed to create vulkan instance!" };
    }

    check_required_extensions();
  }

  void check_required_extensions()
  {
    const auto required = std::invoke([] {
      auto glfw_extension_count = uint32_t{};
      const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
      auto extensions = std::vector<const char*>(glfw_extensions, glfw_extensions + glfw_extension_count);
      if (enable_validation_layers) {
        extensions.push_back({ VK_EXT_DEBUG_UTILS_EXTENSION_NAME });
      }
      return extensions;
    });

    const auto available = std::invoke([] {
      const auto extensions = vk::enumerateInstanceExtensionProperties();
      auto extension_names = std::vector<std::string>(extensions.size());
      std::transform(
        cbegin(extensions), cend(extensions), begin(extension_names), [](const auto& x) { return x.extensionName; });
      return extension_names;
    });

    for (const auto& req : required) {
      const auto result = std::find(cbegin(available), cend(available), req);
      if (result == cend(available)) {
        throw std::runtime_error{ "Required extension \"" + std::string{ req } + "\" not found!" };
      }
      std::cerr << "Extension found: " << req << std::endl;
    }
  }

  void check_validation_layer_support()
  {
    const auto available = std::invoke([] {
      const auto layers = vk::enumerateInstanceLayerProperties();
      auto layer_names = std::vector<std::string>(layers.size());
      std::transform(cbegin(layers), cend(layers), begin(layer_names), [](const auto& x) { return x.layerName; });
      return layer_names;
    });

    for (const auto* layer : validation_layers) {
      const auto result = std::find(cbegin(available), cend(available), std::string{ layer });
      if (result == cend(available)) {
        throw std::runtime_error{ "Validation layer \"" + std::string{ layer } + "\" unavailable!" };
      }
    }
  }

  void pick_physical_device()
  {
    //const auto devices = vk::enumeratePhysicalDevices();
  }

  void setupDebugMessenger()
  {
    if (!enable_validation_layers) {
      return;
    }

    //const auto create_info = vk::DebugUtilsMessengerCreateInfoEXT{};
    //vk::createDebugUtilsMessengerEXTUnique(create_info);
    // TODO: Debug utils messenger
  }

  void mainLoop()
  {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  void cleanup()
  {
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};

int
main()
{
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*
#include <vulkano.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <iostream>

int main() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

  uint32_t extensionCount = 0;
  vk::enumerateInstanceExtensionProperties({}, &extensionCount, {});

  std::cout << extensionCount << " extensions supported" << std::endl;

  glm::mat4 matrix;
  glm::vec4 vec;
  auto test = matrix * vec;

  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);

  glfwTerminate();

  return 0;
}
*/
