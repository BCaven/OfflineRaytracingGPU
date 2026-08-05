#pragma once

#include "VulkanHelper.h"
#include "Utility.h"
#include "KeyInputs.h"

constexpr uint32_t maxFramesInFlight{ 2 };
constexpr uint32_t numHistoryFrames{ 2 };
constexpr uint32_t objectTypes{ 2 };
constexpr glm::vec3 WORLD_UP{ 0, 1, 0 };
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
const bool enableValidationLayers = true;

class VK_Wrap
{
	uint32_t imageIndex{ 0 };
	uint32_t frameIndex{ 0 };
	VkInstance instance{ VK_NULL_HANDLE };
	VkDevice device{ VK_NULL_HANDLE };
	VkQueue queue{ VK_NULL_HANDLE };
	VkSurfaceKHR surface{ VK_NULL_HANDLE };
	bool updateSwapchain{ false };
	VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
	VkCommandPool commandPool{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkImage depthImage;
	VmaAllocator allocator{ VK_NULL_HANDLE };
	VmaAllocation depthImageAllocation;
	VkImageView depthImageView;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::array<VkCommandBuffer, maxFramesInFlight> commandBuffers;
	std::array<VkFence, maxFramesInFlight> fences;
	std::array<VkSemaphore, maxFramesInFlight> imageAcquiredSemaphores;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	VmaAllocation vBufferAllocation{ VK_NULL_HANDLE };
	uint32_t queueFamily{ 0 };

	VkBuffer vBuffer{ VK_NULL_HANDLE };
	VkDeviceSize vBufSize{};


	VkShaderModule shaderModule{};
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};


	std::array<ShaderDataBuffer, maxFramesInFlight> shaderDataBuffers;
	std::array<Texture, 3> textures{};
	Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
	glm::ivec2 windowSize{};

	VkDeviceSize indexCount{};

	SDL_Window* window = nullptr;
	uint32_t deviceCount{ 0 };
	uint32_t deviceIndex{ 0 };
	std::vector<VkPhysicalDevice> devices;

	VkSurfaceCapabilitiesKHR surfaceCaps{};
	VkSwapchainCreateInfoKHR swapchainCI{};
	uint32_t imageCount{ 0 };

	VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	const VkFormat imageFormat{ VK_FORMAT_B8G8R8A8_SRGB };
	VkFormat depthFormat{ VK_FORMAT_UNDEFINED };


	VkImageCreateInfo depthImageCI{};

	std::vector<VkDescriptorImageInfo> textureDescriptors{};


	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;



	// Frame history
	std::array<VkImage, numHistoryFrames> historyImages{};
	std::array<VmaAllocation, numHistoryFrames> historyImageAllocations{};
	std::array<VkImageView, numHistoryFrames> historyImageViews{};
	std::array<VkImageLayout, numHistoryFrames> historyImageLayouts{
		VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkSampler historySampler{ VK_NULL_HANDLE };
	uint32_t historyReadIndex{ 0 };
	const VkFormat historyFormat{ VK_FORMAT_R32G32B32A32_SFLOAT };

	std::array<VkDescriptorSet, maxFramesInFlight> descriptorSets;


	// bindings
	std::vector<VkDescriptorSetLayoutBinding> setBindings;

	std::unordered_map<std::string, ResourceBinding> bindings;

	std::unordered_map<std::string, StructuredBufferBinding> structuredBufferBindings{};

	KeyInputs& inputs = KeyInputs::inputHandler();

	uint64_t lastTime{ SDL_GetTicks() };
	bool quit{ false };

	inline void chkSwapchain(VkResult result) 
	{
		if (result < VK_SUCCESS) {
			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				updateSwapchain = true;
				return;
			}
			std::cerr << "Vulkan call returned an error (" << result << ")\n";
			exit(result);
		}
	}

	inline void beginSingleTimeCommands(VkCommandBuffer commandBuffer, VkFence fence) {
		vkResetFences(device, 1, &fence);
		vkResetCommandBuffer(commandBuffer, 0);
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
	}

	inline void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkFence fence) {
		vkEndCommandBuffer(commandBuffer);
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vkQueueSubmit(queue, 1, &submitInfo, fence);
		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	}

