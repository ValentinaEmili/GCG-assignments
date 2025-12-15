/*
 * Copyright 2023 TU Wien, Institute of Visual Computing & Human-Centered Technology.
 * This file is part of the GCG Lab Framework and must not be redistributed.
 *
 * Original version created by Lukas Gersthofer and Bernhard Steiner.
 * Vulkan edition created by Johannes Unterguggenberger (junt@cg.tuwien.ac.at).
 */
#include <complex>

#include "PathUtils.h"
#include "Utils.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <glm/gtx/rotate_vector.hpp>

#undef min
#undef max

constexpr char WELCOME_MSG[] = ":::::: WELCOME TO GCG 2025 ::::::";
constexpr char WINDOW_TITLE[] = "GCG 2025";


/* --------------------------------------------- */
// Helper Function Declarations
/* --------------------------------------------- */

/*!
 *	From the given list of physical devices, select the first one that satisfies all requirements.
 *	@param		physical_devices		A pointer which points to contiguous memory of #physical_device_count sequentially
                                        stored VkPhysicalDevice handles is expected. The handles can (or should) be those
 *										that are returned from vkEnumeratePhysicalDevices.
 *	@param		physical_device_count	The number of consecutive physical device handles there are at the memory location
 *										that is pointed to by the physical_devices parameter.
 *	@param		surface					A valid VkSurfaceKHR handle, which is used to determine if a certain
 *										physical device supports presenting images to the given surface.
 *	@return		The index of the physical device that satisfies all requirements is returned.
 */
uint32_t selectPhysicalDeviceIndex(const VkPhysicalDevice* physical_devices, uint32_t physical_device_count, VkSurfaceKHR surface);

/*!
 *	From the given list of physical devices, select the first one that satisfies all requirements.
 *	@param		physical_devices	A vector containing all available VkPhysicalDevice handles, like those
 *									that are returned from vkEnumeratePhysicalDevices.
 *	@param		surface				A valid VkSurfaceKHR handle, which is used to determine if a certain
 *									physical device supports presenting images to the given surface.
 *	@return		The index of the physical device that satisfies all requirements is returned.
 */
uint32_t selectPhysicalDeviceIndex(const std::vector<VkPhysicalDevice>& physical_devices, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, select a queue family which supports both,
 *	graphics and presentation to the given surface. Return the INDEX of an appropriate queue family!
 *	@return		The index of a queue family which supports the required features shall be returned.
 */
uint32_t selectQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, a the physical device's surface capabilites are read and returned.
 *	@return		VkSurfaceCapabilitiesKHR data
 */
VkSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, a supported surface image format
 *	which can be used for the framebuffer's attachment formats is searched and returned.
 *	@return		A supported format is returned.
 */
VkSurfaceFormatKHR getSurfaceImageFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, return its surface transform flag.
 *	This can be used to set the swap chain to the same configuration as the surface's current transform.
 *	@return		The surface capabilities' currentTransform value is returned, which is suitable for swap chain config.
 */
VkSurfaceTransformFlagBitsKHR getSurfaceTransform(VkPhysicalDevice physical_device, VkSurfaceKHR surface);


/*!
 *	This callback function gets invoked by GLFW whenever a GLFW error occured.
 */
void errorCallbackFromGlfw(int error, const char* description) { std::cout << "GLFW error " << error << ": " << description << std::endl; }

bool is_wireframe = false;
int cull_mode_idx = 0; // 0: NONE; 1: BACK

void keyCallbackFromGlfw(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // When ESC is pressed, mark window to close
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        is_wireframe = !is_wireframe;
    }

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        cull_mode_idx = (cull_mode_idx + 1) % 2;
    }
}

struct UniformBufferObject {
    alignas(16) glm::vec4 color;
    alignas(16) glm::mat4 view_projection;
};

// Subtask 2.6: Orbit Camera
float zoom = 5.0f; // initial radium
float yaw = 0.0f;
float pitch = 0.0f;
float max_pitch = glm::radians(89.999f);
float min_pitch = glm::radians(-89.999f);
float last_x = 0.0f;
float last_y = 0.0f;
bool first_mouse = true;

glm::vec3 camera_pos;
glm::vec3 target(0.0f, 0.0f, 0.0f);
glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);

void mouse_callback(GLFWwindow* window, double x_pos, double y_pos);
void scroll_callback(GLFWwindow* window, double x_offset, double y_offset);

// Subtask 4.4: Test Scene
float cube_width = 0.34f;
float cube_depth = 0.34f;
float cube_height = 0.34f;

// Subtask 3.8: Test Scene
//float cube_width = 2.0f;
//float cube_height = 1.3f;
//float cube_depth = 1.3f;

struct cubeVertex {
    glm::vec3 position;
    glm::vec3 color;
};

std::vector<cubeVertex> cubeVertices_temp = {
    // front
    {{-cube_width / 2, -cube_height / 2, cube_depth / 2}, {0.75f, 0.25f, 0.01f}}, // left-bottom
    {{cube_width / 2, -cube_height / 2, cube_depth / 2}, {0.75f, 0.25f, 0.01f}}, // right-bottom
    {{cube_width / 2, cube_height / 2, cube_depth / 2}, {0.75f, 0.25f, 0.01f}}, // right-top
    {{-cube_width / 2, cube_height / 2, cube_depth / 2}, {0.75f, 0.25f, 0.01f}}, // left-top

    // back
    {{-cube_width / 2, -cube_height / 2, -cube_depth / 2}, {0.75f, 0.25f, 0.01f}}, // left-bottom
    {{cube_width / 2, -cube_height / 2, -cube_depth / 2}, {0.75f, 0.25f, 0.01f}}, // right-bottom
    {{cube_width / 2, cube_height / 2, -cube_depth / 2}, {0.75f, 0.25f, 0.01f}}, // right-top
    {{-cube_width / 2, cube_height / 2, -cube_depth / 2}, {0.75f, 0.25f, 0.01f}}, // left-top
};

// Subtask 3.5: Cube Geometry

//float cube_width = 1.0f;
//float cube_height = 1.0f;
//float cube_depth = 1.0f;

std::vector<glm::vec3> cube_vertices_temp = {
    // front
    {-cube_width / 2, -cube_height / 2, cube_depth / 2}, // left-bottom
    {cube_width / 2, -cube_height / 2, cube_depth / 2}, // right-bottom
    {cube_width / 2, cube_height / 2, cube_depth / 2}, // right-top
    {-cube_width / 2, cube_height / 2, cube_depth / 2}, // left-top

    // back
    {-cube_width / 2, -cube_height / 2, -cube_depth / 2}, // left-bottom
    {cube_width / 2, -cube_height / 2, -cube_depth / 2}, // right-bottom
    {cube_width / 2, cube_height / 2, -cube_depth / 2}, // right-top
    {-cube_width / 2, cube_height / 2, -cube_depth / 2}, // left-top
};

std::vector<uint32_t> cube_indices = {
    // front
    0, 1, 2, 0, 2, 3,
    // back
    4, 6, 5, 4, 7, 6,
    // left
    0, 3, 7, 0, 7, 4,
    // right
    1, 6, 2, 1, 5, 6,
    // top
    3, 2, 6, 3, 6, 7,
    // bottom
    0, 5, 1, 0, 4, 5
};

// Subtask 3.6 - 3.7: Cornell Box
struct CornellVertex {
    glm::vec3 position;
    glm::vec3 color;
};

float cornell_width = 3.0f;
float cornell_height = 3.0f;
float cornell_depth = 3.0f;

std::vector<CornellVertex> cornell_vertices_temp = {
    // back
    {{-cornell_width / 2, -cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.76, 0.74f, 0.68f)},
    {{-cornell_width / 2, cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.76, 0.74f, 0.68f)},
    {{cornell_width / 2, cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.76, 0.74f, 0.68f)},
    {{cornell_width / 2, -cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.76, 0.74f, 0.68f)},
    // left
    {{-cornell_width / 2, -cornell_height / 2, cornell_depth / 2}, glm::vec3(0.49f, 0.06f, 0.22f)},
    {{-cornell_width / 2, cornell_height / 2, cornell_depth / 2}, glm::vec3(0.49f, 0.06f, 0.22f)},
    {{-cornell_width / 2, cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.49f, 0.06f, 0.22f)},
    {{-cornell_width / 2, -cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.49f, 0.06f, 0.22f)},
    // right
    {{cornell_width / 2, -cornell_height / 2, cornell_depth / 2}, glm::vec3(0.0f, 0.13f, 0.31f)},
    {{cornell_width / 2, cornell_height / 2, cornell_depth / 2}, glm::vec3(0.0f, 0.13f, 0.31f)},
    {{cornell_width / 2, cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.0f, 0.13f, 0.31f)},
    {{cornell_width / 2, -cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.0f, 0.13f, 0.31f)},
    // top
    {{-cornell_width / 2, cornell_height / 2, cornell_depth / 2}, glm::vec3(0.96f, 0.93f, 0.85f)},
    {{-cornell_width / 2, cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.96f, 0.93f, 0.85f)},
    {{cornell_width / 2, cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.96f, 0.93f, 0.85f)},
    {{cornell_width / 2, cornell_height / 2, cornell_depth / 2}, glm::vec3(0.96f, 0.93f, 0.85f)},
    // bottom
    {{-cornell_width / 2, -cornell_height / 2, cornell_depth / 2}, glm::vec3(0.64f, 0.64f, 0.64f)},
    {{-cornell_width / 2, -cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.64f, 0.64f, 0.64f)},
    {{cornell_width / 2, -cornell_height / 2, -cornell_depth / 2}, glm::vec3(0.64f, 0.64f, 0.64f)},
    {{cornell_width / 2, -cornell_height / 2, cornell_depth / 2}, glm::vec3(0.64f, 0.64f, 0.64f)},
};

