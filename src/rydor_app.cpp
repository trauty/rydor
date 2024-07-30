#include "rydor_app.h"
#include "defines.h"

#include <iostream>
#include <stdexcept>
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

	uint32_t extension_count = 0;
	SDL_bool lel = SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr);
	const char** extension_names = new const char* [extension_count];
	SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extension_names);

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = extension_names;
	create_info.enabledLayerCount = 0;

	if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
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
	SDL_Vulkan_UnloadLibrary();
	SDL_DestroyWindow(window);
	SDL_Quit();
	vkDestroyInstance(instance, nullptr);
}