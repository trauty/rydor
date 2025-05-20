#include <iostream>
#include <vector>
#include <cstdlib>

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
//#include <cglm/cglm.h>

#include "defines.h"

const i32 WINDOW_WIDTH = 1280;
const i32 WINDOW_HEIGHT = 720;

const std::vector<const char*> validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

SDL_Window* window;
VkInstance instance;
VkDebugUtilsMessengerEXT debug_messenger;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback
(
	VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
	VkDebugUtilsMessageTypeFlagsEXT msg_type,
	const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
	void* p_user_data
)
{
	std::cerr << "Validation layer: " << p_callback_data->pMessage << "\n";
	return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(
	VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, 
	const VkAllocationCallbacks* p_allocator,
	VkDebugUtilsMessengerEXT* p_debug_messenger
)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, p_create_info, p_allocator, p_debug_messenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroy_debug_utils_messenger_ext(
	VkInstance instance, 
	VkDebugUtilsMessengerEXT debug_messenger,
	const VkAllocationCallbacks* p_allocator
)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debug_messenger, p_allocator);
	}
}

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& messenger_create_info)
{
	messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messenger_create_info.pfnUserCallback = debug_callback;
	messenger_create_info.pUserData = nullptr;
}

void setup_debug_messenger()
{
	if (!enable_validation_layers) { return; }

	VkDebugUtilsMessengerCreateInfoEXT messenger_create_info {};
	populate_debug_messenger_create_info(messenger_create_info);

	if (create_debug_utils_messenger_ext(instance, &messenger_create_info, nullptr, &debug_messenger))
	{
		return;
	}
}

std::vector<const char*> get_required_extensions()
{
	u32 sdl_extension_count;
	const char* const* sdl_extensions;
	sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);

	std::vector<const char*> extensions(sdl_extensions, sdl_extensions + sdl_extension_count);

	if (enable_validation_layers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool check_validation_layer_support()
{
	u32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : validation_layers)
	{
		bool layer_found = false;

		for (const VkLayerProperties& layer_props : available_layers)
		{
			if (strcmp(layer_name, layer_props.layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
		{
			return false;
		}
	}

	return true;
}

bool create_instance()
{
	if (enable_validation_layers && !check_validation_layer_support())
	{
		std::cout << "Validation layers were requested, but are not available!\n";
		return false;
	}

	constexpr VkApplicationInfo app_info 
	{ 
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "rydor",
		.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.pEngineName = "rydor",
		.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};

	VkInstanceCreateInfo instance_create_info 
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info
	};

	std::vector<const char*> extensions = get_required_extensions();

	instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instance_create_info.ppEnabledExtensionNames = extensions.data();
	
	if (enable_validation_layers)
	{
		instance_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		instance_create_info.ppEnabledLayerNames = validation_layers.data();

		VkDebugUtilsMessengerCreateInfoEXT debug_create_info {};
		populate_debug_messenger_create_info(debug_create_info);

		instance_create_info.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debug_create_info);
	}
	else
	{
		instance_create_info.enabledLayerCount = 0;
		instance_create_info.pNext = nullptr;
	}


	if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS)
	{
		std::cout << "Could not create Vulkan instance!\n";
		return false;
	}

	u32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> extension_props(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extension_props.data());

	std::cout << "Available extensions:\n";
	for (const VkExtensionProperties& extension : extension_props)
	{
		std::cout << '\t' << extension.extensionName << '\n';
	}

	return true;
}

bool is_device_suitable(const VkPhysicalDevice& device)
{
	VkPhysicalDeviceProperties device_props {};
	VkPhysicalDeviceFeatures device_features {};
	vkGetPhysicalDeviceProperties(device, &device_props);
	vkGetPhysicalDeviceFeatures(device, &device_features);

	return device_props.deviceType == device_features.geometryShader; //VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
}

void pick_physical_device()
{
	u32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0)
	{
		std::cout << "No GPUs with Vulkan support found!\n";
		return;
	}

	VkPhysicalDevice devices[device_count];
	vkEnumeratePhysicalDevices(instance, &device_count, devices);

	for (const VkPhysicalDevice& device : devices)
	{
		if (is_device_suitable(device))
		{
			physical_device = device;
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE)
	{
		std::cout << "No GPUs with appropriate Vulkan support found!\n";
		return;
	}
}

void init_window()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Vulkan_LoadLibrary(nullptr);
	window = SDL_CreateWindow("rydor", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void init_vulkan()
{
	create_instance();
	setup_debug_messenger();
	pick_physical_device();
}

void main_loop()
{
	bool is_running = true;

	while (is_running)
	{
		static SDL_Event window_event;
		while (SDL_PollEvent(&window_event))
		{
			if (window_event.type == SDL_EVENT_QUIT)
			{
				is_running = false;
				break;
			}
		}
	}
}

void cleanup()
{
	if (enable_validation_layers)
	{
		destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);
	}

	vkDestroyInstance(instance, nullptr);

	SDL_Vulkan_UnloadLibrary();
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void run()
{
	init_window();
	init_vulkan();
	main_loop();
	cleanup();
}

int main(int argc, char* argv[])
{
	run();

	return EXIT_SUCCESS;
}