std::vector<uint32_t> cornell_indices = {
    // back
    0, 3, 1, 3, 2, 1,
    // left
    4, 7, 5, 7, 6, 5,
    // right
    8, 9, 10, 8, 10, 11,
    // top
    12, 13, 14, 12, 14, 15,
    // bottom
    16, 18, 17, 16, 19, 18
};

// Subtask 4.1: Cylinder Geometry
struct MeshResources {
    UniformBufferObject ubo;
    VkBuffer uniformBuffer;
    VkDeviceSize uniformBufferSize;
    VkPipeline pipelines[4];
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
};
MeshResources createMesh(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices, glm::mat4 view_projection, GLFWwindow* window, VkDescriptorSet descriptor_set, VkDevice vk_device);

void buildCylinder(float h, float r, uint32_t n, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices);

// Subtask 4.2: Sphere Geometry
void buildSphere(uint32_t n, uint32_t m, float r, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices);

// Subtask 4.3: Bézier Cylinder Geometry
glm::vec3 bezierPoint(std::vector<glm::vec3>& vertices, float t);
glm::vec3 derivativeBezierPoint(std::vector<glm::vec3>& vertices, float t);
void buildBezierCylinder(uint32_t s, uint32_t n, float r, std::vector<glm::vec3>& controlPoints, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices);
/* --------------------------------------------- */
// Main
/* --------------------------------------------- */

