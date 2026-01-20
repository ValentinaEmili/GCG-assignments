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
int cull_mode_idx = 0;
bool draw_normals = false;
bool draw_fresnel = true;
bool draw_texcoords = false;

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
    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        draw_normals = !draw_normals;
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        draw_fresnel = !draw_fresnel;
    }
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        draw_texcoords = !draw_texcoords;
    }
}

struct UniformBufferObject {
    alignas(16) glm::vec4 color;
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::ivec4 userInput;
    alignas(16) glm::vec4 camera_pos;
    alignas(16) glm::vec4 material;
    alignas(16) glm::vec4 texture;
};

// Subtask 5.8: Uniform Buffers for Lights
struct DirectionalLightUBO {
    alignas(16) glm::vec4 color;
    alignas(16) glm::vec4 direction;
};

struct PointLightUBO {
    alignas(16) glm::vec4 color;
    alignas(16) glm::vec4 position;
    alignas(16) glm::vec4 attenuation;
};

// Subtask 5.9 - 5.10: Phong Illumination with Gouraud/Phong Shading
enum class ShadingMode {
    Multicolor,
    Gouraud,
    Phong,
    PhongSpecular
};

// Subtask 2.6: Orbit Camera
float zoom = 5.0f; // initial radium
float yaw = 0.0f;
float pitch = 0.0f;
float max_pitch = glm::radians(89.999f);
float min_pitch = glm::radians(-89.999f);
double last_x = 0.0f;
double last_y = 0.0f;
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

// Subtask 5.1: Cube geometry with normal vectors
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 tex_coord;
};

