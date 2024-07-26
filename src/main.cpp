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