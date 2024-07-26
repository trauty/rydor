#include "rydor_app.h"
#include "defines.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

void rydor_app::run()
{
	init_vulkan();
	main_loop();
	cleanup();
}

void rydor_app::init_window()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
	SDL_Vulkan_LoadLibrary(nullptr);
	window = SDL_CreateWindow("rydor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
}

void rydor_app::init_vulkan()
{
	create_instance();
}

void rydor_app::create_instance()
{
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "rydor";
	app_info.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	app_info.pEngineName = "rydor";
	app_info.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	u32 extension_count;
	const char** extension_names;
	SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr);
	extension_names = new const char* [extension_count];
	SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extension_names);

	VkInstanceCreateInfo create_info{};
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
	SDL_DestroyWindow(window);
	SDL_Vulkan_UnloadLibrary();
	SDL_Quit();
}