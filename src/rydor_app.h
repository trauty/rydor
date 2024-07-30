#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct SDL_Window;

class rydor_app
{
public:
	void run();

private:
	SDL_Window* window = nullptr;
	VkInstance instance;

	const std::vector<const char*> validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

#ifdef NDEBUG
	const bool enable_validation_layers = false;
#else
	const bool enable_validation_layers = true;
#endif


	void init_window();
	void init_vulkan();
	void create_instance();
	bool check_validation_layer_support();
	void main_loop();
	void cleanup();
};