int main(int argc, char** argv) {
    VKL_LOG(WELCOME_MSG);

    CMDLineArgs cmdline_args;
    gcgParseArgs(cmdline_args, argc, argv);

    /* --------------------------------------------- */
    // Subtask 1.1: Load Settings From File
    /* --------------------------------------------- */
    INIReader window_reader("assets/settings/window.ini");
    int width = static_cast<int>(window_reader.GetInteger("window", "width", 800));
    int height = static_cast<int>(window_reader.GetInteger("window", "height", 800));
    bool fullscreen = window_reader.GetBoolean("window", "fullscreen", false);
    std::string window_title = window_reader.Get("window", "title", "GCG 2025");
    // Install a callback function, which gets invoked whenever a GLFW error occurred.
    glfwSetErrorCallback(errorCallbackFromGlfw);

    // Subtask 3.3: Interaction
    std::string init_renderer_filepath = "assets/settings/renderer_standard.ini";
    if (cmdline_args.init_renderer) {
        init_renderer_filepath = cmdline_args.init_renderer_filepath;
    }
    INIReader renderer_reader(init_renderer_filepath);
    is_wireframe = renderer_reader.GetBoolean("renderer", "wireframe", false);
    cull_mode_idx = renderer_reader.GetBoolean("renderer", "backface_culling", false) ? 1 : 0;

    /* --------------------------------------------- */
    // Subtask 1.2: Create a Window with GLFW
    /* --------------------------------------------- */
    if (!glfwInit()) {
        VKL_EXIT_WITH_ERROR("Failed to init GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No need to create a graphics context for Vulkan
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Use a monitor if we'd like to open the window in fullscreen mode:
    GLFWmonitor* monitor = nullptr;
    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
    }

    // TODO: Get a valid window handle and assign to window:
    GLFWwindow* window = glfwCreateWindow(width, height, window_title.c_str(), monitor, nullptr);

    if (!window) {
        VKL_LOG("If your program reaches this point, that means two things:");
        VKL_LOG("1) Project setup was successful. Everything is working fine.");
        VKL_LOG("2) You haven't implemented Subtask 1.2, which is creating a window with GLFW.");
        VKL_EXIT_WITH_ERROR("No GLFW window created.");
    }

    glfwSetKeyCallback(window, keyCallbackFromGlfw);
    // set callbacks for subtask 2.6
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    VKL_LOG("Subtask 1.2 done.");

    VkResult result;
    VkInstance vk_instance = VK_NULL_HANDLE;              // To be set during Subtask 1.3
    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;             // To be set during Subtask 1.4
    VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE; // To be set during Subtask 1.5
    VkDevice vk_device = VK_NULL_HANDLE;                  // To be set during Subtask 1.7
    VkQueue vk_queue = VK_NULL_HANDLE;                    // To be set during Subtask 1.7
    VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;         // To be set during Subtask 1.8

    /* --------------------------------------------- */
    // Subtask 1.3: Create a Vulkan Instance
    /* --------------------------------------------- */
    VkApplicationInfo application_info = {};                     // Zero-initialize every member
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // Set this struct instance's type
    application_info.pEngineName = "GCG_VK_Library";             // Set some properties...
    application_info.engineVersion = VK_MAKE_API_VERSION(0, 2023, 9, 1);
    application_info.pApplicationName = "GCG_VK_Solution";
    application_info.applicationVersion = VK_MAKE_API_VERSION(0, 2023, 9, 19);
    application_info.apiVersion = VK_API_VERSION_1_1;            // Your system needs to support this Vulkan API version.

    VkInstanceCreateInfo instance_create_info = {};                      // Zero-initialize every member
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // Set the struct's type
    instance_create_info.pApplicationInfo = &application_info;

    // TODO: Hook in required_extensions using VkInstanceCreateInfo::enabledExtensionCount and VkInstanceCreateInfo::ppEnabledExtensionNames!
    // TODO: Hook in enabled_layers using VkInstanceCreateInfo::enabledLayerCount and VkInstanceCreateInfo::ppEnabledLayerNames!
    // TODO: Use vkCreateInstance to create a vulkan instance handle! Assign it to vk_instance!
    uint32_t glfw_extension_count = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    uint32_t framework_extension_count = 0;
    const char** frameworkExtensions = vklGetRequiredInstanceExtensions(&framework_extension_count);

    std::vector<const char*> required_extensions(glfwExtensions, glfwExtensions + glfw_extension_count);
    required_extensions.insert(required_extensions.end(), frameworkExtensions, frameworkExtensions + framework_extension_count);

    std::vector<const char*> enabled_layers = {"VK_LAYER_KHRONOS_validation"};

#ifdef __APPLE__
    required_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    instance_create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
    instance_create_info.ppEnabledExtensionNames = required_extensions.data();
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(enabled_layers.size());;
    instance_create_info.ppEnabledLayerNames = enabled_layers.data();

    uint32_t available_extensions_count = 0;
    result = vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, nullptr);
    VKL_CHECK_VULKAN_RESULT(result);

    std::vector<VkExtensionProperties> available_extensions(available_extensions_count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, available_extensions.data());
    VKL_CHECK_VULKAN_RESULT(result);

    uint32_t available_layers_count = 0;
    result = vkEnumerateInstanceLayerProperties(&available_layers_count, nullptr);
    VKL_CHECK_VULKAN_RESULT(result);

    std::vector<VkLayerProperties> available_layers(available_layers_count);
    result = vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers.data());
    VKL_CHECK_VULKAN_RESULT(result);

    result = vkCreateInstance(&instance_create_info, nullptr, &vk_instance);
    VKL_CHECK_VULKAN_RESULT(result);

    if (!vk_instance) {
        VKL_EXIT_WITH_ERROR("No VkInstance created or handle not assigned.");
    }
    VKL_LOG("Subtask 1.3 done.");

    /* --------------------------------------------- */
    // Subtask 1.4: Create a Vulkan Window Surface
    /* --------------------------------------------- */
    // TODO: Use glfwCreateWindowSurface to create a window surface! Assign its handle to vk_surface!
    result = glfwCreateWindowSurface(vk_instance, window, nullptr, &vk_surface);
    VKL_CHECK_VULKAN_RESULT(result);
    if (!vk_surface) {
        VKL_EXIT_WITH_ERROR("No VkSurfaceKHR created or handle not assigned.");
    }
    VKL_LOG("Subtask 1.4 done.");

    /* --------------------------------------------- */
    // Subtask 1.5: Pick a Physical Device
    /* --------------------------------------------- */
    // TODO: Use vkEnumeratePhysicalDevices get all the available physical device handles!
    //       Select one that is suitable using the helper function selectPhysicalDeviceIndex and assign it to vk_physical_device!
    uint32_t physical_device_count = 0;
    vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr);
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    result = vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices.data());
    VKL_CHECK_VULKAN_RESULT(result);

    uint32_t physical_device_index = selectPhysicalDeviceIndex(physical_devices, vk_surface);
    vk_physical_device = physical_devices[physical_device_index];

    if (!vk_physical_device) {
        VKL_EXIT_WITH_ERROR("No VkPhysicalDevice selected or handle not assigned.");
    }
    VKL_LOG("Subtask 1.5 done.");

    // Subtask 3.1: Wireframe Mode
    VkPhysicalDeviceFeatures device_features = {};
    vkGetPhysicalDeviceFeatures(vk_physical_device, &device_features);
    if (device_features.fillModeNonSolid == VK_FALSE) {
        VKL_EXIT_WITH_ERROR("Device does not support fillModeNonSolidity.");
    }

    /* --------------------------------------------- */
    // Subtask 1.6: Select a Queue Family
    /* --------------------------------------------- */
    // TODO: Find a suitable queue family and assign its index to the following variable:
    //       Hint: Use the function selectQueueFamilyIndex, but complete its implementation first!
    uint32_t selected_queue_family_index = selectQueueFamilyIndex(vk_physical_device, vk_surface);

    // Sanity check if we have selected a valid queue family index:
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, nullptr);
    if (selected_queue_family_index >= queue_family_count) {
        VKL_EXIT_WITH_ERROR("Invalid queue family index selected.");
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = selected_queue_family_index;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &priority;

    VKL_LOG("Subtask 1.6 done.");

    /* --------------------------------------------- */
    // Subtask 1.7: Create a Logical Device and Get Queue
    /* --------------------------------------------- */
    // TODOs: - Create an instance of VkDeviceCreateInfo and use it to create one queue!
    //        - Hook in the address of the VkDeviceQueueCreateInfo instance at the right place!
    //        - Use VkDeviceCreateInfo::enabledExtensionCount and VkDeviceCreateInfo::ppEnabledExtensionNames
    //          to enable the VK_KHR_SWAPCHAIN_EXTENSION_NAME device extension!
    //        - Ensure that the other settings (which are unused in our case) are zero-initialized!
    //        - Finally, use vkCreateDevice to create the device and assign its handle to vk_device!

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.queueCreateInfoCount = 1;

    std::vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef __APPLE__
    device_extensions.push_back("VK_KHR_portability_subset");
#endif

    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();
    device_create_info.pEnabledFeatures = &device_features; // enable physical device features, subtask 3.1: Wireframe Mode

    result = vkCreateDevice(vk_physical_device, &device_create_info, nullptr, &vk_device);
    VKL_CHECK_VULKAN_RESULT(result);

    if (!vk_device) {
        VKL_EXIT_WITH_ERROR("No VkDevice created or handle not assigned.");
    }

    // TODO: After device creation, use vkGetDeviceQueue to get the one and only created queue!
    //       Assign its handle to vk_queue!
    vkGetDeviceQueue(vk_device, selected_queue_family_index, 0, &vk_queue);

    if (!vk_queue) {
        VKL_EXIT_WITH_ERROR("No VkQueue selected or handle not assigned.");
    }
    VKL_LOG("Subtask 1.7 done.");

    /* --------------------------------------------- */
    // Subtask 1.8: Create a Swapchain
    /* --------------------------------------------- */
    uint32_t queueFamilyIndexCount = 1u;
    std::vector<uint32_t> queueFamilyIndices = {selected_queue_family_index};
    VkSurfaceCapabilitiesKHR surface_capabilities = getPhysicalDeviceSurfaceCapabilities(vk_physical_device, vk_surface);
    // Build the swapchain config struct:
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vk_surface;
    swapchain_create_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_create_info.imageArrayLayers = 1u;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    } else {
        std::cout << "Warning: Automatic Testing might fail, VK_IMAGE_USAGE_TRANSFER_SRC_BIT image usage is not supported" << std::endl;
    }
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = queueFamilyIndexCount;
    swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices.data();
    // TODO: Provide values for:
    //        - VkSwapchainCreateInfoKHR::queueFamilyIndexCount
    //        - VkSwapchainCreateInfoKHR::pQueueFamilyIndices
    //        - VkSwapchainCreateInfoKHR::imageFormat
    //        - VkSwapchainCreateInfoKHR::imageColorSpace
    //        - VkSwapchainCreateInfoKHR::imageExtent
    //        - VkSwapchainCreateInfoKHR::presentMode
    //
    VkSurfaceFormatKHR surface_format = getSurfaceImageFormat(vk_physical_device, vk_surface);
    VkExtent2D extent = surface_capabilities.currentExtent;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = extent; // to match the swapchain size with the surface size;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    // TODO: Create the swapchain using vkCreateSwapchainKHR and assign its handle to vk_swapchain!
    result = vkCreateSwapchainKHR(vk_device, &swapchain_create_info, nullptr, &vk_swapchain);
    VKL_CHECK_VULKAN_RESULT(result);

    if (!vk_swapchain) {
        VKL_EXIT_WITH_ERROR("No VkSwapchainKHR created or handle not assigned.");
    }

    // TODO: Use vkGetSwapchainImagesKHR to retrieve the created VkImage handles and store them in a collection (e.g., std::vector)!
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &swapchain_image_count, nullptr);
    std::vector<VkImage> swapchain_images(swapchain_image_count);
    vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &swapchain_image_count, swapchain_images.data());

    VKL_LOG("Subtask 1.8 done.");

    /* --------------------------------------------- */
    // Subtask 1.9: Init GCG Framework
    /* --------------------------------------------- */

    // Gather swapchain config as required by the framework:
    VklSwapchainConfig swapchain_config = {};
    swapchain_config.swapchainHandle = vk_swapchain;
    swapchain_config.imageExtent = extent;
    swapchain_config.swapchainImages.resize(swapchain_images.size());

    for (uint32_t i = 0; i < swapchain_images.size(); i++) {
        swapchain_config.swapchainImages[i].colorAttachmentImageDetails.imageHandle = swapchain_images[i];
        swapchain_config.swapchainImages[i].colorAttachmentImageDetails.imageFormat = surface_format.format;
        swapchain_config.swapchainImages[i].colorAttachmentImageDetails.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_config.swapchainImages[i].colorAttachmentImageDetails.clearValue.color = {{0.14f, 0.4f, 0.37f, 1.0f}};
        swapchain_config.swapchainImages[i].depthAttachmentImageDetails.imageHandle = VK_NULL_HANDLE;
    }

    // Subtask 2.7: Depth Test
    uint32_t swapchain_width = extent.width;
    uint32_t swapchain_height = extent.height;
    VkImage depth_image = vklCreateDeviceLocalImageWithBackingMemory(
        vk_physical_device,
        vk_device,
        swapchain_width,
        swapchain_height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        1,
        0);

    VkClearValue depth_clear_value{};
    depth_clear_value.depthStencil.depth = 1.0f; // far plane distance in normalized device coordinates
    depth_clear_value.depthStencil.stencil = 0;

    for (uint32_t i = 0; i < swapchain_images.size(); i++) {
        swapchain_config.swapchainImages[i].depthAttachmentImageDetails.imageHandle = depth_image;
        swapchain_config.swapchainImages[i].depthAttachmentImageDetails.imageFormat = VK_FORMAT_D32_SFLOAT;
        swapchain_config.swapchainImages[i].depthAttachmentImageDetails.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        swapchain_config.swapchainImages[i].depthAttachmentImageDetails.clearValue = depth_clear_value;
    }
    //vklInitFramework(vk_instance, vk_surface, vk_physical_device, vk_device, vk_queue, swapchain_config);
    // end subtask 2.7

    // Init the framework:
    if (!gcgInitFramework(vk_instance, vk_surface, vk_physical_device, vk_device, vk_queue, swapchain_config)) {
        VKL_EXIT_WITH_ERROR("Failed to init framework");
    }
    VKL_LOG("Subtask 1.9 done.");

    // Subtask 2.1: Create a Custom Graphics Pipeline
    VklGraphicsPipelineConfig graphics_pipe_config{};
    graphics_pipe_config.vertexShaderPath = "shader.vert";
    graphics_pipe_config.fragmentShaderPath = "shader.frag";

    VkVertexInputBindingDescription vertexInputBindings{};
    vertexInputBindings.binding = 0;
    vertexInputBindings.stride = 3 * sizeof(float);
    vertexInputBindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    graphics_pipe_config.vertexInputBuffers.push_back(vertexInputBindings);

    VkVertexInputAttributeDescription vertex_input_attribute_description{};
    vertex_input_attribute_description.location = 0;
    vertex_input_attribute_description.binding = 0;
    vertex_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_description.offset = 0;
    graphics_pipe_config.inputAttributeDescriptions.push_back(vertex_input_attribute_description);

    graphics_pipe_config.polygonDrawMode = VK_POLYGON_MODE_FILL;
    graphics_pipe_config.triangleCullingMode = VK_CULL_MODE_NONE;

    std::string vertex_shader_path = gcgLoadShaderFilePath("assets/shaders/shader.vert");
    std::string fragment_shader_path = gcgLoadShaderFilePath("assets/shaders/shader.frag");

    graphics_pipe_config.vertexShaderPath = vertex_shader_path.c_str();
    graphics_pipe_config.fragmentShaderPath = fragment_shader_path.c_str();

    // Subtask 2.2: Create a Uniform Buffer
    VkDescriptorSetLayoutBinding descriptor_set_layout_binding{};
    descriptor_set_layout_binding.binding = 0;
    descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_layout_binding.descriptorCount = 1;
    descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_set_layout_binding.pImmutableSamplers = nullptr;
    graphics_pipe_config.descriptorLayout.push_back(descriptor_set_layout_binding);

    VkPipeline pipeline = vklCreateGraphicsPipeline(graphics_pipe_config);
    // Subtask 3.3: Interaction
    VkPipeline pipelines[4];
    for (uint32_t wire = 0; wire < 2; wire++) {
        for (uint32_t cull = 0; cull < 2; cull++) {
            VklGraphicsPipelineConfig pipe_config = graphics_pipe_config;
            pipe_config.polygonDrawMode = wire ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
            pipe_config.triangleCullingMode = (cull == 0) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
            pipelines[wire * 2 + cull] = vklCreateGraphicsPipeline(pipe_config);
        }
    }

    UniformBufferObject ubo{};
    ubo.color[0] = 1.0f;
    ubo.color[1] = 0.5f;
    ubo.color[2] = 0.0f;
    ubo.color[3] = 1.0f;

    VkDeviceSize buffer_size = sizeof(ubo);
    VkBuffer buffer = vklCreateHostCoherentBufferWithBackingMemory(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    vklCopyDataIntoHostCoherentBuffer(buffer, &ubo, buffer_size);
    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);

    // Subtask 2.3: Allocate and Write Descriptors
    VkDescriptorPool descriptor_pool;

    VkDescriptorPoolSize pool_size;
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = 8 * 16;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags = 0;
    descriptor_pool_create_info.maxSets = 8;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = &pool_size;

    VkResult des_pool_result = vkCreateDescriptorPool(vk_device, &descriptor_pool_create_info, nullptr, &descriptor_pool);
    VKL_CHECK_VULKAN_ERROR(des_pool_result);

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = nullptr;
    descriptor_set_layout_create_info.flags = 0;
    descriptor_set_layout_create_info.bindingCount = 1;
    descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;

    VkResult des_set_layout_result = vkCreateDescriptorSetLayout(vk_device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout);
    VKL_CHECK_VULKAN_ERROR(des_set_layout_result);

    VkDescriptorSet descriptor_set;
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;

    VkResult allocate_des_result = vkAllocateDescriptorSets(vk_device, &descriptor_set_allocate_info, &descriptor_set);
    VKL_CHECK_VULKAN_ERROR(allocate_des_result);

    VkDescriptorBufferInfo descriptor_buffer_info{};
    descriptor_buffer_info.buffer = buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = buffer_size;

    VkWriteDescriptorSet write_descriptor_set{};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = nullptr;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pImageInfo = nullptr;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk_device, 1, &write_descriptor_set, 0, nullptr);

    // Subtask 2.4: Viewing and Projection
    std::string init_camera_filepath = "assets/settings/camera_front.ini";
    if (cmdline_args.init_camera) {
        init_camera_filepath = cmdline_args.init_camera_filepath;
    }
    INIReader camera_reader(init_camera_filepath);
    float fov = glm::radians(static_cast<float>(camera_reader.GetReal("camera", "fov", 60.0f)));
    float near = static_cast<float>(camera_reader.GetReal("camera", "near", 0.1f));
    float far = static_cast<float>(camera_reader.GetReal("camera", "far", 100.0f));
    float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

    yaw = -static_cast<float>(camera_reader.GetReal("camera", "yaw", 0.0f));
    pitch = static_cast<float>(camera_reader.GetReal("camera", "pitch", 0.0f));
    if (pitch > max_pitch) pitch = max_pitch;
    if (pitch < min_pitch) pitch = min_pitch;
    zoom = 5.0f;

    glm::mat4 projection = gcgCreatePerspectiveProjectionMatrix(fov, aspect_ratio, near, far);

    glm::mat4 rotation_pitch = glm::rotate(glm::mat4(1.0f), pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotation_yaw = glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotation = rotation_yaw * rotation_pitch;

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));
    glm::mat4 camera_to_world = rotation * translation;
    // view-projection matrix
    glm::mat4 view = glm::inverse(camera_to_world);
    glm::mat4 view_projection = projection * view;

    // update the UBOs
    ubo.view_projection = view_projection;
    vklCopyDataIntoHostCoherentBuffer(buffer, &ubo, buffer_size);
    // Subtask 2.5: Multiple Teapots

    // teapot 1
    UniformBufferObject teapot1_ubo{};
    teapot1_ubo.color = glm::vec4(0.49f, 0.06f, 0.22f, 1.0f);

    glm::mat4 model_teapot1 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f));
    model_teapot1 = glm::rotate(model_teapot1, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // rotation, then translation
    teapot1_ubo.view_projection = view_projection * model_teapot1; // initialization

    VkDeviceSize buffer_size1 = sizeof(teapot1_ubo);
    VkBuffer buffer1 = vklCreateHostCoherentBufferWithBackingMemory(buffer_size1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(buffer1, &teapot1_ubo, buffer_size1);

    VkDescriptorSet descriptor_set1;
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info1{};
    descriptor_set_allocate_info1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info1.pNext = nullptr;
    descriptor_set_allocate_info1.descriptorPool = descriptor_pool;
    descriptor_set_allocate_info1.descriptorSetCount = 1;
    descriptor_set_allocate_info1.pSetLayouts = &descriptor_set_layout;

    VkResult allocate_des_result1 = vkAllocateDescriptorSets(vk_device, &descriptor_set_allocate_info1, &descriptor_set1);
    VKL_CHECK_VULKAN_ERROR(allocate_des_result1);

    VkDescriptorBufferInfo descriptor_buffer_info1{};
    descriptor_buffer_info1.buffer = buffer1;
    descriptor_buffer_info1.offset = 0;
    descriptor_buffer_info1.range = buffer_size1;

    VkWriteDescriptorSet write_descriptor_set1{};
    write_descriptor_set1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set1.pNext = nullptr;
    write_descriptor_set1.dstSet = descriptor_set1;
    write_descriptor_set1.dstBinding = 0;
    write_descriptor_set1.dstArrayElement = 0;
    write_descriptor_set1.descriptorCount = 1;
    write_descriptor_set1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set1.pImageInfo = nullptr;
    write_descriptor_set1.pBufferInfo = &descriptor_buffer_info1;
    write_descriptor_set1.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk_device, 1, &write_descriptor_set1, 0, nullptr);

    // teapot 2
    UniformBufferObject teapot2_ubo{};
    teapot2_ubo.color = glm::vec4(0.0f, 0.13f, 0.31f, 1.0f);

    glm::mat4 model_teapot2 = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 1.0f, 0.0f));
    model_teapot2 = glm::scale(model_teapot2, glm::vec3(1.0f, 2.0f, 1.0f)); // scal
    teapot2_ubo.view_projection = view_projection * model_teapot2; // initialization
    VkDeviceSize buffer_size2 = sizeof(teapot2_ubo);
    VkBuffer buffer2 = vklCreateHostCoherentBufferWithBackingMemory(buffer_size2, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(buffer2, &teapot2_ubo, buffer_size2);

    VkDescriptorSet descriptor_set2;
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info2{};
    descriptor_set_allocate_info2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info2.pNext = nullptr;
    descriptor_set_allocate_info2.descriptorPool = descriptor_pool;
    descriptor_set_allocate_info2.descriptorSetCount = 1;
    descriptor_set_allocate_info2.pSetLayouts = &descriptor_set_layout;
    VkResult allocate_des_result2 = vkAllocateDescriptorSets(vk_device, &descriptor_set_allocate_info2, &descriptor_set2);
    VKL_CHECK_VULKAN_ERROR(allocate_des_result2);

    VkDescriptorBufferInfo descriptor_buffer_info2{};
    descriptor_buffer_info2.buffer = buffer2;
    descriptor_buffer_info2.offset = 0;
    descriptor_buffer_info2.range = buffer_size2;

    VkWriteDescriptorSet write_descriptor_set2{};
    write_descriptor_set2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set2.pNext = nullptr;
    write_descriptor_set2.dstSet = descriptor_set2;
    write_descriptor_set2.dstBinding = 0;
    write_descriptor_set2.dstArrayElement = 0;
    write_descriptor_set2.descriptorCount = 1;
    write_descriptor_set2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set2.pImageInfo = nullptr;
    write_descriptor_set2.pBufferInfo = &descriptor_buffer_info2;
    write_descriptor_set2.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk_device, 1, &write_descriptor_set2, 0, nullptr);

    // Subtask 3.5: Cube Geometry (cube map)
    std::vector<float> cube_vertices;
    for (const auto& vertex: cube_vertices_temp) {
        cube_vertices.push_back(vertex.x);
        cube_vertices.push_back(vertex.y);
        cube_vertices.push_back(vertex.z);
    }
    VkDeviceSize cube_vertex_buffer_size = sizeof(float) * cube_vertices.size();
    VkBuffer cube_vertex_buffer = vklCreateHostCoherentBufferWithBackingMemory(cube_vertex_buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(cube_vertex_buffer, cube_vertices.data(), cube_vertex_buffer_size);

    VkDeviceSize cube_index_buffer_size = sizeof(uint32_t) * cube_indices.size();
    VkBuffer cube_index_buffer = vklCreateHostCoherentBufferWithBackingMemory(cube_index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(cube_index_buffer, cube_indices.data(), cube_index_buffer_size);

    // Subtask 3.6 - 3.7: Cornell Box
    std::vector<CornellVertex> cornell_vertices = cornell_vertices_temp;
    VkDeviceSize cornell_vertex_buffer_size = sizeof(CornellVertex) * cornell_vertices.size();
    VkBuffer cornell_vertex_buffer = vklCreateHostCoherentBufferWithBackingMemory(cornell_vertex_buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(cornell_vertex_buffer, cornell_vertices.data(), cornell_vertex_buffer_size);

    VkDeviceSize cornell_index_buffer_size = sizeof(uint32_t) * cornell_indices.size();
    VkBuffer cornell_index_buffer = vklCreateHostCoherentBufferWithBackingMemory(cornell_index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(cornell_index_buffer, cornell_indices.data(), cornell_index_buffer_size);

    VklGraphicsPipelineConfig cornell_pipe_config = graphics_pipe_config;
    VkVertexInputBindingDescription cornell_bindings = {};
    cornell_bindings.binding = 0;
    cornell_bindings.stride = sizeof(CornellVertex);
    cornell_bindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // clear any existing binding/attribute descriptions copied from graphics_pipe_config
    cornell_pipe_config.vertexInputBuffers.clear();
    cornell_pipe_config.inputAttributeDescriptions.clear();

    cornell_pipe_config.vertexInputBuffers.push_back(cornell_bindings);

    vertex_shader_path = gcgLoadShaderFilePath("assets/shaders/cornell.vert");
    fragment_shader_path = gcgLoadShaderFilePath("assets/shaders/cornell.frag");
    cornell_pipe_config.vertexShaderPath = vertex_shader_path.c_str();
    cornell_pipe_config.fragmentShaderPath = fragment_shader_path.c_str();

    // attribute: position
    VkVertexInputAttributeDescription cornell_position = {};
    cornell_position.location = 0;
    cornell_position.binding = 0;
    cornell_position.format = VK_FORMAT_R32G32B32_SFLOAT;
    cornell_position.offset = offsetof(CornellVertex, position);
    cornell_pipe_config.inputAttributeDescriptions.push_back(cornell_position);

    // attribute: color
    VkVertexInputAttributeDescription cornell_color = {};
    cornell_color.location = 1;
    cornell_color.binding = 0;
    cornell_color.format = VK_FORMAT_R32G32B32_SFLOAT;
    cornell_color.offset = offsetof(CornellVertex, color);
    cornell_pipe_config.inputAttributeDescriptions.push_back(cornell_color);

    VkPipeline cornell_pipelines[4];
    for (uint32_t wire = 0; wire < 2; wire++) {
        for (uint32_t cull = 0; cull < 2; cull++) {
            cornell_pipe_config.polygonDrawMode = wire ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
            cornell_pipe_config.triangleCullingMode = (cull == 0) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
            cornell_pipelines[wire * 2 + cull] = vklCreateGraphicsPipeline(cornell_pipe_config);
        }
    }

    // Subtask 3.8: Test Scene
    std::vector<cubeVertex> cubeVertices = cubeVertices_temp;
    VkDeviceSize cubeVertexBuffer_size = sizeof(cubeVertex) * cubeVertices.size();
    VkBuffer cubeVertex_buffer = vklCreateHostCoherentBufferWithBackingMemory(cubeVertexBuffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(cubeVertex_buffer, cubeVertices.data(), cubeVertexBuffer_size);

    VkDeviceSize cubeIndexBuffer_size = sizeof(uint32_t) * cube_indices.size();
    VkBuffer cubeIndexBuffer = vklCreateHostCoherentBufferWithBackingMemory(cubeIndexBuffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(cubeIndexBuffer, cube_indices.data(), cubeIndexBuffer_size);

    VklGraphicsPipelineConfig cube_pipe_config = graphics_pipe_config;
    VkVertexInputBindingDescription cube_bindings = {};
    cube_bindings.binding = 0;
    cube_bindings.stride = sizeof(cubeVertex);
    cube_bindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    cube_pipe_config.vertexInputBuffers.clear();
    cube_pipe_config.inputAttributeDescriptions.clear();

    cube_pipe_config.vertexInputBuffers.push_back(cube_bindings);

    vertex_shader_path = gcgLoadShaderFilePath("assets/shaders/shader.vert");
    fragment_shader_path = gcgLoadShaderFilePath("assets/shaders/shader.frag");
    cube_pipe_config.vertexShaderPath = vertex_shader_path.c_str();
    cube_pipe_config.fragmentShaderPath = fragment_shader_path.c_str();

    // attribute: position
    VkVertexInputAttributeDescription cube_position = {};
    cube_position.location = 0;
    cube_position.binding = 0;
    cube_position.format = VK_FORMAT_R32G32B32_SFLOAT;
    cube_position.offset = offsetof(cubeVertex, position);
    cube_pipe_config.inputAttributeDescriptions.push_back(cube_position);

    VkPipeline cube_pipelines[4];
    for (uint32_t wire = 0; wire < 2; wire++) {
        for (uint32_t cull = 0; cull < 2; cull++) {
            cube_pipe_config.polygonDrawMode = wire ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
            cube_pipe_config.triangleCullingMode = (cull == 0) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
            cube_pipelines[wire * 2 + cull] = vklCreateGraphicsPipeline(cube_pipe_config);
        }
    }

    UniformBufferObject cube_ubo{};
    //cube_ubo.color = glm::vec4(0.75f, 0.25f, 0.01f, 1.0f);

    glm::mat4 model_cube = glm::mat4(1.0f);
    //model_cube = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    // Subtask 4.4: Test Scene (rotation + translation)
    cube_ubo.color = glm::vec4(0.0f, 0.21f, 0.16f, 1.0f);
    model_cube = glm::translate(model_cube, glm::vec3(-0.6f, -0.9f, 0.0f));
    model_cube = glm::rotate(model_cube, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    cube_ubo.view_projection = view_projection * model_cube;

    VkDeviceSize cube_buffer_size = sizeof(cube_ubo);
    VkBuffer cube_buffer = vklCreateHostCoherentBufferWithBackingMemory(cube_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(cube_buffer, &cube_ubo, cube_buffer_size);
    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);

    VkDescriptorSet descriptor_set_cube;
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info_cube{};
    descriptor_set_allocate_info_cube.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info_cube.pNext = nullptr;
    descriptor_set_allocate_info_cube.descriptorPool = descriptor_pool;
    descriptor_set_allocate_info_cube.descriptorSetCount = 1;
    descriptor_set_allocate_info_cube.pSetLayouts = &descriptor_set_layout;

    VkResult allocate_des_result_cube = vkAllocateDescriptorSets(vk_device, &descriptor_set_allocate_info_cube, &descriptor_set_cube);
    VKL_CHECK_VULKAN_ERROR(allocate_des_result_cube);

    VkDescriptorBufferInfo descriptor_buffer_info_cube{};
    descriptor_buffer_info_cube.buffer = cube_buffer;
    descriptor_buffer_info_cube.offset = 0;
    descriptor_buffer_info_cube.range = cube_buffer_size;

    VkWriteDescriptorSet write_descriptor_set_cube{};
    write_descriptor_set_cube.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set_cube.pNext = nullptr;
    write_descriptor_set_cube.dstSet = descriptor_set_cube;
    write_descriptor_set_cube.dstBinding = 0;
    write_descriptor_set_cube.dstArrayElement = 0;
    write_descriptor_set_cube.descriptorCount = 1;
    write_descriptor_set_cube.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set_cube.pImageInfo = nullptr;
    write_descriptor_set_cube.pBufferInfo = &descriptor_buffer_info_cube;
    write_descriptor_set_cube.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk_device, 1, &write_descriptor_set_cube, 0, nullptr);

    // Subtask 4.1: Cylinder Geometry
    std::vector<glm::vec3> cylinderVertices;
    std::vector<uint32_t> cylinderIndices;
    buildCylinder(1.6f, 0.21f, 20, cylinderVertices, cylinderIndices);

    VkDescriptorSet descriptor_set_cylinder{};
    VkDescriptorSetAllocateInfo alloc_info_cylinder{};
    alloc_info_cylinder.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info_cylinder.descriptorPool = descriptor_pool;
    alloc_info_cylinder.descriptorSetCount = 1;
    alloc_info_cylinder.pSetLayouts = &descriptor_set_layout;
    VkResult res = vkAllocateDescriptorSets(vk_device, &alloc_info_cylinder, &descriptor_set_cylinder);
    VKL_CHECK_VULKAN_ERROR(res);

    MeshResources cylinder = createMesh(cylinderVertices, cylinderIndices, view_projection, window, descriptor_set_cylinder, vk_device);

    cylinder.ubo.color = glm::vec4(0.75f, 0.25f, 0.01f, 1.0f);

    // Subtask 4.4: Test Scene
    glm::mat4 model_cylinder = glm::mat4(1.0f);
    model_cylinder = glm::translate(model_cylinder, glm::vec3(0.6f, 0.3f, 0.0f));
    cylinder.ubo.view_projection = view_projection * model_cylinder;

    // Subtask 4.2: Sphere Geometry
    std::vector<glm::vec3> sphereVertices;
    std::vector<uint32_t> sphereIndices;
    buildSphere(36, 18, 0.26, sphereVertices, sphereIndices);

    VkDescriptorSet descriptor_set_sphere{};
    VkDescriptorSetAllocateInfo alloc_info_sphere{};
    alloc_info_sphere.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info_sphere.descriptorPool = descriptor_pool;
    alloc_info_sphere.descriptorSetCount = 1;
    alloc_info_sphere.pSetLayouts = &descriptor_set_layout;
    res = vkAllocateDescriptorSets(vk_device, &alloc_info_sphere, &descriptor_set_sphere);
    VKL_CHECK_VULKAN_ERROR(res);

    MeshResources sphere = createMesh(sphereVertices, sphereIndices, view_projection, window, descriptor_set_sphere, vk_device);

    sphere.ubo.color = glm::vec4(0.12f, 0.12f, 0.12f, 1.0f);
    // Subtask 4.4: Test Scene
    glm::mat4 model_sphere = glm::mat4(1.0f);
    model_sphere = glm::translate(model_sphere, glm::vec3(0.6f, -0.9f, 0.0f));
    sphere.ubo.view_projection = view_projection * model_sphere;

    // Subtask 4.3: Bézier Cylinder Geometry
    std::vector<glm::vec3> bezierCylinderVertices;
    std::vector<uint32_t> bezierCylinderIndices;
    std::vector<glm::vec3> controlPoints = {
        {-0.3f, 0.6f, 0.0f},
        {0.0f, 1.6f, 0.0f},
        {1.4f, 0.3f, 0.0f},
        {0.0f, 0.3f, 0.0f},
        {0.0f, -0.5f, 0.0f}
    };
    buildBezierCylinder(36, 20, 0.21, controlPoints,bezierCylinderVertices, bezierCylinderIndices);

    VkDescriptorSet descriptor_set_bezierCylinder{};
    VkDescriptorSetAllocateInfo alloc_info_bezierCylinder{};
    alloc_info_bezierCylinder.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info_bezierCylinder.descriptorPool = descriptor_pool;
    alloc_info_bezierCylinder.descriptorSetCount = 1;
    alloc_info_bezierCylinder.pSetLayouts = &descriptor_set_layout;
    res = vkAllocateDescriptorSets(vk_device, &alloc_info_bezierCylinder, &descriptor_set_bezierCylinder);
    VKL_CHECK_VULKAN_ERROR(res);

    MeshResources bezierCylinder = createMesh(bezierCylinderVertices, bezierCylinderIndices, view_projection, window, descriptor_set_bezierCylinder, vk_device);

    bezierCylinder.ubo.color = glm::vec4(0.75f, 0.25f, 0.01f, 1.0f);
    // Subtask 4.4: Test Scene
    glm::mat4 model_bezierCylinder = glm::mat4(1.0f);
    model_bezierCylinder = glm::translate(model_bezierCylinder, glm::vec3(-0.6f, 0.0f, 0.0f));
    bezierCylinder.ubo.view_projection = view_projection * model_bezierCylinder;

    /* --------------------------------------------- */
    // Subtask 1.10: Set-up the Render Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // Subtask 2.6: Orbit Camera
        camera_pos.x = zoom * cos(pitch) * sin(yaw);
        camera_pos.y = zoom * sin(pitch);
        camera_pos.z = zoom * cos(pitch) * cos(yaw);

        bool strafing = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        if (strafing) {
            camera_pos += target;
        }

        glm::vec3 direction = glm::normalize(target - camera_pos);
        glm::vec3 right = glm::normalize(glm::cross(direction, world_up));
        glm::vec3 camera_up = glm::cross(right, direction);

        view = glm::mat4(1.0f);
        view[0] = glm::vec4(right, 0.0f);
        view[1] = glm::vec4(camera_up, 0.0f);
        view[2] = glm::vec4(-direction, 0.0f);
        view[3] = glm::vec4(camera_pos, 1.0f);
        view = glm::inverse(view);
        view_projection = projection * view;

        ubo.view_projection = view_projection;
        vklCopyDataIntoHostCoherentBuffer(buffer, &ubo, buffer_size);

        teapot1_ubo.view_projection = view_projection * model_teapot1;
        vklCopyDataIntoHostCoherentBuffer(buffer1, &teapot1_ubo, buffer_size1);

        teapot2_ubo.view_projection = view_projection * model_teapot2;
        vklCopyDataIntoHostCoherentBuffer(buffer2, &teapot2_ubo, buffer_size2);

        vklWaitForNextSwapchainImage();
        vklStartRecordingCommands();
        //gcgDrawTeapot(pipeline, descriptor_set);
        //gcgDrawTeapot(pipeline, descriptor_set1);
        //gcgDrawTeapot(pipeline, descriptor_set2);
        VkPipeline curr_pipeline = pipelines[is_wireframe * 2 + cull_mode_idx];
        //gcgDrawTeapot(curr_pipeline, descriptor_set1); // subtask 3.3: Interaction
        //gcgDrawTeapot(curr_pipeline, descriptor_set2); // subtask 3.3: Interaction

        // Subtask 3.4: Command Buffer Recording
        VkCommandBuffer cmdBuffer = vklGetCurrentCommandBuffer();
        VkPipelineLayout pipeline_layout = vklGetLayoutForPipeline(curr_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curr_pipeline);

        VkBuffer vertex_buffer = gcgGetTeapotPositionsBuffer();
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertex_buffer, offsets);

        VkBuffer index_buffer = gcgGetTeapotIndicesBuffer();
        vkCmdBindIndexBuffer(cmdBuffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

        // draw teapot 1
        //vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set1, 0, nullptr);
        //vkCmdDrawIndexed(cmdBuffer, gcgGetNumTeapotIndices(), 1, 0, 0, 0);

        // draw teapot 2
        //vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set2, 0, nullptr);
        //vkCmdDrawIndexed(cmdBuffer, gcgGetNumTeapotIndices(), 1, 0, 0, 0);

        // Subtask 3.5: Cube Geometry (cube map)
        //vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &cube_vertex_buffer, offsets);
        //vkCmdBindIndexBuffer(cmdBuffer, cube_index_buffer, 0, VK_INDEX_TYPE_UINT32);
        //vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
        //vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(cube_indices.size()), 1, 0, 0, 0);

        // Subtask 3.6 - 3.7: Cornell Box
        VkPipeline curr_cornell_pipeline = cornell_pipelines[is_wireframe * 2 + cull_mode_idx];
        VkPipelineLayout cornell_pipeline_layout = vklGetLayoutForPipeline(curr_cornell_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curr_cornell_pipeline);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &cornell_vertex_buffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, cornell_index_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cornell_pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(cornell_indices.size()), 1, 0, 0, 0);

        // Subtask 3.8: Test Scene
        cube_ubo.view_projection =  view_projection * model_cube;
        vklCopyDataIntoHostCoherentBuffer(cube_buffer, &cube_ubo, cube_buffer_size);

        VkPipeline curr_cube_pipeline = cube_pipelines[is_wireframe * 2 + cull_mode_idx];
        VkPipelineLayout cube_pipeline_layout = vklGetLayoutForPipeline(curr_cube_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curr_cube_pipeline);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &cubeVertex_buffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, cubeIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cube_pipeline_layout, 0, 1, &descriptor_set_cube, 0, nullptr);
        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(cube_indices.size()), 1, 0, 0, 0);

        // Subtask 4.1: Cylinder Geometry
        cylinder.ubo.view_projection = view_projection * model_cylinder;
        vklCopyDataIntoHostCoherentBuffer(cylinder.uniformBuffer, &cylinder.ubo, cylinder.uniformBufferSize);

        VkPipeline curr_cylinder_pipeline = cylinder.pipelines[is_wireframe * 2 + cull_mode_idx];
        VkPipelineLayout cylinder_pipeline_layout = vklGetLayoutForPipeline(curr_cylinder_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curr_cylinder_pipeline);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &cylinder.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, cylinder.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cylinder_pipeline_layout, 0, 1, &descriptor_set_cylinder, 0, nullptr);
        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(cylinderIndices.size()), 1, 0, 0, 0);

        // Subtask 4.2: Sphere Geometry

        sphere.ubo.view_projection = view_projection * model_sphere;
        vklCopyDataIntoHostCoherentBuffer(sphere.uniformBuffer, &sphere.ubo, sphere.uniformBufferSize);

        VkPipeline curr_sphere_pipeline = sphere.pipelines[is_wireframe * 2 + cull_mode_idx];
        VkPipelineLayout sphere_pipeline_layout = vklGetLayoutForPipeline(curr_sphere_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curr_sphere_pipeline);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &sphere.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, sphere.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sphere_pipeline_layout, 0, 1, &descriptor_set_sphere, 0, nullptr);
        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(sphereIndices.size()), 1, 0, 0, 0);


        // Subtask 4.3: Bézier Cylinder

        bezierCylinder.ubo.view_projection = view_projection * model_bezierCylinder;
        vklCopyDataIntoHostCoherentBuffer(bezierCylinder.uniformBuffer, &bezierCylinder.ubo, bezierCylinder.uniformBufferSize);

        VkPipeline curr_bezierCylinder_pipeline = bezierCylinder.pipelines[is_wireframe * 2 + cull_mode_idx];
        VkPipelineLayout bezierCylinder_pipeline_layout = vklGetLayoutForPipeline(curr_bezierCylinder_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curr_bezierCylinder_pipeline);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &bezierCylinder.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, bezierCylinder.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bezierCylinder_pipeline_layout, 0, 1, &descriptor_set_bezierCylinder, 0, nullptr);
        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(bezierCylinderIndices.size()), 1, 0, 0, 0);

        vklEndRecordingCommands();
        vklPresentCurrentSwapchainImage();

        if (cmdline_args.run_headless) {
            uint32_t idx = vklGetCurrentSwapChainImageIndex();
            std::string screenshot_filename = "screenshot";
            if (cmdline_args.set_filename) {
                screenshot_filename = cmdline_args.filename;
            }
            gcgSaveScreenshot(screenshot_filename, swapchain_images[idx],
        width, height, surface_format.format, vk_device, vk_physical_device,
        vk_queue, selected_queue_family_index);
            break; }
    }
    // Subtask 1.11: Register a Key Callback
    /* --------------------------------------------- */

    // Wait for all GPU work to finish before cleaning up:
    vkDeviceWaitIdle(vk_device);

    /* --------------------------------------------- */
    // Subtask 1.12: Cleanup
    /* --------------------------------------------- */
    vklDestroyGraphicsPipeline(pipeline);
    for (uint32_t i = 0; i < 4; i++) {
        vklDestroyGraphicsPipeline(pipelines[i]);
        vklDestroyGraphicsPipeline(cornell_pipelines[i]);
        vklDestroyGraphicsPipeline(cube_pipelines[i]);
        vklDestroyGraphicsPipeline(cylinder.pipelines[i]);
        vklDestroyGraphicsPipeline(sphere.pipelines[i]);
        vklDestroyGraphicsPipeline(bezierCylinder.pipelines[i]);
    }
    vklDestroyHostCoherentBufferAndItsBackingMemory(buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(buffer1);
    vklDestroyHostCoherentBufferAndItsBackingMemory(buffer2);

    vklDestroyHostCoherentBufferAndItsBackingMemory(cube_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(cube_vertex_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(cube_index_buffer);

    vklDestroyHostCoherentBufferAndItsBackingMemory(cornell_vertex_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(cornell_index_buffer);

    vklDestroyHostCoherentBufferAndItsBackingMemory(cubeVertex_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(cubeIndexBuffer);

    vklDestroyHostCoherentBufferAndItsBackingMemory(cylinder.uniformBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(cylinder.vertexBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(cylinder.indexBuffer);

    vklDestroyHostCoherentBufferAndItsBackingMemory(sphere.uniformBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(sphere.vertexBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(sphere.indexBuffer);

    vklDestroyHostCoherentBufferAndItsBackingMemory(bezierCylinder.uniformBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(bezierCylinder.vertexBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(bezierCylinder.indexBuffer);

    vkDestroyDescriptorSetLayout(vk_device, descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(vk_device, descriptor_pool, nullptr);

    vklDestroyDeviceLocalImageAndItsBackingMemory(depth_image);

    gcgDestroyFramework();

    vkDestroySwapchainKHR(vk_device, vk_swapchain, nullptr);

    vkDestroyDevice(vk_device, nullptr);

    vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
    vkDestroyInstance(vk_instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

/* --------------------------------------------- */
// Helper Function Definitions
/* --------------------------------------------- */


uint32_t selectPhysicalDeviceIndex(const VkPhysicalDevice* physical_devices, uint32_t physical_device_count, VkSurfaceKHR surface) {
    // Iterate over all the physical devices and select one that satisfies all our requirements.
    // Our requirements are:
    //  - Must support a queue that must have both, graphics and presentation capabilities
    for (uint32_t physical_device_index = 0u; physical_device_index < physical_device_count; ++physical_device_index) {
        // Get the number of different queue families:
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[physical_device_index], &queue_family_count, nullptr);

        // Get the queue families' data:
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[physical_device_index], &queue_family_count, queue_families.data());

        for (uint32_t queue_family_index = 0u; queue_family_index < queue_family_count; ++queue_family_index) {
            // If this physical device supports a queue family which supports both, graphics and presentation
            //  => select this physical device
            if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                // This queue supports graphics! Let's see if it also supports presentation:
                VkBool32 presentation_supported;
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[physical_device_index], queue_family_index, surface, &presentation_supported);

                if (VK_TRUE == presentation_supported) {
                    // We've found a suitable physical device
                    return physical_device_index;
                }
            }
        }
    }
    VKL_EXIT_WITH_ERROR("Unable to find a suitable physical device that supports graphics and presentation on the same queue.");
}

uint32_t selectPhysicalDeviceIndex(const std::vector<VkPhysicalDevice>& physical_devices, VkSurfaceKHR surface) {
    return selectPhysicalDeviceIndex(physical_devices.data(), static_cast<uint32_t>(physical_devices.size()), surface);
}

uint32_t selectQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    // Get the number of different queue families for the given physical device:
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    // Get the queue families' data:
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            VkBool32 supported_graphics;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supported_graphics);
            if (VK_TRUE == supported_graphics) {
                // We've found a suitable physical device
                return i;
            }
        }
    }
    VKL_EXIT_WITH_ERROR("Unable to find a suitable queue family that supports graphics and presentation on the same queue.");
}

VkSurfaceFormatKHR getSurfaceImageFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VkResult result;

    uint32_t surface_format_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);
    VKL_CHECK_VULKAN_ERROR(result);

    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats.data());
    VKL_CHECK_VULKAN_ERROR(result);

    if (surface_formats.empty()) {
        VKL_EXIT_WITH_ERROR("Unable to find supported surface formats.");
    }

    // Prefer a RGB8/sRGB format; If we are unable to find such, just return any:
    for (const VkSurfaceFormatKHR& f : surface_formats) {
        if ((f.format == VK_FORMAT_B8G8R8A8_SRGB || f.format == VK_FORMAT_R8G8B8A8_SRGB) && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }

    return surface_formats[0];
}

VkSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    VKL_CHECK_VULKAN_ERROR(result);
    return surface_capabilities;
}

VkSurfaceTransformFlagBitsKHR getSurfaceTransform(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    return getPhysicalDeviceSurfaceCapabilities(physical_device, surface).currentTransform;
}

// Subtask 2.6: Orbit Camera
void mouse_callback(GLFWwindow* window, double x_pos, double y_pos) {

    if (first_mouse) {
        last_x = x_pos;
        last_y = y_pos;
        first_mouse = false;
    }
    float x_offset = x_pos - last_x;
    float y_offset = last_y - y_pos;
    last_x = x_pos;
    last_y = y_pos;

    float sensitivity = 0.3f;
    x_offset *= sensitivity;
    y_offset *= sensitivity;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        yaw += glm::radians(x_offset);
        pitch += glm::radians(y_offset);

        if (pitch > max_pitch) pitch = max_pitch;
        if (pitch < min_pitch) pitch = min_pitch;
    }

    // Subtask 2.8: Strafe
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        float strafe_speed = 0.05f;
        glm::vec3 direction = glm::normalize(target - camera_pos);
        glm::vec3 right = glm::normalize(glm::cross(direction, world_up));
        glm::vec3 camera_up = glm::cross(right, direction);
        glm::vec3 strafe = (-right * x_offset * strafe_speed) + (-camera_up * y_offset * strafe_speed);
        target += strafe;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS &&
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) {
        first_mouse = true;
        }
}

