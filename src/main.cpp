#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif


#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

#include "rydor_app.h"

int main(int argc, char* argv[])
{
	rydor_app app;
	
	try
	{
		app.run();
	}
	catch (const std::exception& exc)
	{
		std::cerr << exc.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}