public:

	std::vector<Sphere> spheres;
	std::vector<Material> materials;
	CameraWrapper camera;
	ShaderData shaderData{};




	VK_Wrap() {};
	~VK_Wrap() 
	{
		// Tear down
		chk(vkDeviceWaitIdle(device));

		for (auto& [name, binding] : structuredBufferBindings)
		{
			vmaDestroyBuffer(allocator, binding.stagingBuffer, binding.stagingAllocation);
			vmaDestroyBuffer(allocator, binding.buffer, binding.bufferAllocation);
		}		

		destroyHistoryImages();

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vmaDestroyBuffer(allocator, vBuffer, vBufferAllocation);

		for (auto i = 0; i < maxFramesInFlight; i++) {
			vkDestroyFence(device, fences[i], nullptr);
			vkDestroySemaphore(device, imageAcquiredSemaphores[i], nullptr);
			vmaDestroyBuffer(allocator, shaderDataBuffers[i].buffer, shaderDataBuffers[i].allocation);
		}
		for (auto i = 0; i < renderCompleteSemaphores.size(); i++) {
			vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
		}
		vmaDestroyImage(allocator, depthImage, depthImageAllocation);
		vkDestroyImageView(device, depthImageView, nullptr);
		for (auto i = 0; i < swapchainImageViews.size(); i++) {
			vkDestroyImageView(device, swapchainImageViews[i], nullptr);
		}
		
		//vkDestroyDescriptorSetLayout(device, descriptorSetLayoutTex, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroySwapchainKHR(device, swapchain, nullptr);

		vkDestroyCommandPool(device, commandPool, nullptr);
		vkDestroyShaderModule(device, shaderModule, nullptr);
		vmaDestroyAllocator(allocator);

		vkDestroyDevice(device, nullptr);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		SDL_DestroyWindow(window);
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_Quit();
		vkDestroyInstance(instance, nullptr);
	};

	void init()
	{
		initVulkan();

		initVMA();

		initSwapchain();
		
		
		// Shader data buffers
		for (auto i = 0; i < maxFramesInFlight; i++) {
			VkBufferCreateInfo uBufferCI{ 
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 
				.size = sizeof(ShaderData), 
				.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 
			};
			VmaAllocationCreateInfo uBufferAllocCI{ 
				.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
				VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | 
				VMA_ALLOCATION_CREATE_MAPPED_BIT, 
				.usage = VMA_MEMORY_USAGE_AUTO 
			};
			chk(vmaCreateBuffer(
				allocator, 
				&uBufferCI, 
				&uBufferAllocCI, 
				&shaderDataBuffers[i].buffer, 
				&shaderDataBuffers[i].allocation, 
				&shaderDataBuffers[i].allocationInfo
			));
			VkBufferDeviceAddressInfo uBufferBdaInfo{ 
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, 
				.buffer = shaderDataBuffers[i].buffer 
			};
			shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(device, &uBufferBdaInfo);
		}

		// Sync objects
		
		VkFenceCreateInfo fenceCI{ 
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 
			.flags = VK_FENCE_CREATE_SIGNALED_BIT 
		};
		for (auto i = 0; i < maxFramesInFlight; i++) {
			chk(vkCreateFence(device, &fenceCI, nullptr, &fences[i]));
			chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &imageAcquiredSemaphores[i]));
		}
		renderCompleteSemaphores.resize(swapchainImages.size());
		for (auto& semaphore : renderCompleteSemaphores) {
			chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
		}
		// Command pool
		VkCommandPoolCreateInfo commandPoolCI{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, 
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, 
			.queueFamilyIndex = queueFamily 
		};
		chk(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));
		VkCommandBufferAllocateInfo cbAllocCI{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 
			.commandPool = commandPool, 
			.commandBufferCount = maxFramesInFlight 
		};
		chk(vkAllocateCommandBuffers(device, &cbAllocCI, commandBuffers.data()));


		initShaderCompiler();

		loadStructuredBuffer("spheres", spheres);

		loadStructuredBuffer("materials", materials);

		initHistoryImages();

		initBindings();

		initPipeline();

		updateStructuredBufferDescriptors("spheres");

		updateStructuredBufferDescriptors("materials");


		for (auto& [name, b] : bindings)
		{
			std::cout << name << " binding " << b.binding << " set " << b.set << '\n';
		}

		initVertices();

		initShaderData();

		
	}

	void initVulkan()
	{
		// Make sure asset folder is present from the current working directory
		if (!std::filesystem::is_directory("assets")) {
			std::cerr << "Could not locate assets folder from current working directory\n";
			exit(-1);
		}
		chk(SDL_Init(SDL_INIT_VIDEO));
		chk(SDL_Vulkan_LoadLibrary(NULL));
		volkInitialize();
		// Instance
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "GPU Monte Carlo Raytracing",
			.apiVersion = VK_API_VERSION_1_3
		};
		uint32_t instanceExtensionsCount{ 0 };
		char const* const* instanceExtensions{ SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount) };
		VkInstanceCreateInfo instanceCI{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = instanceExtensionsCount,
			.ppEnabledExtensionNames = instanceExtensions,
		};
		if (enableValidationLayers)
		{
			instanceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceCI.ppEnabledLayerNames = validationLayers.data();
		}
		chk(vkCreateInstance(&instanceCI, nullptr, &instance));
		volkLoadInstance(instance);
		// Device
		chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
		std::cout << "Found " << deviceCount << " Devices\n";
		devices.resize(deviceCount);
		chk(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

		VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
		std::cout << "Selected device: " << deviceProperties.properties.deviceName << "\n";
		// Find a queue family for graphics
		uint32_t queueFamilyCount{ 0 };
		vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, queueFamilies.data());
		for (size_t i = 0; i < queueFamilies.size(); i++) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				queueFamily = i;
				break;
			}
		}
		chk(SDL_Vulkan_GetPresentationSupport(instance, devices[deviceIndex], queueFamily));
		// Logical device
		const float qfpriorities{ 1.0f };
		VkDeviceQueueCreateInfo queueCI{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = queueFamily, .queueCount = 1, .pQueuePriorities = &qfpriorities };
		VkPhysicalDeviceVulkan12Features enabledVk12Features{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			.descriptorIndexing = true,
			.shaderSampledImageArrayNonUniformIndexing = true,
			.descriptorBindingVariableDescriptorCount = true,
			.runtimeDescriptorArray = true,
			.scalarBlockLayout = true,
			.bufferDeviceAddress = true 
		};
		VkPhysicalDeviceVulkan13Features enabledVk13Features{ 
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, 
			.pNext = &enabledVk12Features, 
			.synchronization2 = true, 
			.dynamicRendering = true 
		};
		VkPhysicalDeviceFeatures enabledVk10Features{ .samplerAnisotropy = VK_TRUE };
		const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkDeviceCreateInfo deviceCI{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &enabledVk13Features,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &queueCI,
			.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &enabledVk10Features
		};
		chk(vkCreateDevice(devices[deviceIndex], &deviceCI, nullptr, &device));
		vkGetDeviceQueue(device, queueFamily, 0, &queue);

	}

	void initVMA()
	{
		// VMA
		VmaVulkanFunctions vkFunctions{ .vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr, .vkCreateImage = vkCreateImage };
		VmaAllocatorCreateInfo allocatorCI{ .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT, .physicalDevice = devices[deviceIndex], .device = device, .pVulkanFunctions = &vkFunctions, .instance = instance };
		chk(vmaCreateAllocator(&allocatorCI, &allocator));
	}
	
	void initSwapchain()
	{
		// Window and surface
		window = SDL_CreateWindow("GPU Monte Carlo Raytracing", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		assert(window);
		chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));
		chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
		// relative mouse mode
		chk(SDL_SetWindowRelativeMouseMode(window, true));
		chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface, &surfaceCaps));
		VkExtent2D swapchainExtent{ surfaceCaps.currentExtent };
		if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
			swapchainExtent = { .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) };
		}

		// Swap chain
		swapchainCI = VkSwapchainCreateInfoKHR{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = surface,
			.minImageCount = surfaceCaps.minImageCount,
			.imageFormat = imageFormat,
			.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
			.imageExtent{.width = swapchainExtent.width, .height = swapchainExtent.height },
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR
		};
		chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));
		chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
		swapchainImages.resize(imageCount);
		chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
		swapchainImageViews.resize(imageCount);
		for (auto i = 0; i < imageCount; i++) {
			VkImageViewCreateInfo viewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = swapchainImages[i], .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = imageFormat, .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
			chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
		}
		// Depth attachment
		std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		for (VkFormat& format : depthFormatList) {
			VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
			vkGetPhysicalDeviceFormatProperties2(devices[deviceIndex], format, &formatProperties);
			if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				depthFormat = format;
				break;
			}
		}
		assert(depthFormat != VK_FORMAT_UNDEFINED);
		depthImageCI = VkImageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = depthFormat,
			.extent{.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y), .depth = 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		VmaAllocationCreateInfo allocCI{ .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, .usage = VMA_MEMORY_USAGE_AUTO };
		chk(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr));
		VkImageViewCreateInfo depthViewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = depthImage, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = depthFormat, .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 } };
		chk(vkCreateImageView(device, &depthViewCI, nullptr, &depthImageView));

	}

	void initShaderData()
	{
		shaderData.windowMax = windowSize;
		shaderData.numSpheres = spheres.size();
		shaderData.frameCount = 0;
	}

	void initHistoryImages()
	{
		VkImageCreateInfo historyImageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = historyFormat,
			.extent{
				.width = static_cast<uint32_t>(windowSize.x),
				.height = static_cast<uint32_t>(windowSize.y),
				.depth = 1
			},
			.mipLevels = 1, .arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		VmaAllocationCreateInfo allocCI{
			.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO
		};
		for (auto i = 0; i < numHistoryFrames; i++)
		{
			chk(vmaCreateImage(allocator, &historyImageCI, &allocCI, &historyImages[i], &historyImageAllocations[i], nullptr));
			VkImageViewCreateInfo viewCI{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = historyImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = historyFormat,
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			};
			chk(vkCreateImageView(device, &viewCI, nullptr, &historyImageViews[i]));
		}
		VkSamplerCreateInfo samplerCI{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.maxLod = 1.0f,
		};
		chk(vkCreateSampler(device, &samplerCI, nullptr, &historySampler));

		// bindings:
		setBindings.push_back(
			VkDescriptorSetLayoutBinding{
				.binding = bindings["previousFrame"].binding,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			}
		);

	}

	void updateHistoryDescriptor()
	{
		VkDescriptorImageInfo historyInfo{
			.sampler = historySampler,
			.imageView = historyImageViews[historyReadIndex],
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		VkWriteDescriptorSet write{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[frameIndex],
			.dstBinding = bindings["previousFrame"].binding,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &historyInfo
		};
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	void destroyHistoryImages()
	{
		// destroy history buffers
		for (auto i = 0; i < historyImages.size(); i++)
		{
			auto historyImage = historyImages[i];
			auto historyImageAllocation = historyImageAllocations[i];
			vmaDestroyImage(allocator, historyImage, historyImageAllocation);
			vkDestroyImageView(device, historyImageViews[i], nullptr);
		}
		vkDestroySampler(device, historySampler, nullptr);
	}

	void initVertices()
	{
		VkVertexInputBindingDescription vertexBinding{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
		std::vector<VkVertexInputAttributeDescription> vertexAttributes{
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT
			}
		};
		VkPipelineVertexInputStateCreateInfo vertexInputState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vertexBinding,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
			.pVertexAttributeDescriptions = vertexAttributes.data(),
		};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
		};
		std::vector<VkDynamicState> dynamicStates{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = 2,
			.pDynamicStates = dynamicStates.data()
		};
		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1
		};
		VkPipelineRasterizationStateCreateInfo rasterizationState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.lineWidth = 1.0f
		};
		VkPipelineMultisampleStateCreateInfo multisampleState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
		};
		VkPipelineDepthStencilStateCreateInfo depthStencilState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
		};
		VkPipelineColorBlendAttachmentState blendAttachment{
			.colorWriteMask = 0xF
		};
		VkPipelineColorBlendStateCreateInfo colorBlendState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blendAttachment
		};
		VkPipelineRenderingCreateInfo renderingCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &historyFormat,
			.depthAttachmentFormat = depthFormat
		};
		VkGraphicsPipelineCreateInfo pipelineCI{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &renderingCI,
			.stageCount = 2,
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInputState,
			.pInputAssemblyState = &inputAssemblyState,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizationState,
			.pMultisampleState = &multisampleState,
			.pDepthStencilState = &depthStencilState,
			.pColorBlendState = &colorBlendState,
			.pDynamicState = &dynamicState,
			.layout = pipelineLayout
		};
		chk(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

		// The "screen" that rays will get shot out of in the fragment shader
		// Since we arent using any of the intrinsics
		std::vector<Vertex> vertices
		{
			Vertex
			{
				.pos = glm::vec3(1.0, 1.0, 0.0)
			},
			Vertex
			{
				.pos = glm::vec3(-1.0, -1.0, 0.0)
			},
			Vertex
			{
				.pos = glm::vec3(1.0, -1.0, 0.0)
			},
			Vertex
			{
				.pos = glm::vec3(1.0, 1.0, 0.0)
			},
			Vertex
			{
				.pos = glm::vec3(-1.0, -1.0, 0.0)
			},
			Vertex
			{
				.pos = glm::vec3(-1.0, 1.0, 0.0)
			}
		};
		std::vector<uint16_t> indices
		{
			0, 1, 2, 3, 4, 5
		};
		indexCount = indices.size();

		// Load full screen quad :)
		vBufSize = VkDeviceSize{ sizeof(Vertex) * vertices.size() };
		VkDeviceSize iBufSize{ sizeof(uint16_t) * indices.size() };
		VkBufferCreateInfo bufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = vBufSize + iBufSize,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		};
		VmaAllocationCreateInfo vBufferAllocCI{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO
		};
		VmaAllocationInfo vBufferAllocInfo{};

		chk(vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &vBuffer, &vBufferAllocation, &vBufferAllocInfo));
		std::memcpy(vBufferAllocInfo.pMappedData, vertices.data(), vBufSize);
		std::memcpy(((char*)vBufferAllocInfo.pMappedData) + vBufSize, indices.data(), iBufSize);
	}

	void initShaderCompiler()
	{
		// Initialize Slang shader compiler
		slang::createGlobalSession(slangGlobalSession.writeRef());
		auto slangTargets{
			std::to_array<slang::TargetDesc>({
				{
					.format{SLANG_SPIRV},
					.profile{slangGlobalSession->findProfile("spirv_1_4")},
					.forceGLSLScalarBufferLayout = true,
				}
				})
		};
		auto slangOptions{
			std::to_array<slang::CompilerOptionEntry>({ { slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1} } })
		};
		slang::SessionDesc slangSessionDesc{
			.targets{slangTargets.data()},
			.targetCount{SlangInt(slangTargets.size())},
			.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
			.compilerOptionEntries{slangOptions.data()},
			.compilerOptionEntryCount{uint32_t(slangOptions.size())}
		};
		// Load shader
		Slang::ComPtr<slang::ISession> slangSession;
		slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());
		Slang::ComPtr<slang::IModule> slangModule{
			slangSession->loadModuleFromSource("triangle", "assets/shader.slang", nullptr, nullptr)
		};

		// get reflection indices
		Slang::ComPtr<ISlangBlob> reflectionDiagnostics;
		slang::ProgramLayout* programLayout = slangModule->getLayout(0, reflectionDiagnostics.writeRef());
		if (reflectionDiagnostics && reflectionDiagnostics->getBufferSize() > 0)
		{

		};

		unsigned paramCount = programLayout->getParameterCount();
		for (unsigned i = 0; i < paramCount; i++)
		{
			slang::VariableLayoutReflection* param = programLayout->getParameterByIndex(i);
			std::string name = param->getName();
			bindings[name] = {
				.binding = param->getBindingIndex(),
				.set = param->getBindingSpace()
			};
		}


		Slang::ComPtr<ISlangBlob> spirv;
		slangModule->getTargetCode(0, spirv.writeRef());
		VkShaderModuleCreateInfo shaderModuleCI{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = spirv->getBufferSize(),
			.pCode = (uint32_t*)spirv->getBufferPointer()
		};
		chk(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));

	}

	void initPipeline()
	{
		// Pipeline
		VkPushConstantRange pushConstantRange{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.size = sizeof(VkDeviceAddress)
		};
		VkPipelineLayoutCreateInfo pipelineLayoutCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange
		};
		chk(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));
		shaderStages = std::vector<VkPipelineShaderStageCreateInfo>{
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = shaderModule,
				.pName = "main"
			},
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = shaderModule,
				.pName = "main"
			}
		};

	}

	void initBindings()
	{
		
		VkDescriptorSetLayoutCreateInfo sphereSetLayoutCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = static_cast<uint32_t>(setBindings.size()),
			.pBindings = setBindings.data()
		};
		chk(vkCreateDescriptorSetLayout(device, &sphereSetLayoutCI, nullptr, &descriptorSetLayout));


		std::array<VkDescriptorPoolSize, objectTypes + 1> poolSizes{
			VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = maxFramesInFlight
			},
			VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = maxFramesInFlight
			},
			VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = maxFramesInFlight
			},
		};
		VkDescriptorPoolCreateInfo poolCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = maxFramesInFlight,
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data()
		};
		chk(vkCreateDescriptorPool(device, &poolCI, nullptr, &descriptorPool));

		std::array<VkDescriptorSetLayout, maxFramesInFlight> layouts;
		layouts.fill(descriptorSetLayout);

		VkDescriptorSetAllocateInfo sphereSetAllocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = maxFramesInFlight,
			.pSetLayouts = layouts.data()
		};
		chk(vkAllocateDescriptorSets(device, &sphereSetAllocInfo, descriptorSets.data()));
	}

	template<typename T>
	void loadStructuredBuffer(std::string bindingName, std::vector<T> data)
	{
		// check if a binding exists, if not return
		if (structuredBufferBindings.find(bindingName) != structuredBufferBindings.end())
		{
			std::cout << "Bindings already exist for " << bindingName << "\n";
			return;
		}
		if (bindings.find(bindingName) == bindings.end())
		{
			std::cout << "Binding does not exist in reflection for " << bindingName << "\n";
			return;
		}
		
		auto& binding = structuredBufferBindings[bindingName];

		// Step 1: device-local buffer

		binding.bufferInfo = VkBufferCreateInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(T) * data.size(),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
		VmaAllocationCreateInfo allocCreateInfo{
			.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO,
		};
		chk(vmaCreateBuffer(allocator, &binding.bufferInfo, &allocCreateInfo, &binding.buffer, &binding.bufferAllocation, nullptr));

		// step 2: Staging buffer
		VkBufferCreateInfo stagingInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = binding.bufferInfo.size,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
		VmaAllocationCreateInfo stagingCreateInfo{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO,
		};

		chk(vmaCreateBuffer(allocator, &stagingInfo, &stagingCreateInfo,
			&binding.stagingBuffer, &binding.stagingAllocation, &binding.stagingAllocInfo));

		memcpy(binding.stagingAllocInfo.pMappedData, data.data(), (size_t)binding.bufferInfo.size);

		VkCommandBufferAllocateInfo oneTimeAllocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};
		VkCommandBuffer cbOneTime;
		chk(vkAllocateCommandBuffers(device, &oneTimeAllocInfo, &cbOneTime));

		VkFenceCreateInfo fenceOneTimeCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		VkFence fenceOneTime{};
		chk(vkCreateFence(device, &fenceOneTimeCI, nullptr, &fenceOneTime));
		beginSingleTimeCommands(cbOneTime, fenceOneTime);
		VkBufferCopy copyRegion{
			.size = binding.bufferInfo.size
		};
		vkCmdCopyBuffer(cbOneTime, binding.stagingBuffer, binding.buffer, 1, &copyRegion);
		VkBufferMemoryBarrier2 barrier{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
			.buffer = binding.buffer,
			.size = VK_WHOLE_SIZE
		};
		VkDependencyInfo barrierInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &barrier
		};

		vkCmdPipelineBarrier2(cbOneTime, &barrierInfo);
		endSingleTimeCommands(cbOneTime, fenceOneTime);

		// step 3: descriptor set layout using reflected binding index
		setBindings.push_back(
			VkDescriptorSetLayoutBinding{
				.binding = bindings[bindingName].binding,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
			}
			);

		vkDestroyFence(device, fenceOneTime, nullptr);
		vkFreeCommandBuffers(device, commandPool, 1, &cbOneTime);
	}

	void updateStructuredBufferDescriptors(std::string bindingName)
	{
		// TODO: could have a function that updates all descriptors

		// check if a binding exists, if not return
		if (structuredBufferBindings.find(bindingName) == structuredBufferBindings.end())
		{
			std::cout << "Bindings do not exist for " << bindingName << "\n";
			return;
		}
		if (bindings.find(bindingName) == bindings.end())
		{
			std::cout << "Binding does not exist in reflection for " << bindingName << "\n";
			return;
		}

		auto& binding = structuredBufferBindings[bindingName];

		VkDescriptorBufferInfo descBufInfo{
			.buffer = binding.buffer, //sphereBuffer,
			.range = VK_WHOLE_SIZE
		};
		for (auto i = 0; i < maxFramesInFlight; i++)
		{
			VkWriteDescriptorSet write{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = descriptorSets[i],
				.dstBinding = bindings[bindingName].binding,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &descBufInfo
			};
			vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

		}
	}

	bool loadObj(std::string filepath)
	{
		// TODO: separate mesh data :)
		// Mesh data
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, filepath.c_str()));
		indexCount = VkDeviceSize{ shapes[0].mesh.indices.size() };
		std::vector<Vertex> vertices{};
		std::vector<uint16_t> indices{};
		// Load vertex and index data
		for (auto& index : shapes[0].mesh.indices) {
			Vertex v{
				.pos = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2] },
				//.normal = { attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] },
				//.uv = { attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1] }
			};
			vertices.push_back(v);
			indices.push_back(indices.size());
		}
		//vBufSize = VkDeviceSize{ sizeof(Vertex) * vertices.size() };
		//VkDeviceSize iBufSize{ sizeof(uint16_t) * indices.size() };
		//VkBufferCreateInfo bufferCI{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = vBufSize + iBufSize, .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT };
		//VmaAllocationCreateInfo vBufferAllocCI{ .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, .usage = VMA_MEMORY_USAGE_AUTO };
		//VmaAllocationInfo vBufferAllocInfo{};

		/*
		Future plan: throw these in an array that gets passed to the GPU, since we dont actually want to use any of these vertices
		for the vert/frag shaders (since we are doing jank raytracing instead :)
		*/

		//chk(vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &vBuffer, &vBufferAllocation, &vBufferAllocInfo));
		//memcpy(vBufferAllocInfo.pMappedData, vertices.data(), vBufSize);
		//memcpy(((char*)vBufferAllocInfo.pMappedData) + vBufSize, indices.data(), iBufSize);
		return true;
	}

	bool draw()
	{
		// Sync
		chk(vkWaitForFences(device, 1, &fences[frameIndex], true, UINT64_MAX));
		chk(vkResetFences(device, 1, &fences[frameIndex]));
		chkSwapchain(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex));
		
		shaderData.camera.rotationMatrix = glm::mat3x3(
			camera.right,
			camera.up,
			camera.direction
		);
		shaderData.camera.origin = camera.origin;
		shaderData.camera.fov = camera.fov;
		shaderData.frameCount += 1;
		memcpy(shaderDataBuffers[frameIndex].allocationInfo.pMappedData, &shaderData, sizeof(ShaderData));

		// historyReadIndex is either 0 or 1, so this is either 1 or 0
		// TODO: better system that can handle more frames (irrelevant for this specific task)
		int historyWriteIndex = 1 - historyReadIndex;
		updateHistoryDescriptor();
		// also build the two barriers
		std::array<VkImageMemoryBarrier2, 2> historyBarriers{
			VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				.oldLayout = historyImageLayouts[historyWriteIndex],
				.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
				.image = historyImages[historyWriteIndex],
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			},
			VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
				.oldLayout = historyImageLayouts[historyReadIndex],
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.image = historyImages[historyReadIndex],
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			}
		};

		// Build command buffer
		auto cb = commandBuffers[frameIndex];
		chk(vkResetCommandBuffer(cb, 0));
		VkCommandBufferBeginInfo cbBI{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT 
		};
		chk(vkBeginCommandBuffer(cb, &cbBI));

		// history images
		VkDependencyInfo historyBarrierDI{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, 
			.imageMemoryBarrierCount = 2, 
			.pImageMemoryBarriers = historyBarriers.data()
		};
		vkCmdPipelineBarrier2(cb, &historyBarrierDI);
		historyImageLayouts[historyWriteIndex] = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		historyImageLayouts[historyReadIndex] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// swapchain and depth
		std::array<VkImageMemoryBarrier2, 1> outputBarriers{
			VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
				.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
				.image = depthImage,
				.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 }
			}
		};
		VkDependencyInfo barrierDependencyInfo{ 
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, 
			.imageMemoryBarrierCount = static_cast<uint32_t>(outputBarriers.size()), 
			.pImageMemoryBarriers = outputBarriers.data()
		};
		vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);
		VkRenderingAttachmentInfo colorAttachmentInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = historyImageViews[historyWriteIndex],
			.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue{.color{ 0.0f, 0.0f, 0.0f, 1.0f }}
		};
		VkRenderingAttachmentInfo depthAttachmentInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = depthImageView,
			.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.clearValue = {.depthStencil = {1.0f,  0}}
		};
		VkRenderingInfo renderingInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea{.extent{.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) }},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentInfo,
			.pDepthAttachment = &depthAttachmentInfo
		};
		vkCmdBeginRendering(cb, &renderingInfo);
		VkViewport vp{ 
			.width = static_cast<float>(windowSize.x), .height = static_cast<float>(windowSize.y), .minDepth = 0.0f, .maxDepth = 1.0f 
		};
		vkCmdSetViewport(cb, 0, 1, &vp);
		VkRect2D scissor{ 
			.extent{.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) } 
		};
		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdSetScissor(cb, 0, 1, &scissor);
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
		VkDeviceSize vOffset{ 0 };
		vkCmdBindVertexBuffers(cb, 0, 1, &vBuffer, &vOffset);
		vkCmdBindIndexBuffer(cb, vBuffer, vBufSize, VK_INDEX_TYPE_UINT16);
		vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &shaderDataBuffers[frameIndex].deviceAddress);
		vkCmdDrawIndexed(cb, indexCount, 1, 0, 0, 0);
		vkCmdEndRendering(cb);

		// blit image
		std::array<VkImageMemoryBarrier2, 2> blitSrcDstBarriers{
			VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.image = historyImages[historyWriteIndex],
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			},
			VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				.srcAccessMask = 0,
				.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.image = swapchainImages[imageIndex],
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1,

				}
			}
		};
		VkDependencyInfo blitSrcDstDI{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 2,
			.pImageMemoryBarriers = blitSrcDstBarriers.data()
		};
		vkCmdPipelineBarrier2(cb, &blitSrcDstDI);

		VkImageBlit blitRegion{
			.srcSubresource{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1
			},
			.srcOffsets = {{0, 0, 0}, {windowSize.x, windowSize.y, 1}},
			.dstSubresource{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1
			},
			.dstOffsets = {{0, 0, 0}, {windowSize.x, windowSize.y, 1}}
		};
		vkCmdBlitImage(cb,
			historyImages[historyWriteIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blitRegion, VK_FILTER_NEAREST);

		historyImageLayouts[historyWriteIndex] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;


		VkImageMemoryBarrier2 barrierPresent{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.image = swapchainImages[imageIndex],
			.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
		};
		VkDependencyInfo barrierPresentDependencyInfo{ 
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, 
			.imageMemoryBarrierCount = 1, 
			.pImageMemoryBarriers = &barrierPresent 
		};
		vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);
		chk(vkEndCommandBuffer(cb));
		// Submit to graphics queue
		VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &imageAcquiredSemaphores[frameIndex],
			.pWaitDstStageMask = &waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &cb,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &renderCompleteSemaphores[imageIndex],
		};
		chk(vkQueueSubmit(queue, 1, &submitInfo, fences[frameIndex]));
		frameIndex = (frameIndex + 1) % maxFramesInFlight;
		VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &renderCompleteSemaphores[imageIndex],
			.swapchainCount = 1,
			.pSwapchains = &swapchain,
			.pImageIndices = &imageIndex
		};
		chkSwapchain(vkQueuePresentKHR(queue, &presentInfo));
		// Event polling
		float elapsedTime{ (SDL_GetTicks() - lastTime) / 1000.0f };
		lastTime = SDL_GetTicks();

		inputs.handleKeyInputs();
		quit = KeyInputs::QUIT;
		bool camMoved = false;
		if (KeyInputs::MOUSE_DELTA_X != 0 || KeyInputs::MOUSE_DELTA_Y != 0)
		{
			float pitchDelta = -KeyInputs::MOUSE_DELTA_Y * elapsedTime * KeyInputs::MOUSE_SENSITIVITY;
			float yawDelta = KeyInputs::MOUSE_DELTA_X * elapsedTime * KeyInputs::MOUSE_SENSITIVITY;

			camera.pitch += pitchDelta;
			camera.pitch = std::clamp(camera.pitch, -89.f, 89.f);
			camera.yaw += yawDelta;
			float yawRad = glm::radians(camera.yaw);
			float pitchRad = glm::radians(camera.pitch);

			glm::vec3 direction;

			direction.x = std::cos(pitchRad) * std::cos(yawRad);
			direction.y = std::sin(pitchRad);
			direction.z = std::cos(pitchRad) * std::sin(yawRad);
			camera.direction = glm::normalize(direction);
			camera.right = glm::normalize(glm::cross(camera.direction, WORLD_UP));
			camera.up = glm::cross(camera.right, camera.direction);

			camMoved = true;
		}
		if (KeyInputs::MOUSE_WHEEL)
		{
			camera.fov -= (float)KeyInputs::MOUSE_WHEEL * elapsedTime * 50.f;
			camera.fov = std::clamp(camera.fov, 1.f, 179.f);
			camMoved = true;
		}
		if (KeyInputs::FORWARD)
		{
			camera.origin += camera.direction * elapsedTime;
			camMoved = true;
		}
		if (KeyInputs::BACKWARD)
		{
			camera.origin -= camera.direction * elapsedTime;
			camMoved = true;
		}
		if (KeyInputs::LEFT)
		{
			camera.origin -= camera.right * elapsedTime;
			camMoved = true;
		}
		if (KeyInputs::RIGHT)
		{
			camera.origin += camera.right * elapsedTime;
			camMoved = true;
		}
		if (KeyInputs::UP)
		{
			camera.origin += camera.up * elapsedTime;
			camMoved = true;
		}
		if (KeyInputs::DOWN)
		{
			camera.origin -= camera.up * elapsedTime;
			camMoved = true;
		}

		updateSwapchain = KeyInputs::WINDOW_RESIZED;
		if (camMoved)
		{
			shaderData.frameCount = 0;
		}

		// update history read index
		historyReadIndex = historyWriteIndex;

		if (updateSwapchain) {
			chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
			shaderData.windowMax = windowSize;
			updateSwapchain = false;
			chk(vkDeviceWaitIdle(device));
			chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface, &surfaceCaps));
			swapchainCI.oldSwapchain = swapchain;
			swapchainCI.imageExtent = { .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) };
			chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));
			for (auto i = 0; i < imageCount; i++) {
				vkDestroyImageView(device, swapchainImageViews[i], nullptr);
			}
			chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
			swapchainImages.resize(imageCount);
			chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
			swapchainImageViews.resize(imageCount);
			for (auto i = 0; i < imageCount; i++) {
				VkImageViewCreateInfo viewCI{ 
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, 
					.image = swapchainImages[i], 
					.viewType = VK_IMAGE_VIEW_TYPE_2D, 
					.format = imageFormat, 
					.subresourceRange = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
						.levelCount = 1, 
						.layerCount = 1
					} 
				};
				chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
			}

			destroyHistoryImages();
			initHistoryImages();

			for (auto& semaphore : renderCompleteSemaphores) {
				vkDestroySemaphore(device, semaphore, nullptr);
			}
			renderCompleteSemaphores.resize(imageCount);
			for (auto& semaphore : renderCompleteSemaphores) {
				chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
			}
			vkDestroySwapchainKHR(device, swapchainCI.oldSwapchain, nullptr);
			vmaDestroyImage(allocator, depthImage, depthImageAllocation);
			vkDestroyImageView(device, depthImageView, nullptr);
			depthImageCI.extent = { .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y), .depth = 1 };
			VmaAllocationCreateInfo allocCI{ .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, .usage = VMA_MEMORY_USAGE_AUTO };
			chk(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr));
			VkImageViewCreateInfo viewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = depthImage, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = depthFormat, .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 } };
			chk(vkCreateImageView(device, &viewCI, nullptr, &depthImageView));
		}
		return quit;
	}

	

};