void scroll_callback(GLFWwindow* window, double x_offset, double y_offset) {
    zoom -= static_cast<float>(y_offset);
    if (zoom < 1.0f) zoom = 1.0f;
    if (zoom > 45.0f) zoom = 45.0f;
}


// Subtask 4.1: Cylinder Geometry
MeshResources createMesh(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices, glm::mat4 view_projection, GLFWwindow* window, VkDescriptorSet descriptor_set, VkDevice vk_device) {
    if (descriptor_set == VK_NULL_HANDLE) {
        throw std::runtime_error("Unable to create mesh descriptor set");
    }

    MeshResources mesh;

    VkDeviceSize VertexBuffer_size = sizeof(glm::vec3) * vertices.size();
    mesh.vertexBuffer = vklCreateHostCoherentBufferWithBackingMemory(VertexBuffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(mesh.vertexBuffer, vertices.data(), VertexBuffer_size);

    VkDeviceSize IndexBuffer_size = sizeof(uint32_t) * indices.size();
    mesh.indexBuffer = vklCreateHostCoherentBufferWithBackingMemory(IndexBuffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(mesh.indexBuffer, indices.data(), IndexBuffer_size);

    VklGraphicsPipelineConfig pipe_config{};
    VkVertexInputBindingDescription bindings = {};
    bindings.binding = 0;
    bindings.stride = sizeof(glm::vec3);
    bindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    pipe_config.vertexInputBuffers.clear();
    pipe_config.inputAttributeDescriptions.clear();
    pipe_config.vertexInputBuffers.push_back(bindings);

    std::string vertex_shader_path = gcgLoadShaderFilePath("assets/shaders/shader.vert");
    std::string fragment_shader_path = gcgLoadShaderFilePath("assets/shaders/shader.frag");
    pipe_config.vertexShaderPath = vertex_shader_path.c_str();
    pipe_config.fragmentShaderPath = fragment_shader_path.c_str();

    // attribute: position
    VkVertexInputAttributeDescription position = {};
    position.location = 0;
    position.binding = 0;
    position.format = VK_FORMAT_R32G32B32_SFLOAT;
    position.offset = 0;
    pipe_config.inputAttributeDescriptions.push_back(position);

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding{};
    descriptor_set_layout_binding.binding = 0;
    descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_layout_binding.descriptorCount = 1;
    descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_set_layout_binding.pImmutableSamplers = nullptr;
    pipe_config.descriptorLayout.push_back(descriptor_set_layout_binding);

    for (uint32_t wire = 0; wire < 2; wire++) {
        for (uint32_t cull = 0; cull < 2; cull++) {
            pipe_config.polygonDrawMode = wire ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
            pipe_config.triangleCullingMode = (cull == 0) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
            mesh.pipelines[wire * 2 + cull] = vklCreateGraphicsPipeline(pipe_config);
        }
    }

    mesh.ubo.view_projection = view_projection;

    mesh.uniformBufferSize = sizeof(mesh.ubo);
    mesh.uniformBuffer = vklCreateHostCoherentBufferWithBackingMemory(mesh.uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(mesh.uniformBuffer, &mesh.ubo, mesh.uniformBufferSize);
    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);

    VkDescriptorBufferInfo descriptor_buffer_info{};
    descriptor_buffer_info.buffer = mesh.uniformBuffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = mesh.uniformBufferSize;

    VkWriteDescriptorSet write_descriptor_set{};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = nullptr;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pImageInfo = nullptr;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk_device, 1, &write_descriptor_set, 0, nullptr);

    return mesh;
}

void buildCylinder(float h, float r, uint32_t n, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices) {
    // top face
    vertices.push_back({glm::vec3(0.0f, h * 0.5f, 0.0f)});
    for (uint32_t i = 0; i < n; ++i) {
        float angle = float(i) / n * glm::two_pi<float>();
        float x = r * glm::cos(angle);
        float z = r * glm::sin(angle);
        vertices.push_back({glm::vec3(x, h * 0.5f, z)});
    }
    uint32_t topCenter = 0;
    uint32_t topStart = topCenter + 1;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t curr = topStart + i;
        uint32_t next = topStart + ((i + 1) % n);
        indices.push_back(topCenter);
        indices.push_back(next);
        indices.push_back(curr);
    }

    // bottom face
    vertices.push_back({glm::vec3(0.0f, -h * 0.5f, 0.0f)});
    for (uint32_t i = 0; i < n; ++i) {
        float angle = float(i) / n * glm::two_pi<float>();
        float x = r * glm::cos(angle);
        float z = r * glm::sin(angle);
        vertices.push_back({glm::vec3(x, -h * 0.5f, z)});
    }
    uint32_t bottomCenter = topStart + n;
    uint32_t bottomStart = bottomCenter + 1;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t curr = bottomStart + i;
        uint32_t next = bottomStart + ((i + 1) % n);
        indices.push_back(bottomCenter);
        indices.push_back(curr);
        indices.push_back(next);
    }

    // side faces
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t top_curr = topStart + i;
        uint32_t top_next = topStart + ((i + 1) % n);
        uint32_t bottom_curr = bottomStart + i;
        uint32_t bottom_next = bottomStart + ((i + 1) % n);

        indices.push_back(top_curr);
        indices.push_back(top_next);
        indices.push_back(bottom_curr);

        indices.push_back(top_next);
        indices.push_back(bottom_next);
        indices.push_back(bottom_curr);
    }
}