std::vector<Vertex> cube_vertices = {
    // front
    {{-cube_width / 2, -cube_height / 2,  cube_depth / 2}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ cube_width / 2, -cube_height / 2,  cube_depth / 2}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ cube_width / 2,  cube_height / 2,  cube_depth / 2}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-cube_width / 2,  cube_height / 2,  cube_depth / 2}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    // back
    {{-cube_width / 2, -cube_height / 2, -cube_depth / 2}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{ cube_width / 2, -cube_height / 2, -cube_depth / 2}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{ cube_width / 2,  cube_height / 2, -cube_depth / 2}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{-cube_width / 2,  cube_height / 2, -cube_depth / 2}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // left
    {{-cube_width / 2, -cube_height / 2, -cube_depth / 2}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{-cube_width / 2, -cube_height / 2,  cube_depth / 2}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{-cube_width / 2,  cube_height / 2,  cube_depth / 2}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-cube_width / 2,  cube_height / 2, -cube_depth / 2}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    // right
    {{ cube_width / 2, -cube_height / 2, -cube_depth / 2}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ cube_width / 2, -cube_height / 2,  cube_depth / 2}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ cube_width / 2,  cube_height / 2,  cube_depth / 2}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{ cube_width / 2,  cube_height / 2, -cube_depth / 2}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    // bottom
    {{-cube_width / 2, -cube_height / 2, -cube_depth / 2}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ cube_width / 2, -cube_height / 2, -cube_depth / 2}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ cube_width / 2, -cube_height / 2,  cube_depth / 2}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-cube_width / 2, -cube_height / 2,  cube_depth / 2}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    // top
    {{-cube_width / 2,  cube_height / 2, -cube_depth / 2}, { 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{ cube_width / 2,  cube_height / 2, -cube_depth / 2}, { 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{ cube_width / 2,  cube_height / 2,  cube_depth / 2}, { 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{-cube_width / 2,  cube_height / 2,  cube_depth / 2}, { 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
};

std::vector<uint32_t> cube_indices = {
    // front
    0, 1, 2, 0, 2, 3,
    // back
    4, 6, 5, 4, 7, 6,
    // left
    8, 9, 10, 8, 10, 11,
    // right
    12, 14, 13, 12, 15, 14,
    // bottom
    16, 17, 18, 16, 18, 19,
    // top
    20, 22, 21, 20, 23, 22
};

// Subtask 6.7: Load DDS Textures into Images
struct Texture {
    VkBuffer buffer;
    VkImage image;
    VkImageView view;
};
Texture loadTexture(const char* file, VkPhysicalDevice vk_physical_device, VkDevice vk_device, VkCommandPool command_pool, VkQueue vk_queue);

// Subtask 4.1 - 5.3: Cylinder Geometry with normal vectors
struct MeshResources {
    UniformBufferObject ubo;
    VkBuffer uniformBuffer;
    VkDeviceSize uniformBufferSize;
    VkPipeline pipelines[4];
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
};

MeshResources SetupMesh(
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices,
    glm::mat4 view,
    glm::mat4 projection,
    GLFWwindow* window,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout,
    VkDescriptorSet &descriptor_set,
    std::array<VkDescriptorSetLayoutBinding, 5>& descriptor_set_layout_binding,
    VkBuffer dirLightBuffer,
    VkBuffer pointLightBuffer,
    VkSampler textureSampler,
    Texture diffuse_texture,
    Texture specular_texture,
    ShadingMode shadingMode,
    VkDevice vk_device,
    glm::vec3 translation,
    glm::vec3 rotation_axis,
    glm::vec4 color,
    glm::vec4 material,
    float rotation_degrees=0.0f,
    bool multicolor=false,
    bool translate=true
    );

MeshResources createMesh(
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices,
    glm::mat4 view,
    glm::mat4 projection,
    GLFWwindow* window,
    VkDescriptorSet &descriptor_set,
    std::array<VkDescriptorSetLayoutBinding, 5>& descriptor_set_layout_binding,
    VkBuffer dirLightBuffer,
    VkBuffer pointLightBuffer,
    VkSampler textureSampler,
    Texture diffuse_texture,
    Texture specular_texture,
    ShadingMode shadingMode,
    VkDevice vk_device);

void destroyMeshResources(VkDevice vk_device, MeshResources& mesh);
void destroyTexture(VkDevice vk_device, Texture& diffuse, Texture& specular);

void buildCylinder(float h, float r, uint32_t n, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

// Subtask 4.2 - 5.4: Sphere Geometry with normal vectors
void buildSphere(uint32_t n, uint32_t m, float r, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

// Subtask 4.3 - 5.5: Bézier Cylinder Geometry with normal vectors
glm::vec3 bezierPoint(std::vector<glm::vec3>& vertices, float t);
glm::vec3 derivativeBezierPoint(std::vector<glm::vec3>& vertices, float t);
void buildBezierCylinder(uint32_t s, uint32_t n, float r, std::vector<glm::vec3>& controlPoints, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

PFN_vkCmdPipelineBarrier2KHR g_vkCmdPipelineBarrier2KHR;
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

    // Subtask 3.3 - 5.7: Interaction
    std::string init_renderer_filepath = "assets/settings/renderer_standard.ini";
    if (cmdline_args.init_renderer) {
        init_renderer_filepath = cmdline_args.init_renderer_filepath;
    }

    INIReader renderer_reader(init_renderer_filepath);
    is_wireframe = renderer_reader.GetBoolean("renderer", "wireframe", false);
    cull_mode_idx = renderer_reader.GetBoolean("renderer", "backface_culling", false) ? 1 : 0;
    draw_normals = renderer_reader.GetBoolean("renderer", "normals", false);
    draw_fresnel = renderer_reader.GetBoolean("renderer", "fresnel", true);
    draw_texcoords = renderer_reader.GetBoolean("renderer", "texcoords", false);

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

    std::vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME};

#ifdef __APPLE__
    device_extensions.push_back("VK_KHR_portability_subset");
#endif

    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();
    device_create_info.pEnabledFeatures = &device_features; // enable physical device features, subtask 3.1: Wireframe Mode

    VkPhysicalDeviceSynchronization2Features sync2Features{};
    sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Features.pNext = nullptr;
    sync2Features.synchronization2 = VK_TRUE;

    device_create_info.pNext = &sync2Features;

    result = vkCreateDevice(vk_physical_device, &device_create_info, nullptr, &vk_device);
    VKL_CHECK_VULKAN_RESULT(result);

    if (!vk_device) {
        VKL_EXIT_WITH_ERROR("No VkDevice created or handle not assigned.");
    }

    auto* procAddr = vkGetDeviceProcAddr(vk_device, "vkCmdPipelineBarrier2KHR");
    if (procAddr == nullptr) {
        throw std::runtime_error("Failed to load vkCmdPipelineBarrier2KHR!");
    } else {
        g_vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(procAddr);
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
    // end subtask 2.7

    // Init the framework:
    if (!gcgInitFramework(vk_instance, vk_surface, vk_physical_device, vk_device, vk_queue, swapchain_config)) {
        VKL_EXIT_WITH_ERROR("Failed to init framework");
    }
    VKL_LOG("Subtask 1.9 done.");

    // Subtask 6.7: Load DDS Textures into Images
    VkCommandPoolCreateInfo command_pool_info{};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    command_pool_info.queueFamilyIndex = selected_queue_family_index;

    VkCommandPool texture_command_pool;
    vkCreateCommandPool(vk_device, &command_pool_info, nullptr, &texture_command_pool);

    Texture wood = loadTexture("../assets/textures/wood_texture.dds", vk_physical_device, vk_device, texture_command_pool, vk_queue);
    Texture wood_specular = loadTexture("../assets/textures/wood_texture_specular.dds", vk_physical_device, vk_device, texture_command_pool, vk_queue);
    Texture tiles_diffuse = loadTexture("../assets/textures/tiles_diffuse.dds", vk_physical_device, vk_device, texture_command_pool, vk_queue);
    Texture tiles_specular = loadTexture("../assets/textures/tiles_specular.dds", vk_physical_device, vk_device, texture_command_pool, vk_queue);


    // Subtask 6.9: Create a Sampler
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = VK_LOD_CLAMP_NONE;
    VkSampler vk_sampler;
    vkCreateSampler(vk_device, &sampler_info, nullptr, &vk_sampler);

    // Subtask 2.2: Create a Uniform Buffer
    std::array<VkDescriptorSetLayoutBinding, 5> descriptor_set_layout_binding{};
    for (uint32_t i = 0; i < 5; i++) {
        descriptor_set_layout_binding[i].binding = i;
        descriptor_set_layout_binding[i].descriptorCount = 1;
        if (i >= 3) { // texture
            descriptor_set_layout_binding[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_set_layout_binding[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        } else {
            descriptor_set_layout_binding[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_set_layout_binding[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        descriptor_set_layout_binding[i].pImmutableSamplers = nullptr;
    }

    // Subtask 2.3: Allocate and Write Descriptors
    std::vector<VkDescriptorPoolSize> pool_size(2);
    pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size[0].descriptorCount = 8 * 16;
    pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size[1].descriptorCount = 8;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags = 0;
    descriptor_pool_create_info.maxSets = 8;
    descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_size.size());
    descriptor_pool_create_info.pPoolSizes = pool_size.data();
    descriptor_pool_create_info.maxSets = 8;

    VkDescriptorPool descriptor_pool;
    VkResult des_pool_result = vkCreateDescriptorPool(vk_device, &descriptor_pool_create_info, nullptr, &descriptor_pool);
    VKL_CHECK_VULKAN_ERROR(des_pool_result);

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = nullptr;
    descriptor_set_layout_create_info.flags = 0;
    descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(descriptor_set_layout_binding.size());
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_binding.data();

    VkDescriptorSetLayout descriptor_set_layout;
    VkResult des_set_layout_result = vkCreateDescriptorSetLayout(vk_device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout);
    VKL_CHECK_VULKAN_ERROR(des_set_layout_result);


    DirectionalLightUBO dirLightUBO{};
    dirLightUBO.color = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
    dirLightUBO.direction = glm::vec4(glm::normalize(glm::vec3(0.0f, 1.0f, -1.0f)), 0.0f);
    VkBuffer dirLightBuffer = vklCreateHostCoherentBufferWithBackingMemory(sizeof(DirectionalLightUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(dirLightBuffer, &dirLightUBO, sizeof(DirectionalLightUBO));

    PointLightUBO pointLightUBO{};
    pointLightUBO.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    pointLightUBO.position = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    pointLightUBO.attenuation = glm::vec4(1.0f, 0.4f, 0.1f, 0.0f);
    VkBuffer pointLightBuffer = vklCreateHostCoherentBufferWithBackingMemory(sizeof(PointLightUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(pointLightBuffer, &pointLightUBO, sizeof(PointLightUBO));

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
    glm::mat4 view = glm::inverse(camera_to_world);

    // Subtask 3.5 - 5.1: Cubemap with normals vectors
    VkDescriptorSet descriptor_set_cube{};
    MeshResources cube = SetupMesh(cube_vertices, cube_indices, view, projection, window,
        descriptor_pool, descriptor_set_layout, descriptor_set_cube, descriptor_set_layout_binding,
        dirLightBuffer, pointLightBuffer, vk_sampler, wood,  wood_specular, ShadingMode::PhongSpecular, vk_device,
        glm::vec3(-0.6f, -0.9f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.21f, 0.16f, 1.0f), glm::vec4(0.05f, 0.8f, 0.5f, 10.0f), 45.0f);

    // Subtask 4.1 - 5.3: Cylinder Geometry with normal vectors
    std::vector<Vertex> cylinder_vertices;
    std::vector<uint32_t> cylinder_indices;
    buildCylinder(1.6f, 0.21f, 20, cylinder_vertices, cylinder_indices);

    VkDescriptorSet descriptor_set_cylinder{};
    MeshResources cylinder = SetupMesh(cylinder_vertices, cylinder_indices, view, projection, window,
        descriptor_pool, descriptor_set_layout, descriptor_set_cylinder, descriptor_set_layout_binding,
        dirLightBuffer, pointLightBuffer,vk_sampler, tiles_diffuse, tiles_specular, ShadingMode::PhongSpecular, vk_device,
        glm::vec3(0.6f, 0.3f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec4(0.75f, 0.25f, 0.01f, 1.0f), glm::vec4(0.1f, 0.9f, 0.3f, 5.0f));

    // Subtask 4.2 - 5.4: Sphere Geometry with normal vectors
    std::vector<Vertex> sphere_vertices;
    std::vector<uint32_t> sphere_indices;
    buildSphere(36, 18, 0.26f, sphere_vertices, sphere_indices);

    VkDescriptorSet descriptor_set_sphere{};
    MeshResources sphere = SetupMesh(sphere_vertices, sphere_indices, view, projection, window,
        descriptor_pool, descriptor_set_layout, descriptor_set_sphere, descriptor_set_layout_binding,
        dirLightBuffer, pointLightBuffer, vk_sampler, tiles_diffuse, tiles_specular, ShadingMode::PhongSpecular, vk_device,
        glm::vec3(0.6f, -0.9f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.21f, 0.16f, 1.0f), glm::vec4(0.05f, 0.8f, 0.5f, 10.0f));

    // Subtask 4.3 - 5.5: Bézier Cylinder Geometry with normal vectors
    std::vector<Vertex> bezier_cylinder_vertices;
    std::vector<uint32_t> bezier_cylinder_indices;
    std::vector<glm::vec3> control_points = {
        {-0.3f, 0.6f, 0.0f},
        {0.0f, 1.6f, 0.0f},
        {1.4f, 0.3f, 0.0f},
        {0.0f, 0.3f, 0.0f},
        {0.0f, -0.5f, 0.0f}
    };
    buildBezierCylinder(36, 20, 0.21f, control_points,bezier_cylinder_vertices, bezier_cylinder_indices);

    VkDescriptorSet descriptor_set_bezier_cylinder{};
    MeshResources bezier_cylinder = SetupMesh(bezier_cylinder_vertices, bezier_cylinder_indices, view, projection, window,
        descriptor_pool, descriptor_set_layout, descriptor_set_bezier_cylinder, descriptor_set_layout_binding,
        dirLightBuffer, pointLightBuffer, vk_sampler, tiles_diffuse, tiles_specular, ShadingMode::PhongSpecular, vk_device,
        glm::vec3(-0.6f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec4(0.75f, 0.25f, 0.01f, 1.0f), glm::vec4(0.1f, 0.9f, 0.3f, 5.0f));

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

        vklWaitForNextSwapchainImage();
        vklStartRecordingCommands();

        // Subtask 3.4: Command Buffer Recording
        VkCommandBuffer cmdBuffer = vklGetCurrentCommandBuffer();
        VkDeviceSize offsets[] = {0};

        // Subtask 3.5 - 5.1: Cubemap with normal vectors
        cube.ubo.view = view;
        cube.ubo.projection = projection;
        cube.ubo.userInput = glm::ivec4(draw_normals ? 1 : 0, draw_fresnel ? 1 : 0, draw_texcoords ? 1: 0, 0);
        cube.ubo.camera_pos = glm::vec4(camera_pos, 1.0f);
        vklCopyDataIntoHostCoherentBuffer(cube.uniformBuffer, &cube.ubo, cube.uniformBufferSize);

        VkPipeline cube_pipeline = cube.pipelines[is_wireframe * 2 + cull_mode_idx];
        VkPipelineLayout cube_pipeline_layout = vklGetLayoutForPipeline(cube_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cube_pipeline);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &cube.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, cube.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cube_pipeline_layout, 0, 1, &descriptor_set_cube, 0, nullptr);
        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(cube_indices.size()), 1, 0, 0, 0);

        // Subtask 4.1 - 5.3: Cylinder Geometry with normal vectors
        cylinder.ubo.view = view;
        cylinder.ubo.projection = projection;
        cylinder.ubo.userInput = glm::ivec4(draw_normals ? 1 : 0, draw_fresnel ? 1 : 0, draw_texcoords ? 1: 0, 0);
        cylinder.ubo.camera_pos = glm::vec4(camera_pos, 1.0f);
        vklCopyDataIntoHostCoherentBuffer(cylinder.uniformBuffer, &cylinder.ubo, cylinder.uniformBufferSize);

        VkPipeline cylinder_pipeline = cylinder.pipelines[is_wireframe * 2 + cull_mode_idx];
        VkPipelineLayout cylinder_pipeline_layout = vklGetLayoutForPipeline(cylinder_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cylinder_pipeline);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &cylinder.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, cylinder.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cylinder_pipeline_layout, 0, 1, &descriptor_set_cylinder, 0, nullptr);
        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(cylinder_indices.size()), 1, 0, 0, 0);

        // Subtask 4.2 - 5.4: Sphere Geometry with normal vectors
        sphere.ubo.view = view;
        sphere.ubo.projection = projection;
        sphere.ubo.userInput = glm::ivec4(draw_normals ? 1 : 0, draw_fresnel ? 1 : 0, draw_texcoords ? 1: 0, 0);
        sphere.ubo.camera_pos = glm::vec4(camera_pos, 1.0f);
        vklCopyDataIntoHostCoherentBuffer(sphere.uniformBuffer, &sphere.ubo, sphere.uniformBufferSize);

        VkPipeline sphere_pipeline = sphere.pipelines[is_wireframe * 2 + cull_mode_idx];
        VkPipelineLayout sphere_pipeline_layout = vklGetLayoutForPipeline(sphere_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sphere_pipeline);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &sphere.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, sphere.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sphere_pipeline_layout, 0, 1, &descriptor_set_sphere, 0, nullptr);
        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(sphere_indices.size()), 1, 0, 0, 0);

        // Subtask 4.3 - 5.5: Bézier Cylinder with normal vectors
        bezier_cylinder.ubo.view = view;
        bezier_cylinder.ubo.projection = projection;
        bezier_cylinder.ubo.userInput = glm::ivec4(draw_normals ? 1 : 0, draw_fresnel ? 1 : 0, draw_texcoords ? 1 : 0, 0);
        bezier_cylinder.ubo.camera_pos = glm::vec4(camera_pos, 1.0f);
        vklCopyDataIntoHostCoherentBuffer(bezier_cylinder.uniformBuffer, &bezier_cylinder.ubo, bezier_cylinder.uniformBufferSize);

        VkPipeline curr_bezierCylinder_pipeline = bezier_cylinder.pipelines[is_wireframe * 2 + cull_mode_idx];
        VkPipelineLayout bezierCylinder_pipeline_layout = vklGetLayoutForPipeline(curr_bezierCylinder_pipeline);
        vklCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curr_bezierCylinder_pipeline);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &bezier_cylinder.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuffer, bezier_cylinder.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bezierCylinder_pipeline_layout, 0, 1, &descriptor_set_bezier_cylinder, 0, nullptr);
        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(bezier_cylinder_indices.size()), 1, 0, 0, 0);

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
    destroyMeshResources(vk_device, cylinder);
    destroyMeshResources(vk_device, sphere);
    destroyMeshResources(vk_device, cube);
    destroyMeshResources(vk_device, bezier_cylinder);

    vklDestroyHostCoherentBufferAndItsBackingMemory(dirLightBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(pointLightBuffer);

    destroyTexture(vk_device, wood, wood_specular);
    destroyTexture(vk_device, tiles_diffuse, tiles_specular);

    vklDestroyDeviceLocalImageAndItsBackingMemory(depth_image);
    vkDestroySampler(vk_device, vk_sampler, nullptr);

    vkDestroyDescriptorSetLayout(vk_device, descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(vk_device, descriptor_pool, nullptr);
    vkDestroyCommandPool(vk_device, texture_command_pool, nullptr);

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

MeshResources SetupMesh(
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices,
    glm::mat4 view,
    glm::mat4 projection,
    GLFWwindow* window,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout,
    VkDescriptorSet &descriptor_set,
    std::array<VkDescriptorSetLayoutBinding, 5>& descriptor_set_layout_binding,
    VkBuffer dirLightBuffer,
    VkBuffer pointLightBuffer,
    VkSampler textureSampler,
    Texture diffuse_texture,
    Texture specular_texture,
    ShadingMode shadingMode,
    VkDevice vk_device,
    glm::vec3 translation,
    glm::vec3 rotation_axis,
    glm::vec4 color,
    glm::vec4 material,
    float rotation_degrees,
    bool multicolor,
    bool translate
    ) {

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor_set_layout;

    VkResult res = vkAllocateDescriptorSets(vk_device, &alloc_info, &descriptor_set);
    VKL_CHECK_VULKAN_ERROR(res);

    MeshResources mesh = createMesh(vertices, indices, view, projection, window, descriptor_set,
        descriptor_set_layout_binding, dirLightBuffer, pointLightBuffer, textureSampler, diffuse_texture, specular_texture,
        shadingMode, vk_device);

    glm::mat4 model = glm::mat4(1.0f);
    if (translate) {
        model = glm::translate(model, translation);
    }
    if (rotation_degrees != 0.0f) {
        model = glm::rotate(model, glm::radians(rotation_degrees), rotation_axis);
    }
    mesh.ubo.model = model;
    mesh.ubo.view = view;
    mesh.ubo.projection = projection;
    mesh.ubo.userInput = glm::ivec4(draw_normals ? 1 : 0, draw_fresnel ? 1 : 0, multicolor? 1: 0, 0);
    mesh.ubo.material = material;
    mesh.ubo.camera_pos = glm::vec4(0.0f, 0.0f, zoom, 1.0f);
    if (!multicolor) {
        mesh.ubo.color = color;
    }
    vklCopyDataIntoHostCoherentBuffer(mesh.uniformBuffer, &mesh.ubo, mesh.uniformBufferSize);

    return mesh;
}

MeshResources createMesh(
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices,
    glm::mat4 view,
    glm::mat4 projection,
    GLFWwindow* window,
    VkDescriptorSet &descriptor_set,
    std::array<VkDescriptorSetLayoutBinding, 5>& descriptor_set_layout_binding,
    VkBuffer dirLightBuffer,
    VkBuffer pointLightBuffer,
    VkSampler textureSampler,
    Texture diffuse_texture,
    Texture specular_texture,
    ShadingMode shadingMode,
    VkDevice vk_device) {
    if (descriptor_set == VK_NULL_HANDLE) {
        throw std::runtime_error("Unable to create mesh descriptor set");
    }
    MeshResources mesh;

    VkDeviceSize vertex_buffer_size = sizeof(Vertex) * vertices.size();
    mesh.vertexBuffer = vklCreateHostCoherentBufferWithBackingMemory(vertex_buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(mesh.vertexBuffer, vertices.data(), vertex_buffer_size);

    VkDeviceSize index_buffer_size = sizeof(uint32_t) * indices.size();
    mesh.indexBuffer = vklCreateHostCoherentBufferWithBackingMemory(index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(mesh.indexBuffer, indices.data(), index_buffer_size);

    VklGraphicsPipelineConfig pipe_config{};
    VkVertexInputBindingDescription bindings{};
    bindings.binding = 0;
    bindings.stride = sizeof(Vertex);
    bindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    pipe_config.vertexInputBuffers.clear();
    pipe_config.inputAttributeDescriptions.clear();
    pipe_config.vertexInputBuffers.push_back(bindings);

    // attribute: position
    VkVertexInputAttributeDescription position = {};
    position.location = 0;
    position.binding = 0;
    position.format = VK_FORMAT_R32G32B32_SFLOAT;
    position.offset = offsetof(Vertex, position);
    pipe_config.inputAttributeDescriptions.push_back(position);

    // attribute: normal
    VkVertexInputAttributeDescription normal = {};
    normal.location = 1;
    normal.binding = 0;
    normal.format = VK_FORMAT_R32G32B32_SFLOAT;
    normal.offset = offsetof(Vertex, normal);
    pipe_config.inputAttributeDescriptions.push_back(normal);

    // attribute: color
    VkVertexInputAttributeDescription color = {};
    color.location = 2;
    color.binding = 0;
    color.format = VK_FORMAT_R32G32B32_SFLOAT;
    color.offset = offsetof(Vertex, color);
    pipe_config.inputAttributeDescriptions.push_back(color);

    if (shadingMode != ShadingMode::Multicolor) {
        // attribute: texture
        VkVertexInputAttributeDescription texture = {};
        texture.location = 3;
        texture.binding = 0;
        texture.format = VK_FORMAT_R32G32_SFLOAT;
        texture.offset = offsetof(Vertex, tex_coord);
        pipe_config.inputAttributeDescriptions.push_back(texture);
    }

    std::string vertex_shader_path;
    std::string fragment_shader_path;

    switch(shadingMode) {
        case ShadingMode::Gouraud:
            vertex_shader_path = gcgLoadShaderFilePath("assets/shaders/gouraud.vert");
            fragment_shader_path = gcgLoadShaderFilePath("assets/shaders/gouraud.frag");
            break;
        case ShadingMode::Phong:
            vertex_shader_path = gcgLoadShaderFilePath("assets/shaders/phong.vert");
            fragment_shader_path = gcgLoadShaderFilePath("assets/shaders/phong.frag");
            break;
        case ShadingMode::PhongSpecular:
            vertex_shader_path = gcgLoadShaderFilePath("assets/shaders/phong.vert");
            fragment_shader_path = gcgLoadShaderFilePath("assets/shaders/phong_specular.frag");
            break;
        case ShadingMode::Multicolor:
            vertex_shader_path = gcgLoadShaderFilePath("assets/shaders/cornell.vert");
            fragment_shader_path = gcgLoadShaderFilePath("assets/shaders/cornell.frag");
    }

    pipe_config.vertexShaderPath = vertex_shader_path.c_str();
    pipe_config.fragmentShaderPath = fragment_shader_path.c_str();

    pipe_config.descriptorLayout.push_back(descriptor_set_layout_binding[0]);
    pipe_config.descriptorLayout.push_back(descriptor_set_layout_binding[1]);
    pipe_config.descriptorLayout.push_back(descriptor_set_layout_binding[2]);
    pipe_config.descriptorLayout.push_back(descriptor_set_layout_binding[3]);
    pipe_config.descriptorLayout.push_back(descriptor_set_layout_binding[4]);

    for (uint32_t wire = 0; wire < 2; wire++) {
        for (uint32_t cull = 0; cull < 2; cull++) {
            pipe_config.polygonDrawMode = wire ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
            pipe_config.triangleCullingMode = (cull == 0) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
            mesh.pipelines[wire * 2 + cull] = vklCreateGraphicsPipeline(pipe_config);
        }
    }

    mesh.ubo.view = view;
    mesh.ubo.projection = projection;
    mesh.uniformBufferSize = sizeof(mesh.ubo);
    mesh.uniformBuffer = vklCreateHostCoherentBufferWithBackingMemory(mesh.uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(mesh.uniformBuffer, &mesh.ubo, mesh.uniformBufferSize);

    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);

    // Subtask 5.8: Uniform Buffers for Lights

    // UBO
    VkDescriptorBufferInfo descriptor_buffer_object_info{};
    descriptor_buffer_object_info.buffer = mesh.uniformBuffer;
    descriptor_buffer_object_info.offset = 0;
    descriptor_buffer_object_info.range = mesh.uniformBufferSize;

    // directional light
    VkDescriptorBufferInfo descriptor_buffer_dirLight_info{};
    descriptor_buffer_dirLight_info.buffer = dirLightBuffer;
    descriptor_buffer_dirLight_info.range = sizeof(DirectionalLightUBO);

    // point light
    VkDescriptorBufferInfo descriptor_buffer_pointLight_info{};
    descriptor_buffer_pointLight_info.buffer = pointLightBuffer;
    descriptor_buffer_pointLight_info.range = sizeof(PointLightUBO);

    // diffuse texture
    VkDescriptorImageInfo descriptor_diffuse_image_info{};
    descriptor_diffuse_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_diffuse_image_info.imageView = diffuse_texture.view;
    descriptor_diffuse_image_info.sampler = textureSampler;

    // specular texture
    VkDescriptorImageInfo descriptor_specular_image_info{};
    descriptor_specular_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_specular_image_info.imageView = specular_texture.view;
    descriptor_specular_image_info.sampler = textureSampler;

    std::array<VkWriteDescriptorSet, 5> write_descriptor_set{};
    // UBO
    write_descriptor_set[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set[0].dstSet = descriptor_set;
    write_descriptor_set[0].dstBinding = 0;
    write_descriptor_set[0].dstArrayElement = 0;
    write_descriptor_set[0].descriptorCount = 1;
    write_descriptor_set[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set[0].pBufferInfo = &descriptor_buffer_object_info;

    // directional light
    write_descriptor_set[1] = write_descriptor_set[0];
    write_descriptor_set[1].dstBinding = 1;
    write_descriptor_set[1].pBufferInfo = &descriptor_buffer_dirLight_info;

    // point light
    write_descriptor_set[2] = write_descriptor_set[0];
    write_descriptor_set[2].dstBinding = 2;
    write_descriptor_set[2].pBufferInfo = &descriptor_buffer_pointLight_info;

    // diffuse texture
    write_descriptor_set[3] = write_descriptor_set[0];
    write_descriptor_set[3].dstBinding = 3;
    write_descriptor_set[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set[3].pImageInfo = &descriptor_diffuse_image_info;
    write_descriptor_set[3].pBufferInfo = nullptr;

    // specular texture
    write_descriptor_set[4] = write_descriptor_set[0];
    write_descriptor_set[4].dstBinding = 4;
    write_descriptor_set[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set[4].pImageInfo = &descriptor_specular_image_info;
    write_descriptor_set[4].pBufferInfo = nullptr;

    vkUpdateDescriptorSets(vk_device, static_cast<uint32_t>(write_descriptor_set.size()), write_descriptor_set.data(), 0, nullptr);

    return mesh;
}
void destroyMeshResources(VkDevice vk_device, MeshResources& mesh) {
    // Destroy pipelines (you have an array of 4)
    for (int i = 0; i < 4; i++) {
        if (mesh.pipelines[i] != VK_NULL_HANDLE) {
            vkDestroyPipeline(vk_device, mesh.pipelines[i], nullptr);
        }
    }
    // Destroy buffers and free their memory
    vklDestroyHostCoherentBufferAndItsBackingMemory(mesh.vertexBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(mesh.indexBuffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(mesh.uniformBuffer);
}

void destroyTexture(VkDevice vk_device, Texture& diffuse, Texture& specular) {

    vklDestroyHostCoherentBufferAndItsBackingMemory(diffuse.buffer);
    vklDestroyDeviceLocalImageAndItsBackingMemory(diffuse.image);
    vklDestroyHostCoherentBufferAndItsBackingMemory(specular.buffer);
    vklDestroyDeviceLocalImageAndItsBackingMemory(specular.image);

    vkDestroyImageView(vk_device, diffuse.view, nullptr);
    vkDestroyImageView(vk_device, specular.view, nullptr);
}

// Subtask 4.2 - 5.4 - 6.13: Sphere Geometry with normal vectors and fixed uv coordinates
void buildSphere(uint32_t n, uint32_t m, float r, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {

    // north pole
    uint32_t topCenter = vertices.size();
    for (uint32_t j = 0; j < n + 1; ++j) {
        float u = float(j) / float(n);
        vertices.push_back({{0.0f, r, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {u, 0.0f}});
    }

    for (uint32_t i = 1; i < m; ++i) {
        for (uint32_t j = 0; j < n + 1; ++j) {
            float theta = glm::pi<float>() * float (i) / float(m);
            float phi = glm::two_pi<float>() * float(j) / float(n);
            float x = r * glm::sin(theta) * glm::cos(phi);
            float y = r * glm::cos(theta);
            float z = r * glm::sin(theta) * glm::sin(phi);
            float u = float(j) / float(n);
            float v = float(i) / float(m);
            glm::vec3 position(x, y, z);
            glm::vec3 normal = glm::normalize(position);
            vertices.push_back({position, normal, {0.0f, 0.0f, 0.0f}, {u, v}});
        }
    }

    // south pole
    uint32_t southPoleStart = vertices.size();
    for (uint32_t j = 0; j < n + 1; ++j) {
        float u = float(j) / float(n);
        vertices.push_back({{0.0f, -r, 0.0f},{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {u, 1.0f} });
    }

    uint32_t RingStart = topCenter + (n + 1);
    // indices: first ring
    for (uint32_t j = 0; j < n; ++j) {
        uint32_t curr = RingStart + j;
        uint32_t next = RingStart + (j + 1);
        indices.push_back(topCenter + j);
        indices.push_back(next);
        indices.push_back(curr);
    }
    // indices: mid quads
    for (uint32_t i = 0; i < m - 2; ++i) {
        RingStart = topCenter + (n + 1) + (i * (n + 1));
        uint32_t nextRingStart = RingStart + (n + 1);
        for (uint32_t j = 0; j < n; ++j) {
            uint32_t curr = RingStart + j;
            uint32_t next = RingStart + (j + 1);
            uint32_t curr_down = nextRingStart + j;
            uint32_t next_down = nextRingStart + (j + 1);

            indices.push_back(curr);
            indices.push_back(next);
            indices.push_back(curr_down);

            indices.push_back(next);
            indices.push_back(next_down);
            indices.push_back(curr_down);
        }
    }

    // indices: last ring
    RingStart = topCenter + (n + 1) + (n + 1) * (m - 2);
    uint32_t bottomCenter = RingStart + (n + 1);
    for (uint32_t j = 0; j < n; ++j) {
        uint32_t curr = RingStart + j;
        uint32_t next = RingStart + (j + 1);
        indices.push_back(bottomCenter + j);
        indices.push_back(curr);
        indices.push_back(next);
    }
}

// Subtask 4.1 - 5.3 - 6.13: Cylinder Geometry with normal vectors and fixed uv coordinates
void buildCylinder(float h, float r, uint32_t n, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    // top face
    uint32_t topCenter = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{0.0f, h * 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});
    uint32_t topStart = static_cast<uint32_t>(vertices.size());

    for (uint32_t i = 0; i < n; ++i) {
        float angle = float(i) / n * glm::two_pi<float>();
        float x = r * glm::cos(angle);
        float z = r * glm::sin(angle);
        float u = (x / (2.0f * r)) + 0.5f;
        float v = 1.0f - ((z / (2.0f * r)) + 0.5f);
        vertices.push_back({{x, h * 0.5f, z}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {u, v}});
    }
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t curr = topStart + i;
        uint32_t next = topStart + ((i + 1) % n);
        indices.push_back(topCenter);
        indices.push_back(next);
        indices.push_back(curr);
    }

    // bottom face
    uint32_t bottomCenter = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{0.0f, -h * 0.5f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});
    uint32_t bottomStart = static_cast<uint32_t>(vertices.size());

    for (uint32_t i = 0; i < n; ++i) {
        float angle = float(i) / n * glm::two_pi<float>();
        float x = r * glm::cos(angle);
        float z = r * glm::sin(angle);
        float u = (x / (2.0f * r)) + 0.5f;
        float v = (z / (2.0f * r)) + 0.5f;
        vertices.push_back({{x, -h * 0.5f, z}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {u, v}});
    }
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t curr = bottomStart + i;
        uint32_t next = bottomStart + ((i + 1) % n);
        indices.push_back(bottomCenter);
        indices.push_back(curr);
        indices.push_back(next);
    }

    // side faces
    for (uint32_t i = 0; i < n+1; ++i) {
        float angle = float(i) / n * glm::two_pi<float>();
        float x = r * glm::cos(angle);
        float z = r * glm::sin(angle);
        float u = float(i) / n;
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));
        vertices.push_back({{x, h * 0.5f, z}, normal, {0.0f, 0.0f, 0.0f}, {u, 1.0f}});
        vertices.push_back({{x, -h * 0.5f, z}, normal, {0.0f, 0.0f, 0.0f}, {u,  0.0f}});
    }

    uint32_t sideStart = vertices.size() - 2 * (n+1);
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t curr = sideStart + 2 * i;
        uint32_t next = sideStart + 2 * (i + 1);

        indices.push_back(curr);
        indices.push_back(next);
        indices.push_back(curr + 1);

        indices.push_back(next);
        indices.push_back(next + 1);
        indices.push_back(curr + 1);
    }
}

// Subtask 4.3 - 5.5 - 6.13: Bézier Cylinder Geometry with normal vectors and fixed uv coordinates

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

void buildBezierCylinder(uint32_t s, uint32_t n, float r, std::vector<glm::vec3>& controlPoints, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
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
        up = glm::vec3(0.0f, 0.0f, 1.0f);
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
    std::vector<float> vCoords(n_circles);
    vCoords[0] = 0.0f;
    for (uint32_t j = 1; j < n_circles; ++j) {
        vCoords[j] = vCoords[j - 1] + glm::length(centers[j] - centers[j - 1]);
    }

    // side vertices
    for (uint32_t i = 0; i < n_circles; ++i) {
        for (uint32_t j = 0; j < n+1; ++j) {
            float theta = glm::two_pi<float>() * float(j) / float(n);
            glm::vec3 local_dir = glm::cos(theta) * binormals[i] + glm::sin(theta) * normals[i];
            glm::vec3 position = centers[i] + r * local_dir;
            glm::vec3 normal = glm::normalize(local_dir);
            float u = float(j) / float(n);
            float v = vCoords[i];
            vertices.push_back({position, normal, {0.0f, 0.0f, 0.0f}, {u, v}});
        }
    }

    // top base
    uint32_t topCapStart = static_cast<uint32_t>(vertices.size());
    for (uint32_t j = 0; j < n; ++j) {
        float theta = glm::two_pi<float>() * float(j) / float(n);
        glm::vec3 local_dir = glm::cos(theta) * normals[0] + glm::sin(theta) * binormals[0];
        glm::vec3 position = centers[0] + r * local_dir;
        glm::vec3 normal = -tangents[0];
        float u = (-glm::dot(local_dir, binormals[0]) * 0.5f) + 0.5f; // sin(theta) in [-1, 1] goes in [-0.5, 0.5] goes in [0, 1]
        float v = (glm::dot(local_dir, normals[0]) * 0.5f) + 0.5f; // cos(theta)
        vertices.push_back({position, normal, {0.0f, 0.0f, 0.0f}, {u, v}});
    }
    uint32_t topCenter = vertices.size();
    vertices.push_back({centers[0], -tangents[0], {0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});

    // bottom base
    uint32_t bottomCapStart = static_cast<uint32_t>(vertices.size());
    for (uint32_t j = 0; j < n; ++j) {
        float theta = glm::two_pi<float>() * float(j) / float(n);
        glm::vec3 local_dir = glm::cos(theta) * normals[s] + glm::sin(theta) * binormals[s];
        glm::vec3 position = centers[s] + r * local_dir;
        glm::vec3 normal = tangents[s];
        float u = (-glm::dot(local_dir, binormals[s]) * 0.5f) + 0.5f;
        float v = (glm::dot(local_dir, normals[s]) * 0.5f) + 0.5f;
        vertices.push_back({position, normal, {0.0f, 0.0f, 0.0f}, {u, v}});
    }
    uint32_t bottomCenter = vertices.size();
    vertices.push_back({centers[s], tangents[s], {0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});

    // indices: side faces
    for (uint32_t i = 0; i < s; ++i) {
        for (uint32_t j = 0; j < n; ++j) {
            uint32_t top_curr = i * (n+1) + j;
            uint32_t top_next = i * (n+1) + (j + 1);
            uint32_t bottom_curr = (i + 1) * (n+1) + j;
            uint32_t bottom_next = (i + 1) * (n+1) + (j + 1);

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
        uint32_t curr = topCapStart + j;
        uint32_t next = topCapStart + (j + 1) % n;
        indices.push_back(topCenter);
        indices.push_back(next);
        indices.push_back(curr);
    }

    // indices: last circle
    for (uint32_t j = 0; j < n; ++j) {
        uint32_t curr = bottomCapStart + j;
        uint32_t next = bottomCapStart + (j + 1) % n;
        indices.push_back(bottomCenter);
        indices.push_back(curr);
        indices.push_back(next);
    }
}

Texture loadTexture(const char* file, VkPhysicalDevice vk_physical_device, VkDevice vk_device, VkCommandPool command_pool, VkQueue vk_queue) {
    Texture texture{};
    VklImageInfo info = vklGetDdsImageInfo(file);
    texture.buffer = vklLoadDdsImageIntoHostCoherentBuffer(file);
    texture.image = vklCreateDeviceLocalImageWithBackingMemory(vk_physical_device, vk_device, info.extent.width, info.extent.height, info.imageFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1, 0);

    uint32_t mip_levels = static_cast<uint32_t>(std::log2(std::max(info.extent.width, info.extent.height)) + 1);
    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.pNext = nullptr;
    view_create_info.flags = 0;
    view_create_info.image = texture.image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = info.imageFormat;
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = mip_levels;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;
    vkCreateImageView(vk_device, &view_create_info, nullptr, &texture.view);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(vk_device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);

    // record an image layout transition
    VkImageMemoryBarrier2 image_memory_barrier{};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    image_memory_barrier.pNext = nullptr;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.image = texture.image;
    image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = mip_levels;

    VkDependencyInfo dependency_info;
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.pNext = nullptr;
    dependency_info.dependencyFlags = 0;
    dependency_info.memoryBarrierCount = 0;
    dependency_info.pMemoryBarriers = nullptr;
    dependency_info.bufferMemoryBarrierCount = 0;
    dependency_info.pBufferMemoryBarriers = nullptr;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;

    g_vkCmdPipelineBarrier2KHR(command_buffer, &dependency_info);

    //Subtask 6.12: Enable Mipmapping
    // copy the content of the buffer into the image for each level
    std::vector<VkBuffer> staging_buffers;
    for (uint32_t level = 0; level < mip_levels; level++){
        VklImageInfo vkl_image_info = vklGetDdsImageLevelInfo(file, level);
        VkBuffer curr_buffer = vklLoadDdsImageLevelIntoHostCoherentBuffer(file, level);
        staging_buffers.push_back(curr_buffer);
        VkBufferImageCopy image_copy{};
        image_copy.bufferOffset = 0;
        image_copy.bufferRowLength = 0;
        image_copy.bufferImageHeight = 0;
        image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_copy.imageSubresource.mipLevel = level;
        image_copy.imageSubresource.baseArrayLayer = 0;
        image_copy.imageSubresource.layerCount = 1;
        image_copy.imageOffset = {0, 0, 0};
        image_copy.imageExtent = {vkl_image_info.extent.width, vkl_image_info.extent.height, 1};
        vkCmdCopyBufferToImage(command_buffer, curr_buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
    }
    // it enables the shaders to access the image
    image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    g_vkCmdPipelineBarrier2KHR(command_buffer, &dependency_info);

    vkEndCommandBuffer(command_buffer);

    // create a fence
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0; // unsignaled
    VkFence vk_fence;
    vkCreateFence(vk_device, &fence_info, nullptr, &vk_fence);

    // submit the recorded cmd buffer to the queue
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    vkQueueSubmit(vk_queue, 1, &submit_info, vk_fence);
    vkWaitForFences(vk_device, 1, &vk_fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(vk_device, vk_fence, nullptr);
    vkFreeCommandBuffers(vk_device, command_pool, 1, &command_buffer);
    for (auto& buffer : staging_buffers) {
        vklDestroyHostCoherentBufferAndItsBackingMemory(buffer);
    }

    return texture;
}