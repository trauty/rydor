#pragma once

#include <vulkan/vulkan.h>

struct SDL_Window;

class rydor_app
{
public:
	void run();

private:
	SDL_Window* window;
	VkInstance instance;

	void init_window();
	void init_vulkan();
	void create_instance();
	void main_loop();
	void cleanup();
};