// Subtask 4.2: Sphere Geometry
void buildSphere(uint32_t n, uint32_t m, float r, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices) {

    // north pole
    vertices.push_back({glm::vec3(0.0f, r, 0.0f)});

    for (uint32_t i = 1; i < m; ++i) {
        for (uint32_t j = 0; j < n; ++j) {
            float theta = glm::pi<float>() * float (i) / float(m);
            float phi = glm::two_pi<float>() * float(j) / float(n);
            float x = r * glm::sin(theta) * glm::cos(phi);
            float y = r * glm::cos(theta);
            float z = r * glm::sin(theta) * glm::sin(phi);
            vertices.push_back({glm::vec3(x, y, z)});
        }
    }

    // south pole
    vertices.push_back({glm::vec3(0.0f, -r, 0.0f)});

    uint32_t topCenter = 0;
    uint32_t RingStart = topCenter + 1;
    // indices: first ring
    for (uint32_t j = 0; j < n; ++j) {
        uint32_t curr = RingStart + j;
        uint32_t next = RingStart + ((j + 1) % n);
        indices.push_back(topCenter);
        indices.push_back(next);
        indices.push_back(curr);
    }
    // indices: mid quads
    for (uint32_t i = 0; i < m - 2; ++i) {
        RingStart = i * n + 1;
        uint32_t nextRingStart = RingStart + n;
        for (uint32_t j = 0; j < n; ++j) {
            uint32_t curr = RingStart + j;
            uint32_t next = RingStart + ((j + 1) % n);
            uint32_t curr_down = nextRingStart + j;
            uint32_t next_down = nextRingStart + ((j + 1) % n);

            indices.push_back(curr);
            indices.push_back(next);
            indices.push_back(curr_down);

            indices.push_back(next);
            indices.push_back(next_down);
            indices.push_back(curr_down);
        }
    }

    // indices: last ring
    RingStart = n * (m - 2) + 1;
    uint32_t bottomCenter = RingStart + n;
    for (uint32_t j = 0; j < n; ++j) {
        uint32_t curr = RingStart + j;
        uint32_t next = RingStart + ((j + 1) % n);
        indices.push_back(bottomCenter);
        indices.push_back(curr);
        indices.push_back(next);
    }
}

