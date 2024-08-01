#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

#include "defines.h"
#include <optional>

struct SDL_Window;

struct queue_family_indices
{
	std::optional<u32> graphics_family;
	std::optional<u32> present_family;

	bool is_complete() const
	{
		return graphics_family.has_value() && present_family.has_value();
	};
};

struct swapchain_support_details
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

inline VkResult create_debug_utils_messenger_ext
(
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

inline void destroy_debug_utils_messenger_ext
(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debug_messenger,
	const VkAllocationCallbacks* p_allocator
)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, debug_messenger, p_allocator);
	}
}

class rydor_app
{
public:
	void run();

private:
	SDL_Window* window = nullptr;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue present_queue;
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchain_images;
	VkFormat swapchain_image_format;
	VkExtent2D swapchain_extent;

	const std::vector<const char*> validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
	std::vector<const char*> get_required_extensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback
	(
		VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
		VkDebugUtilsMessageTypeFlagsEXT msg_type,
		const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
		void* p_user_data
	);
	void populate_debug_messenger_info(VkDebugUtilsMessengerCreateInfoEXT& messenger_info);
	void setup_debug_messenger();
	void create_surface();
	void pick_physical_device();
	bool is_device_suitable(VkPhysicalDevice device);
	bool check_device_extension_support(VkPhysicalDevice device);
	queue_family_indices find_queue_families(VkPhysicalDevice device);
	void create_logical_device();
	swapchain_support_details query_swapchain_support(VkPhysicalDevice device);
	void create_swapchain();
	VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
	VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);
	void main_loop();
	void cleanup();
};