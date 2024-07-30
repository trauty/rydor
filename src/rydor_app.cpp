#include "rydor_app.h"
#include "defines.h"

#include <iostream>
#include <stdexcept>
#include <optional>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_syswm.h>

void rydor_app::run()
{
	init_window();
	init_vulkan();
	main_loop();
	cleanup();
}

void rydor_app::init_window()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Vulkan_LoadLibrary(nullptr);
	window = SDL_CreateWindow("rydor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
}

void rydor_app::init_vulkan()
{
	create_instance();
	setup_debug_messenger();
	pick_physical_device();
}

void rydor_app::create_instance()
{
	if (enable_validation_layers && !check_validation_layer_support())
	{
		throw std::runtime_error("ERROR: Validation layers requested but none are available!");
	}

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "rydor";
	app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	app_info.pEngineName = "rydor";
	app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	const std::vector<const char*> sdl_extensions = get_required_extensions();
	instance_info.enabledExtensionCount = static_cast<u32>(sdl_extensions.size());
	instance_info.ppEnabledExtensionNames = sdl_extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info = {};
	if (enable_validation_layers)
	{
		instance_info.enabledLayerCount = static_cast<u32>(validation_layers.size());
		instance_info.ppEnabledLayerNames = validation_layers.data();

		populate_debug_messenger_info(debug_messenger_info);
		instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_messenger_info;
	}
	else
	{
		instance_info.enabledLayerCount = 0;
		instance_info.pNext = nullptr;
	}

	if (vkCreateInstance(&instance_info, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create a Vulkan instance!");
	}

	u32 extension_test_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_test_count, nullptr);
	std::vector<VkExtensionProperties> extensions(extension_test_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_test_count, extensions.data());

	std::cout << "Available extensions:\n";
	for (const VkExtensionProperties& extension : extensions)
	{
		std::cout << "\t" << extension.extensionName << "\n";
	}
}

bool rydor_app::check_validation_layer_support()
{
	u32 layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
	
	for (const char* layer_name : validation_layers)
	{
		bool layer_found = false;

		for (const VkLayerProperties& layer_properties : available_layers)
		{
			if (std::strcmp(layer_name, layer_properties.layerName) == 0)
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

std::vector<const char*> rydor_app::get_required_extensions()
{
	u32 sdl_extension_count = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, nullptr);
	const char** sdl_extensions = new const char* [sdl_extension_count];
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, sdl_extensions);

	std::vector<const char*> extensions(sdl_extensions, sdl_extensions + sdl_extension_count);

	if (enable_validation_layers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL rydor_app::debug_callback
(
	VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
	VkDebugUtilsMessageTypeFlagsEXT msg_type,
	const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
	void* p_user_data
)
{
	std::cerr << "Validation layer:" << p_callback_data->pMessage << "\n";
	return VK_FALSE;
}

void rydor_app::populate_debug_messenger_info(VkDebugUtilsMessengerCreateInfoEXT& messenger_info)
{
	messenger_info = {};
	messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messenger_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messenger_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messenger_info.pfnUserCallback = debug_callback;
	messenger_info.pUserData = nullptr;
}

void rydor_app::setup_debug_messenger()
{
	if (!enable_validation_layers) { return; }

	VkDebugUtilsMessengerCreateInfoEXT messenger_info;
	populate_debug_messenger_info(messenger_info);

	if (create_debug_utils_messenger_ext(instance, &messenger_info, nullptr, &debug_messenger) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to set up the debug messenger!");
	}
}

void rydor_app::pick_physical_device()
{
	u32 device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0)
	{
		throw std::runtime_error("No GPUs with Vulkan support detected!");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

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
		throw std::runtime_error("Selected GPU doesn't meet requirements!");
	}
}

bool rydor_app::is_device_suitable(VkPhysicalDevice device)
{
	queue_family_indices indices = find_queue_families(device);

	return indices.is_complete();
}

queue_family_indices rydor_app::find_queue_families(VkPhysicalDevice device)
{
	queue_family_indices indices = {};
	
	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	i32 i = 0;
	for (const VkQueueFamilyProperties& queue_family : queue_families)
	{
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics_family = i;
		}

		if (indices.is_complete()) { break; }

		i++;
	}

	return indices;
}

void rydor_app::main_loop()
{
	bool is_running = true;

	while (is_running)
	{
		SDL_Event window_event;
		while (SDL_PollEvent(&window_event))
		{
			if (window_event.type == SDL_QUIT)
			{
				is_running = false;
				break;
			}
		}
	}
}

void rydor_app::cleanup()
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