// Subtask 4.3: Bézier Cylinder Geometry

// de Casteljau algorithm
glm::vec3 bezierPoint(std::vector<glm::vec3>& vertices, float t) {
    std::vector<glm::vec3> vertices_temp = vertices;
    uint32_t size = int(vertices.size());
    for (uint32_t i = 1; i < size; ++i) {
        for (uint32_t j = 0; j < size - i; ++j) {
            vertices_temp[j] = (1.0f - t) * vertices_temp[j] + t * vertices_temp[j + 1];
        }
    }
    return vertices_temp[0];
}

glm::vec3 derivativeBezierPoint(std::vector<glm::vec3>& vertices, float t) {
    std::vector<glm::vec3> vertices_temp;
    uint32_t degree = uint32_t(vertices.size() - 1);
    for (uint32_t i = 0; i < degree; ++i) {
        vertices_temp.push_back(float(degree) * (vertices[i + 1] - vertices[i]));
    }
    return bezierPoint(vertices_temp, t);
}

void buildBezierCylinder(uint32_t s, uint32_t n, float r, std::vector<glm::vec3>& controlPoints, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices) {
    if (controlPoints.size() < 2) {
        throw std::runtime_error("Need at least 2 control points");
    }
    uint32_t n_circles = s + 1;

    // new axes for every circle
    std::vector<glm::vec3> centers(n_circles);
    std::vector<glm::vec3> tangents(n_circles);
    for (uint32_t i = 0; i < n_circles; ++i) {
        float t = float(i) / float(s);
        centers[i] = bezierPoint(controlPoints, t);
        tangents[i] = glm::normalize(derivativeBezierPoint(controlPoints, t));
    }

    std::vector<glm::vec3> normals(n_circles);
    std::vector<glm::vec3> binormals(n_circles);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(up, tangents[0])) > 0.999f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    normals[0] = glm::normalize(glm::cross(tangents[0],up));
    binormals[0] = glm::normalize(glm::cross(tangents[0], normals[0]));

    // parallel transport to compute the normal and binormal for each circle
    for (uint32_t i = 1; i < n_circles; ++i) {
        glm::vec3 axis = glm::cross(tangents[i - 1], tangents[i]);
        if (glm::length(axis) < 1e-8f) {
            normals[i] = normals[i - 1];
            binormals[i] = binormals[i - 1];
        }
        else {
            axis = glm::normalize(axis);
            float cos_angle = glm::clamp(glm::dot(tangents[i - 1], tangents[i]), -1.0f, 1.0f);
            float angle = acosf(cos_angle);
            normals[i] = glm::rotate(normals[i - 1], angle, axis);
            binormals[i] = glm::normalize(glm::cross(tangents[i], normals[i]));
        }
    }

    // vertices
    for (uint32_t i = 0; i < n_circles; ++i) {
        for (uint32_t j = 0; j < n; ++j) {
            float theta = glm::two_pi<float>() * float(j) / float(n);
            glm::vec3 local_dir = glm::cos(theta) * normals[i] + glm::sin(theta) * binormals[i];
            glm::vec3 position = centers[i] + r * local_dir;
            vertices.push_back(position);
        }
    }
    vertices.push_back(centers[0]);
    vertices.push_back(centers[s]);

    uint32_t topCenter = n_circles * n;
    uint32_t bottomCenter = topCenter + 1;
    uint32_t bottomOffset = s * n;

    // indices: side faces
    for (uint32_t i = 0; i < s; ++i) {
        for (uint32_t j = 0; j < n; ++j) {
            uint32_t top_curr = i * n + j;
            uint32_t top_next = i * n + ((j + 1) % n);
            uint32_t bottom_curr = (i + 1) * n + j;
            uint32_t bottom_next = (i + 1) * n + ((j + 1) % n);

            indices.push_back(top_curr);
            indices.push_back(bottom_next);
            indices.push_back(bottom_curr);

            indices.push_back(top_curr);
            indices.push_back(top_next);
            indices.push_back(bottom_next);
        }
    }

    // indices: first circle
    for (uint32_t j = 0; j < n; ++j) {
        uint32_t curr = j;
        uint32_t next = (j + 1) % n;
        indices.push_back(topCenter);
        indices.push_back(next);
        indices.push_back(curr);
    }

    // indices: last circle
    for (uint32_t j = 0; j < n; ++j) {
        uint32_t curr = bottomOffset + j;
        uint32_t next = bottomOffset + (j + 1) % n;
        indices.push_back(bottomCenter);
        indices.push_back(curr);
        indices.push_back(next);
    }
}
