#include "rydor_app.h"
#include "defines.h"
#include "utils.h"

#include <iostream>
#include <stdexcept>
#include <optional>
#include <vector>
#include <set>
#include <limits>
#include <algorithm>
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
	create_surface();
	pick_physical_device();
	create_logical_device();
	create_swapchain();
	create_image_views();
	create_render_pass();
	create_graphics_pipeline();
}

void rydor_app::create_instance()
{
	if (enable_validation_layers && !check_validation_layer_support())
	{
		throw std::runtime_error("ERROR: Validation layers requested but none are available!");
	}

	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.pApplicationName = "rydor";
	app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	app_info.pEngineName = "rydor";
	app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instance_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
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
			if (strcmp(layer_name, layer_properties.layerName) == 0)
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
	messenger_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
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

void rydor_app::create_surface()
{
	if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE)
	{
		throw std::runtime_error("Failed to create window surface!");
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

	bool extensions_supported = check_device_extension_support(device);

	bool swapchain_adequate = false;
	if (extensions_supported)
	{
		swapchain_support_details swapchain_support = query_swapchain_support(device);
		swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
	}

	return indices.is_complete() && extensions_supported && swapchain_adequate;
}

bool rydor_app::check_device_extension_support(VkPhysicalDevice device)
{
	u32 extension_count = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	for (const VkExtensionProperties& extension : available_extensions)
	{
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
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

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

		if (present_support)
		{
			indices.present_family = i;
		}

		if (indices.is_complete()) { break; }

		i++;
	}

	return indices;
}

void rydor_app::create_logical_device()
{
	queue_family_indices indices = find_queue_families(physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<u32> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

	const f32 queue_priority = 1.0f;
	for (u32 queue_family : unique_queue_families)
	{
		VkDeviceQueueCreateInfo queue_create_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features = {};

	VkDeviceCreateInfo device_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	device_info.pQueueCreateInfos = queue_create_infos.data();
	device_info.queueCreateInfoCount = static_cast<u32>(queue_create_infos.size());
	device_info.pEnabledFeatures = &device_features;
	device_info.enabledExtensionCount = static_cast<u32>(device_extensions.size());
	device_info.ppEnabledExtensionNames = device_extensions.data();

	if (enable_validation_layers)
	{
		device_info.enabledLayerCount = static_cast<u32>(validation_layers.size());
		device_info.ppEnabledLayerNames = validation_layers.data();
	}
	else
	{
		device_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &device_info, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

void rydor_app::create_swapchain()
{
	swapchain_support_details swapchain_support = query_swapchain_support(physical_device);

	VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.formats);
	VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.present_modes);
	swapchain_extent = choose_swap_extent(swapchain_support.capabilities);
	swapchain_image_format = surface_format.format;

	u32 image_count = swapchain_support.capabilities.minImageCount + 1;

	if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
	{
		image_count = swapchain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchain_info.surface = surface;
	swapchain_info.minImageCount = image_count;
	swapchain_info.imageFormat = surface_format.format;
	swapchain_info.imageColorSpace = surface_format.colorSpace;
	swapchain_info.imageExtent = swapchain_extent;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	queue_family_indices indices = find_queue_families(physical_device);
	u32 queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

	if (indices.graphics_family != indices.present_family)
	{
		swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_info.queueFamilyIndexCount = 2;
		swapchain_info.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.queueFamilyIndexCount = 0;
		swapchain_info.pQueueFamilyIndices = nullptr;
	}

	swapchain_info.preTransform = swapchain_support.capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = present_mode;
	swapchain_info.clipped = VK_TRUE;
	swapchain_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swapchain!");
	}

	vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
	swapchain_images.resize(image_count);
	vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());
}

swapchain_support_details rydor_app::query_swapchain_support(VkPhysicalDevice device)
{
	swapchain_support_details details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	u32 format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

	if (format_count != 0)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
	}

	u32 present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}

VkSurfaceFormatKHR rydor_app::choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	for (const VkSurfaceFormatKHR& available_format : available_formats)
	{
		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return available_format;
		}
	}

	return available_formats[0];
}

VkPresentModeKHR rydor_app::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	for (const VkPresentModeKHR& available_present_mode : available_present_modes)
	{
		if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			return available_present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D rydor_app::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<u32>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		i32 width, height;
		SDL_Vulkan_GetDrawableSize(window, &width, &height);

		VkExtent2D actual_extent = {
			static_cast<u32>(width),
			static_cast<u32>(height)
		};

		actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actual_extent;
	}
}

void rydor_app::create_image_views()
{
	swapchain_image_views.resize(swapchain_images.size());

	for (size_t i = 0; i < swapchain_images.size(); i++)
	{
		VkImageViewCreateInfo image_view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		image_view_info.image = swapchain_images[i];
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = swapchain_image_format;
		image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &image_view_info, nullptr, &swapchain_image_views[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image views");
		}
	}
}

void rydor_app::create_render_pass()
{
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = swapchain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void rydor_app::create_graphics_pipeline()
{
	std::vector<char> vert_shader_code = utils::read_file("shaders/default.vert.spv");
	std::vector<char> frag_shader_code = utils::read_file("shaders/default.frag.spv");

	VkShaderModule vert_shader_module = create_shader_module(vert_shader_code);
	VkShaderModule frag_shader_module = create_shader_module(frag_shader_code);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vert_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	vert_shader_stage_info.module = frag_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

	VkPipelineVertexInputStateCreateInfo vertex_input_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertex_input_info.vertexBindingDescriptionCount = 0;
	vertex_input_info.pVertexBindingDescriptions = nullptr;
	vertex_input_info.vertexAttributeDescriptionCount = 0;
	vertex_input_info.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo input_assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (f32)swapchain_extent.width;
	viewport.height = (f32)swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchain_extent;

	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic_state.dynamicStateCount = static_cast<u32>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();

	VkPipelineViewportStateCreateInfo viewport_state = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling_info = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling_info.sampleShadingEnable = VK_FALSE;
	multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling_info.minSampleShading = 1.0f;
	multisampling_info.pSampleMask = nullptr;
	multisampling_info.alphaToCoverageEnable = VK_FALSE;
	multisampling_info.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipeline_layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_layout_info.setLayoutCount = 0;
	pipeline_layout_info.pSetLayouts = nullptr;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	vkDestroyShaderModule(device, vert_shader_module, nullptr);
	vkDestroyShaderModule(device, frag_shader_module, nullptr);
}

VkShaderModule rydor_app::create_shader_module(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo shader_module_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	shader_module_info.codeSize = code.size();
	shader_module_info.pCode = reinterpret_cast<const u32*>(code.data());

	VkShaderModule shader_module;
	if (vkCreateShaderModule(device, &shader_module_info, nullptr, &shader_module) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module!");
	}

	return shader_module;
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

	for (VkImageView image_view : swapchain_image_views)
	{
		vkDestroyImageView(device, image_view, nullptr);
	}

	vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	SDL_Vulkan_UnloadLibrary();
	SDL_DestroyWindow(window);
	SDL_Quit();
}