#pragma once

#include "VulkanHelper.h"
#include "Utility.h"
#include "KeyInputs.h"
#include <unordered_set>
#include <bit>

constexpr uint32_t maxBounces{ 10 };
constexpr uint32_t maxFramesInFlight{ 2 };
constexpr uint32_t numHistoryFrames{ 2 };
constexpr uint32_t objectTypes{ 10 };
constexpr glm::vec3 WORLD_UP{ 0, 1, 0 };
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
const bool enableValidationLayers = true;

class VK_Wrap
{
	uint32_t bounces{ 0 };
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
	std::vector<VkImageLayout> swapchainImageLayouts;
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

	// per frame image
	VkImage frameImage{};
	VkImageView frameImageView{};
	VkImageLayout frameImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VmaAllocation frameImageAllocation{};
	VkFormat frameImageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

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
	std::vector<Triangle> triangles;
	std::vector<bvhNode> bvhNodes;
	std::vector<bvhPacked> bvhNodesPacked;
	std::vector<Material> materials;
	std::vector<GaussianSplat> splats;
	std::vector<Transform> transforms;
	std::vector<SphericalHarmonic> shMats;
	std::vector<Ray> rayOrigins;
	std::vector<KDopNode> kdopNodes;
	std::vector<KDopNodeHot> kdopHotNodes;
	std::vector<K14DopNodeCold> kdopColdNodes;
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

		destroyFrameImage();

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
				.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE
			};
			VmaAllocationCreateInfo uBufferAllocCI{ 
				.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |  // removed: VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
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

		loadStructuredBuffer("triangles", triangles);

		loadStructuredBuffer("splats", splats);

		loadStructuredBuffer("shMats", shMats);

		loadStructuredBuffer("materials", materials);

		packBvhNodes();

		loadStructuredBuffer("bvhNodes", bvhNodesPacked);

		std::cout << "kdop size: " << kdopHotNodes.size() << "\n";

		loadStructuredBuffer("kdopHotNodes", kdopHotNodes);
		loadStructuredBuffer("kdopColdNodes", kdopColdNodes);


		loadStructuredBuffer("transforms", transforms);

		loadStructuredBuffer("rays", rayOrigins, windowSize.x * windowSize.y);


		initHistoryImages();

		initFrameImage();

		initBindings();

		initPipeline();

		updateStructuredBufferDescriptors("spheres");

		updateStructuredBufferDescriptors("triangles");

		updateStructuredBufferDescriptors("splats");

		updateStructuredBufferDescriptors("shMats");

		updateStructuredBufferDescriptors("materials");
		
		updateStructuredBufferDescriptors("bvhNodes");

		updateStructuredBufferDescriptors("kdopHotNodes");
		updateStructuredBufferDescriptors("kdopColdNodes");
		
		updateStructuredBufferDescriptors("transforms");

		updateStructuredBufferDescriptors("rays");

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
		VkDeviceQueueCreateInfo queueCI{ 
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 
			.queueFamilyIndex = queueFamily, 
			.queueCount = 1, 
			.pQueuePriorities = &qfpriorities 
		};
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
		VkPhysicalDeviceFeatures enabledVk10Features{ 
			.samplerAnisotropy = VK_TRUE,
			.fragmentStoresAndAtomics = VK_TRUE,
			.shaderInt64 = VK_TRUE

		};
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
		swapchainImageLayouts.assign(imageCount, VK_IMAGE_LAYOUT_UNDEFINED);
		for (auto i = 0; i < imageCount; i++) {
			VkImageViewCreateInfo viewCI{ 
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, 
				.image = swapchainImages[i], 
				.viewType = VK_IMAGE_VIEW_TYPE_2D, 
				.format = imageFormat, 
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
					.levelCount = 1, 
					.layerCount = 1 } 
			};
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
		shaderData.frameCount = 0;
		shaderData.resetRays = true;
	}

	void initFrameImage()
	{
		VkImageCreateInfo imageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.extent{
				.width = static_cast<uint32_t>(windowSize.x),
				.height = static_cast<uint32_t>(windowSize.y),
				.depth = 1
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage =
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
				VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		VmaAllocationCreateInfo allocCI{
			.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO
		};

		chk(vmaCreateImage(
			allocator,
			&imageCI,
			&allocCI,
			&frameImage,
			&frameImageAllocation,
			nullptr
		));

		VkImageViewCreateInfo viewCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = frameImage,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.subresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		chk(vkCreateImageView(
			device,
			&viewCI,
			nullptr,
			&frameImageView
		));

		frameImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	void destroyFrameImage()
	{
		if (frameImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(device, frameImageView, nullptr);
			frameImageView = VK_NULL_HANDLE;
		}

		if (frameImage != VK_NULL_HANDLE)
		{
			vmaDestroyImage(
				allocator,
				frameImage,
				frameImageAllocation
			);

			frameImage = VK_NULL_HANDLE;
			frameImageAllocation = VK_NULL_HANDLE;
		}

		frameImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
					VK_IMAGE_USAGE_TRANSFER_DST_BIT |
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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
			.pColorAttachmentFormats = &frameImageFormat,
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
			std::to_array<slang::CompilerOptionEntry>({
				{
					slang::CompilerOptionName::EmitSpirvDirectly,
					{slang::CompilerOptionValueKind::Int, 1}
				},
				{
					slang::CompilerOptionName::DebugInformation,
					{
						slang::CompilerOptionValueKind::Int,
						SLANG_DEBUG_INFO_LEVEL_STANDARD
					}
				},
				{
					slang::CompilerOptionName::DebugInformationFormat,
					{
						slang::CompilerOptionValueKind::Int,
						SLANG_DEBUG_INFO_FORMAT_DEFAULT
					}
				}
			})
		};
		const char* searchPaths[] = {
			"shaders"
		};
		slang::SessionDesc slangSessionDesc{
			.targets{slangTargets.data()},
			.targetCount{SlangInt(slangTargets.size())},
			.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
			.searchPaths = searchPaths,
			.searchPathCount = 1,
			.compilerOptionEntries{slangOptions.data()},
			.compilerOptionEntryCount{uint32_t(slangOptions.size())}
		};
		Slang::ComPtr<ISlangBlob> diagnostics;

		// Load shader
		Slang::ComPtr<slang::ISession> slangSession;
		slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());
		Slang::ComPtr<slang::IModule> slangModule{
			slangSession->loadModuleFromSource("triangle", "shaders/shader.slang", nullptr, diagnostics.writeRef())
		};
		if (diagnostics && diagnostics->getBufferSize() > 0)
		{
			std::cerr << (const char*)diagnostics->getBufferPointer() << "\n";
		}

		if (!slangModule)
		{
			std::cout << "Failed to load shaders/shader.slang\n";
			exit(-1);
		}



		// Get all the shaders actually used
		int entryPointCount = (int)slangModule->getDefinedEntryPointCount();
		std::vector<Slang::ComPtr<slang::IEntryPoint>> entryPoints(entryPointCount);
		std::vector<slang::IComponentType*> componentsToLink;
		componentsToLink.push_back(slangModule);
		for (int i =0; i < entryPointCount; i++)
		{
			slangModule->getDefinedEntryPoint(i, entryPoints[i].writeRef());
			componentsToLink.push_back(entryPoints[i]);
		}

		Slang::ComPtr<slang::IComponentType> composedProgram;
		SlangResult composeResult = slangSession->createCompositeComponentType(
			componentsToLink.data(), (SlangInt)componentsToLink.size(),
			composedProgram.writeRef(), diagnostics.writeRef()
		);
		if (diagnostics && diagnostics->getBufferSize() > 0)
		{
			std::cerr << (const char*)diagnostics->getBufferPointer() << "\n";
		}
		chk(SLANG_SUCCEEDED(composeResult));


		Slang::ComPtr<slang::IComponentType> linkedProgram;
		SlangResult linkResult = composedProgram->link(linkedProgram.writeRef(), diagnostics.writeRef());
		if (diagnostics && diagnostics->getBufferSize() > 0)
		{
			std::cerr << (const char*)diagnostics->getBufferPointer() << "\n";
		}
		chk(SLANG_SUCCEEDED(linkResult));



		// get reflection indices
		slang::ProgramLayout* programLayout = linkedProgram->getLayout(0, diagnostics.writeRef());
		if (diagnostics && diagnostics->getBufferSize() > 0)
		{
			std::cerr << (const char*)diagnostics->getBufferPointer() << "\n";
		}
		

		unsigned paramCount = programLayout->getParameterCount();
		for (unsigned i = 0; i < paramCount; i++)
		{
			slang::VariableLayoutReflection* param = programLayout->getParameterByIndex(i);
			std::string name = param->getName();

			auto* bufferLayout = param->getTypeLayout();
			std::cout << "parameter: " << param->getName() << "\n";
			std::cout << "type:      " << bufferLayout->getName() << "\n";
			std::cout << "kind:      " << int(bufferLayout->getKind()) << "\n";


			bindings[name] = {
				.binding = param->getBindingIndex(),
				.set = param->getBindingSpace()
			};
		}


		Slang::ComPtr<ISlangBlob> spirv;
		linkedProgram->getTargetCode(0, spirv.writeRef(), diagnostics.writeRef());
		if (diagnostics && diagnostics->getBufferSize() > 0)
		{
			std::cerr << (const char*)diagnostics->getBufferPointer() << "\n";
		}

		std::ofstream f("triangle.spv", std::ios::binary);

		f.write(
			static_cast<const char*>(spirv->getBufferPointer()),
			spirv->getBufferSize()
		);

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
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.size = sizeof(ShaderData)
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


		std::array<VkDescriptorPoolSize, objectTypes + 1> poolSizes{};
		for (int i = 0; i < objectTypes; i++)
		{
			poolSizes[i] = VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = maxFramesInFlight
			};
		}
		poolSizes[objectTypes] = VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = maxFramesInFlight
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
	void loadStructuredBuffer(std::string bindingName, const std::vector<T>& data, size_t elementCount)
	{
		if (data.size() == 0 && elementCount == 0)
		{
			std::cout << bindingName << " was empty\n";
			elementCount = 1;
		}
		// check if a binding exists, if it does clear it
		if (structuredBufferBindings.find(bindingName) != structuredBufferBindings.end())
		{
			std::cout << "Bindings already exist for " << bindingName << "\n";
			auto& binding = structuredBufferBindings[bindingName];
			vmaDestroyBuffer(allocator, binding.stagingBuffer, binding.stagingAllocation);
			vmaDestroyBuffer(allocator, binding.buffer, binding.bufferAllocation);
			
			//return;
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
			.size = sizeof(T) * elementCount,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
		VmaAllocationCreateInfo allocCreateInfo{
			.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
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

		if (!data.empty())
		{
			memcpy(binding.stagingAllocInfo.pMappedData, data.data(), (size_t)binding.bufferInfo.size);
		}
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
	template <typename T>
	void loadStructuredBuffer(std::string bindingName, const std::vector<T>& data)
	{
		size_t elementCount = std::max(data.size(), (size_t) 1);
		loadStructuredBuffer(bindingName, data, elementCount);
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
			.buffer = binding.buffer,
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

	void resizeRaysBuffer(glm::vec2 windowSize)
	{
		std::string bindingName = "rays";
		if (structuredBufferBindings.find(bindingName) != structuredBufferBindings.end())
		{
			auto& binding = structuredBufferBindings[bindingName];
			vmaDestroyBuffer(allocator, binding.stagingBuffer, binding.stagingAllocation);
			vmaDestroyBuffer(allocator, binding.buffer, binding.bufferAllocation);
		}
		if (bindings.find(bindingName) == bindings.end())
		{
			std::cout << "Binding does not exist in reflection for " << bindingName << "\n";
			return;
		}

		std::vector<Ray> rays = {};
		size_t elementCount = windowSize.x * windowSize.y;
		loadStructuredBuffer(bindingName, rays, elementCount);
		updateStructuredBufferDescriptors(bindingName);
	}

	/*
	Object loaders:
		return the index of the head of their BVH

		
	*/
	void transformAABB(const glm::mat4 M, const glm::vec3& min, const glm::vec3& max, glm::vec3& outMin, glm::vec3& outMax)
	{
		glm::vec3 center = (max + min) * 0.5f;
		glm::vec3 extent = (max - min) * 0.5f;
		glm::vec3 worldCenter = glm::vec3(M * glm::vec4(center, 1.f));
		glm::mat3 linear(M);

		glm::vec3 worldExtent(
			std::abs(M[0][0]) * extent.x +
			std::abs(M[1][0]) * extent.y +
			std::abs(M[2][0]) * extent.z,

			std::abs(M[0][1]) * extent.x +
			std::abs(M[1][1]) * extent.y +
			std::abs(M[2][1]) * extent.z,

			std::abs(M[0][2]) * extent.x +
			std::abs(M[1][2]) * extent.y +
			std::abs(M[2][2]) * extent.z
		);		
		outMin = worldCenter - worldExtent;
		outMax = worldCenter + worldExtent;
	}

	void getChildMinMax(const Transform& transform, glm::vec3& min, glm::vec3& max)
	{
		PrimType prim = transform.childPrim;
		if (prim == BVH_NODE)
		{
			const auto& c = bvhNodes[transform.childIndex];
			if (c.left.type != PrimType::EMPTY)
			{
				min = make_aabb_min(min, c.left.min);
				max = make_aabb_max(max, c.left.max);
			}
			if (c.right.type != PrimType::EMPTY)
			{
				min = make_aabb_min(min, c.right.min);
				max = make_aabb_max(max, c.right.max);
			}
		}
		else if (prim == KDOP_NODE)
		{
			const auto& c = kdopNodes[transform.childIndex];
			PrimType left = unpackType(c.packedIndexType_left);
			PrimType right = unpackType(c.packedIndexType_right);
			if (left != PrimType::EMPTY)
			{
				glm::vec3 aabbMin(c.kDop_left.min[0], c.kDop_left.min[1], c.kDop_left.min[2]);
				glm::vec3 aabbMax(c.kDop_left.max[0], c.kDop_left.max[1], c.kDop_left.max[2]);
				min = make_aabb_min(min, aabbMin);
				max = make_aabb_max(max, aabbMax);
			}
			if (right != PrimType::EMPTY)
			{
				glm::vec3 aabbMin(c.kDop_right.min[0], c.kDop_right.min[1], c.kDop_right.min[2]);
				glm::vec3 aabbMax(c.kDop_right.max[0], c.kDop_right.max[1], c.kDop_right.max[2]);
				min = make_aabb_min(min, aabbMin);
				max = make_aabb_max(max, aabbMax);
			}
		}
		else 
		{
			throw std::runtime_error("Not a supported type: " + transform.childPrim);
		}
		
	}

	void getTransformMinMax(const Transform& transform, glm::vec3& min, glm::vec3& max)
	{
		glm::vec3 childMin(FLT_MAX);
		glm::vec3 childMax(-FLT_MAX);
		getChildMinMax(transform, childMin, childMax);
		transformAABB(transform.matrix, childMin, childMax, min, max);
	}

	void getSceneBounds(glm::vec3& sceneMin, glm::vec3& sceneMax)
	{
		for (const auto& s : spheres)
		{
			sceneMin = make_aabb_min(s.center - s.radius, sceneMin);
			sceneMax = make_aabb_max(s.center + s.radius, sceneMax);
		}
		for (const auto& t : transforms)
		{
			glm::vec3 leftMin;
			glm::vec3 leftMax;
			getTransformMinMax(t, leftMin, leftMax);
			sceneMin = make_aabb_min(leftMin, sceneMin);
			sceneMax = make_aabb_max(leftMax, sceneMax);
		}
	}

	int flattenKDop(int rootIndex)
	{
		struct FrontierChild
		{
			unsigned int packed;
			K14Dop kdop;
		};
		const KDopNode& root = kdopNodes[rootIndex];
		std::vector<FrontierChild> frontier;
		frontier.reserve(KDOP_WIDTH);

		frontier.push_back(FrontierChild{
			.packed = root.packedIndexType_left,
			.kdop = root.kDop_left
			});
		frontier.push_back(FrontierChild{
			.packed = root.packedIndexType_right,
			.kdop = root.kDop_right
			});

		while (frontier.size() < KDOP_WIDTH)
		{
			int bestCandidate = -1;
			float bestDelta = FLT_MAX;

			for (int i = 0; i < (int)frontier.size(); ++i)
			{
				const FrontierChild& candidate = frontier[i];
				PrimType type = unpackType(candidate.packed);

				if (type != PrimType::KDOP_NODE)
				{
					continue;
				}

				int childIndex = unpackIndex(candidate.packed);
				const KDopNode& binaryChild = kdopNodes[childIndex];
				float oldCost = kdopSAHCost(candidate.kdop);

				float newCost =
					kdopSAHCost(binaryChild.kDop_left) +
					kdopSAHCost(binaryChild.kDop_right);

				float delta = newCost - oldCost;

				if (delta < bestDelta)
				{
					bestDelta = delta;
					bestCandidate = i;
				}
			}
			if (bestCandidate == -1)
			{
				break;
			}
			FrontierChild candidate = frontier[bestCandidate];

			int childIndex = unpackIndex(candidate.packed);
			const KDopNode& binaryChild = kdopNodes[childIndex];

			frontier[bestCandidate] = FrontierChild{
				.packed = binaryChild.packedIndexType_left,
				.kdop = binaryChild.kDop_left
			};
			if (unpackType(binaryChild.packedIndexType_right) != PrimType::EMPTY)
			{
				frontier.push_back(FrontierChild{
					.packed = binaryChild.packedIndexType_right,
					.kdop = binaryChild.kDop_right
					});
			}
		}

		for (FrontierChild& child : frontier)
		{
			PrimType type = unpackType(child.packed);

			if (type != PrimType::KDOP_NODE) continue;

			int binaryChildIndex = unpackIndex(child.packed);
			int wideChildIndex = flattenKDop(binaryChildIndex);
			child.packed = packChild(
				PrimType::KDOP_NODE,
				wideChildIndex
			);
		}

		KDopNodeHot hot{};
		K14DopNodeCold cold{};

		for (int i = 0; i < KDOP_WIDTH; ++i)
		{
			if (i < (int)frontier.size())
			{
				const FrontierChild& child = frontier[i];
				// Hot: first three axes + packed child index/type.
				hot.min_packed[i] = glm::vec4(
					child.kdop.min[0],
					child.kdop.min[1],
					child.kdop.min[2],
					std::bit_cast<float>(child.packed)
				);

				// w is reserved.
				hot.max[i] = glm::vec4(
					child.kdop.max[0],
					child.kdop.max[1],
					child.kdop.max[2],
					0.0f
				);

				// Cold: remaining four k-DOP axes.
				cold.min[i] = glm::vec4(
					child.kdop.min[3],
					child.kdop.min[4],
					child.kdop.min[5],
					child.kdop.min[6]
				);

				cold.max[i] = glm::vec4(
					child.kdop.max[3],
					child.kdop.max[4],
					child.kdop.max[5],
					child.kdop.max[6]
				);
			}
			else
			{
				// empty slot
				float packed = std::bit_cast<float>(packChild(PrimType::EMPTY, 0));
				hot.min_packed[i] = glm::vec4(FLT_MAX, FLT_MAX, FLT_MAX, packed);
				hot.max[i] = glm::vec4(-FLT_MAX);

				cold.min[i] = glm::vec4(FLT_MAX);
				cold.max[i] = glm::vec4(-FLT_MAX);
			}
		}

		int wideIndex = (int)kdopHotNodes.size();
		kdopHotNodes.push_back(hot);
		kdopColdNodes.push_back(cold);
		return wideIndex;		
	}

	void packBvhNodes()
	{
		// convert old bvh nodes to the packed version (aka the gpu version)
		// TODO later: just make everything the packed version
		bvhNodesPacked.reserve(bvhNodes.size());
		for (const auto& node : bvhNodes)
		{
			/*
			struct bvhPacked
			{
				glm::vec4 leftMin_leftPacked; // xyz left.min, w = packed left.type / left.index
				glm::vec4 leftMax_rightPacked; // xyz left.max, w = packed right.type / right.index
				glm::vec4 rightMin_reserved0; // xyz right.min, w unused
				glm::vec4 rightMax_reserved1; // same
			};
			*/
			// pack
			unsigned int packedLeft = packChild(node.left.type, node.left.index);
			unsigned int packedRight = packChild(node.right.type, node.right.index);
			bvhNodesPacked.push_back(bvhPacked{
				.leftMin_leftPacked = glm::vec4(node.left.min, std::bit_cast<float>(packedLeft)),
				.leftMax_rightPacked = glm::vec4(node.left.max, std::bit_cast<float>(packedRight)),
				.rightMin_reserved0 = glm::vec4(node.right.min, 0),
				.rightMax_reserved1 = glm::vec4(node.right.max, 0)
				});
		}
	}

	int loadBVH()
	{
		// build TLAS bvh
		
		// get scene bounds
		glm::vec3 sceneMax(-FLT_MAX);
		glm::vec3 sceneMin(FLT_MAX);
		

		std::vector<LeafItem> leaves;
		for (size_t i = 0; i < spheres.size(); ++i)
		{
			const auto& s = spheres[i];
			leaves.push_back({ s.center - s.radius, s.center + s.radius,
								s.center, PrimType::SPHERE, (int)i });
		}
		for (size_t i = 0; i < transforms.size(); ++i)
		{
			glm::vec3 min, max;
			getTransformMinMax(transforms[i], min, max);
			leaves.push_back({ min, max, (min + max) * 0.5f, PrimType::TRANSFORM, (int)i });
		}

		glm::vec3 rootMin, rootMax;
		int root = buildSAH(leaves, 0, (int)leaves.size(), rootMin, rootMax);

		/*
		for (const auto& node : bvhNodes)
		{
			std::cout << "BVH Node: " << node.mortonCode << "\nLeft: " << node.left.index << " type: " << node.left.type <<
				"\nRight: " << node.right.index << " type: " << node.right.type << "\n";
		}
		*/
		return root;
	}

	int build14DOP(std::vector<KDopLeaf>& leaves, int start, int end, K14Dop& outKDop)
	{
		int count = end - start;
		if (count <= 2)
		{
			K14Dop leftKDop = leaves[start].kDop;
			unsigned int leftPacked = packChild(leaves[start].type, leaves[start].index);
			K14Dop rightKDop = makeEmptyKDop();
			unsigned int rightPacked = packChild(PrimType::EMPTY, 0);
			if (count == 2)
			{
				rightKDop = leaves[start + 1].kDop;
				rightPacked = packChild(leaves[start + 1].type, leaves[start + 1].index);
			}

			K14Dop mergedKDop = mergeKDop(leftKDop, rightKDop);
			outKDop = mergedKDop;
			kdopNodes.push_back(KDopNode{
				.packedIndexType_left = leftPacked,
				.packedIndexType_right = rightPacked,
				.kDop_left = leftKDop,
				.kDop_right = rightKDop
				});
			return (int)kdopNodes.size() - 1;
		}
		// find centroid spread
		glm::vec3 cMin(FLT_MAX);
		glm::vec3 cMax(-FLT_MAX);

		for (int i = start; i < end; ++i)
		{
			cMin = make_aabb_min(leaves[i].center, cMin);
			cMax = make_aabb_max(leaves[i].center, cMax);
		}

		// pick axis

		glm::vec3 ext = cMax - cMin;
		int axis = (ext.x > ext.y)
			? (ext.x > ext.z ? 0 : 2)
			: (ext.y > ext.z ? 1 : 2);

		// sort along axis
		std::sort(
			leaves.begin() + start,
			leaves.begin() + end,
			[axis](const KDopLeaf& a, const KDopLeaf& b)
			{
				return a.center[axis] < b.center[axis];
			}
		);
		// try split locations
		int bestSplit = start + count / 2;
		float bestCost = FLT_MAX;

		int buckets = std::min(count - 1, 16);
		for (int b = 1; b <= buckets; ++b)
		{
			int split = start + (count * b) / (buckets + 1);
			if (split <= start || split >= end) continue;

			K14Dop leftKDop = makeEmptyKDop();
			K14Dop rightKDop = makeEmptyKDop();

			for (int i = start; i < split; ++i)
			{
				leftKDop = mergeKDop(leftKDop, leaves[i].kDop);
			}
			for (int i = split; i < end; ++i)
			{
				rightKDop = mergeKDop(rightKDop, leaves[i].kDop);
			}

			float leftCount = float(split - start);
			float rightCount = float(end - split);

			float cost = kdopSAHCost(leftKDop) * leftCount + kdopSAHCost(rightKDop) * rightCount;

			if (cost < bestCost)
			{
				bestCost = cost;
				bestSplit = split;
			}
		}
		
		// recursively build children
		K14Dop leftKDop;
		K14Dop rightKDop;

		int leftIndex = build14DOP(leaves, start, bestSplit, leftKDop);
		int rightIndex = build14DOP(leaves, bestSplit, end, rightKDop);

		outKDop = mergeKDop(leftKDop, rightKDop);

		kdopNodes.push_back(KDopNode{
			.packedIndexType_left = packChild(PrimType::KDOP_NODE, leftIndex),
			.packedIndexType_right = packChild(PrimType::KDOP_NODE, rightIndex),
			.kDop_left = leftKDop,
			.kDop_right = rightKDop
			});
		return (int) kdopNodes.size() - 1;
	}

	int buildSAH(std::vector<LeafItem>& leaves, int start, int end, glm::vec3& outMin, glm::vec3& outMax)
	{
		float emptyBonusFactor = 0.5;
		int count = end - start;

		if (count <= 2)
		{
			bvhChild left{ 
				leaves[start].min, 
				leaves[start].max, 
				leaves[start].type, 
				leaves[start].index };

			bvhChild right = bvhChild{ glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX), PrimType::EMPTY, -1 };
			if (count == 2)
			{
				right = bvhChild{ 
					leaves[start + 1].min, 
					leaves[start + 1].max, 
					leaves[start + 1].type, 
					leaves[start + 1].index };
			}

			outMin = make_aabb_min(left.min, right.min);
			outMax = make_aabb_max(left.max, right.max);
			bvhNodes.push_back(bvhNode{ .mortonCode = 0, .left = left, .right = right });
			return (int)bvhNodes.size() - 1;
		}

		// choose split axis by spread of centers
		glm::vec3 cMin(FLT_MAX), cMax(-FLT_MAX);
		for (int i = start; i < end; ++i)
		{
			cMin = make_aabb_min(leaves[i].center, cMin);
			cMax = make_aabb_max(leaves[i].center, cMax);
		}
		glm::vec3 ext = cMax - cMin;
		int axis = (ext.x > ext.y) ? (ext.x > ext.z ? 0 : 2) : (ext.y > ext.z ? 1 : 2);
		
		std::sort(leaves.begin() + start, leaves.begin() + end,
			[axis](const LeafItem& a, const LeafItem& b) { return a.center[axis] < b.center[axis]; });

		int bestSplit = start + count / 2;
		float bestCost = FLT_MAX;
		int buckets = std::min(count - 1, 16);

		for (int b = 1; b <= buckets; ++b)
		{
			int split = start + (count * b) / (buckets + 1);
			//if (split <= start || split >= end) continue;

			glm::vec3 lMin(FLT_MAX), lMax(-FLT_MAX), rMin(FLT_MAX), rMax(-FLT_MAX);
			for (int i = start; i < split; ++i) 
			{ 
				lMin = make_aabb_min(leaves[i].min, lMin); 
				lMax = make_aabb_max(leaves[i].max, lMax); 
			}
			for (int i = split; i < end; ++i) 
			{ 
				rMin = make_aabb_min(leaves[i].min, rMin); 
				rMax = make_aabb_max(leaves[i].max, rMax); 
			}
			float leftCount = split - start;
			float rightCount = end - split;
			float cost = surfaceArea(lMax - lMin) * (leftCount) + surfaceArea(rMax - rMin) * (rightCount);
			if (leftCount == 0 || rightCount == 0)
			{
				// reward empty space - noticably improved number of nodes hit :)
				cost *= emptyBonusFactor;
			}
			if (cost < bestCost) 
			{ 
				bestCost = cost; 
				bestSplit = split; 
			}
		}

		glm::vec3 lMin(FLT_MAX), lMax(-FLT_MAX), rMin(FLT_MAX), rMax(-FLT_MAX);
		int leftIndex = buildSAH(leaves, start, bestSplit, lMin, lMax);
		int rightIndex = buildSAH(leaves, bestSplit, end, rMin, rMax);

		outMin = make_aabb_min(lMin, rMin);
		outMax = make_aabb_max(lMax, rMax);

		bvhNodes.push_back(bvhNode{
			.mortonCode = 0,
			.left = bvhChild{
				lMin, lMax, PrimType::BVH_NODE, leftIndex
			},
			.right = bvhChild{
				rMin, rMax, PrimType::BVH_NODE, rightIndex
			}
			});
		return (int)bvhNodes.size() - 1;

	}
	
	int buildChildBVH(std::vector<LeafItem>& leaves)
	{
		if (leaves.size() == 0)
		{
			// return the current root node
			return bvhNodes.size() - 1;
		}
		glm::vec3 sceneMin(FLT_MAX);
		glm::vec3 sceneMax(-FLT_MAX);

		for (auto& leaf : leaves)
		{
			sceneMin = make_aabb_min(leaf.min, sceneMin);
			sceneMax = make_aabb_max(leaf.max, sceneMax);
		}

		glm::vec3 invSceneExtent = 1.f / make_aabb_max(sceneMax - sceneMin, glm::vec3(1e-8));

		std::sort(leaves.begin(), leaves.end(), [&](const LeafItem& a, const LeafItem& b)
			{
				return compute_morton_code(a.center, sceneMin, invSceneExtent)
					< compute_morton_code(b.center, sceneMin, invSceneExtent);
			});
		
		// add to bvhNodes and build inner bvhNodes
		int startIndex = bvhNodes.size();
		int endIndex = startIndex + (leaves.size() / 2);

		// return root node index
		glm::vec3 childMin;
		glm::vec3 childMax;
		return buildSAH(leaves, 0, leaves.size(), childMin, childMax);
	}
	
	int loadTransform(glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale, PrimType childPrim, int childIndex)
	{
		assert(childPrim == BVH_NODE || KDOP_NODE);
		int index = childIndex;
		if (childPrim == KDOP_NODE)
		{
			// flatten into wide kdop before transforming
			index = flattenKDop(childIndex);
			validateWideKDop(index);
		}

		constexpr float MIN_SCALE = 1e-8f;

		if (std::abs(scale.x) < MIN_SCALE ||
			std::abs(scale.y) < MIN_SCALE ||
			std::abs(scale.z) < MIN_SCALE)
		{
			throw std::runtime_error("Degenerate transform scale");
		}

		// dont need to build another bvh node for this since they are all in the TLAS bvh
		glm::mat4 transMat = glm::translate(glm::mat4(1), translation);
		glm::quat rotationQuat(rotation);
		glm::mat4 rotMat = glm::mat4_cast(rotationQuat);
		glm::mat4 scaleMat = glm::scale(glm::mat4(1), scale);

		glm::mat4 transformMatrix = transMat * rotMat * scaleMat;
		Transform transform{
			.matrix = transformMatrix,
			.invMatrix = glm::inverse(transformMatrix),
			.childPrim = childPrim,
			.childIndex = index
		};
		transforms.push_back(transform);
		return transforms.size() - 1;
	}

	int loadObj(std::string filepath, unsigned int materialIndex)
	{
		// load mesh and build its BLAS bvh
		// the only way for this to be in the scene is to make it the child of a transform node
		// Mesh data
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, filepath.c_str()));
		indexCount = VkDeviceSize{ shapes[0].mesh.indices.size() };
		std::vector<Vertex> vertices{};
		// Load vertex and index data
		for (auto& index : shapes[0].mesh.indices) {
			Vertex v{
				.pos = { attrib.vertices[index.vertex_index * 3], attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2] },
				// normal and uv are intentionally unused
				//.normal = { attrib.normals[index.normal_index * 3], attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] },
				//.uv = { attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1] }
			};
			vertices.push_back(v);
		}
		if (vertices.size() % 3 != 0)
		{
			throw std::runtime_error( "When loading " + filepath + " number of vertices was not a multiple of 3!");
		}
		std::cout << "Loading " << filepath << " with " << (int)vertices.size() / 3 << " triangles\n";
		std::vector<LeafItem> leaves;
		for (size_t i = 0; i < vertices.size(); i += 3)
		{
			glm::vec3 v0 = vertices[i].pos;
			glm::vec3 v1 = vertices[i + 1].pos;
			glm::vec3 v2 = vertices[i + 2].pos;

			glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

			glm::vec3 min;
			glm::vec3 max;
			max = make_aabb_max(make_aabb_max(v0, v1), v2);
			min = make_aabb_min(make_aabb_min(v0, v1), v2);

			max += 0.001f;
			min -= 0.001f;
			leaves.push_back(LeafItem{
				.min = min,
				.max = max,
				.center = (min + max) * 0.5f,
				.type = PrimType::TRIANGLE,
				.index = (int) triangles.size()
				});
			triangles.push_back(
				Triangle{
					.v0 = v0, .v1 = v1, .v2 = v2,
					.normal = normal,
					.d = -1 * dot(normal, v0),
					.materialIndex = materialIndex
				}
			);
		}

		return buildChildBVH(leaves);
	}

	int loadSplat(std::string filepath)
	{
		std::cout << "Loading " << filepath << "\n";
		// same as obj loader in terms of BLAS and TLAS
		std::vector<bvhNode> tmpNodes{};
		std::ifstream file(filepath, std::ios::binary);
		if (!file) {
			throw std::runtime_error("Could not open PLY file: " + filepath);
		}

		std::string line;
		if (!std::getline(file, line) || line != "ply") {
			throw std::runtime_error("Not a PLY file: " + filepath);
		}

		bool sawFormat = false;
		bool inVertex = false;
		std::size_t vertexCount = 0;
		std::size_t rowStride = 0;
		std::vector<plyDetail::Property> properties;

		while (std::getline(file, line)) {
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}

			if (line == "end_header") {
				break;
			}

			std::vector<std::string> words = plyDetail::splitWords(line);
			if (words.empty() || words[0] == "comment") {
				continue;
			}

			if (words[0] == "format") {
				if (words.size() < 3 || words[1] != "binary_little_endian") {
					throw std::runtime_error("Only binary_little_endian PLY files are supported");
				}
				sawFormat = true;
				continue;
			}

			if (words[0] == "element") {
				if (words.size() < 3) {
					throw std::runtime_error("Malformed PLY element line: " + line);
				}
				inVertex = words[1] == "vertex";
				if (inVertex) {
					vertexCount = static_cast<std::size_t>(std::stoull(words[2]));
				}
				continue;
			}

			if (inVertex && words[0] == "property") {
				if (words.size() < 3 || words[1] == "list") {
					throw std::runtime_error("Only scalar vertex properties are supported");
				}

				plyDetail::Property property;
				property.name = words[2];
				property.type = words[1];
				property.offset = rowStride;
				property.size = plyDetail::scalarSize(property.type);
				rowStride += property.size;
				properties.push_back(property);
			}
		}

		if (!sawFormat) {
			throw std::runtime_error("PLY file is missing a format line");
		}
		if (vertexCount == 0) {
			throw std::runtime_error("PLY file has no vertex records");
		}

		std::unordered_map<std::string, std::size_t> lookup;
		for (std::size_t i = 0; i < properties.size(); ++i) {
			lookup[properties[i].name] = i;
		}

		const plyDetail::Property& x = plyDetail::requireProperty(lookup, properties, "x");
		const plyDetail::Property& y = plyDetail::requireProperty(lookup, properties, "y");
		const plyDetail::Property& z = plyDetail::requireProperty(lookup, properties, "z");
		const plyDetail::Property& dcR = plyDetail::requireProperty(lookup, properties, "f_dc_0");
		const plyDetail::Property& dcG = plyDetail::requireProperty(lookup, properties, "f_dc_1");
		const plyDetail::Property& dcB = plyDetail::requireProperty(lookup, properties, "f_dc_2");
		const plyDetail::Property& opacity = plyDetail::requireProperty(lookup, properties, "opacity");
		const plyDetail::Property& scale0 = plyDetail::requireProperty(lookup, properties, "scale_0");
		const plyDetail::Property& scale1 = plyDetail::requireProperty(lookup, properties, "scale_1");
		const plyDetail::Property& scale2 = plyDetail::requireProperty(lookup, properties, "scale_2");
		const plyDetail::Property& rot0 = plyDetail::requireProperty(lookup, properties, "rot_0");
		const plyDetail::Property& rot1 = plyDetail::requireProperty(lookup, properties, "rot_1");
		const plyDetail::Property& rot2 = plyDetail::requireProperty(lookup, properties, "rot_2");
		const plyDetail::Property& rot3 = plyDetail::requireProperty(lookup, properties, "rot_3");

		std::vector<const plyDetail::Property*> shRest;
		for (int i = 0; i < SH_REST_FLOAT_COUNT; ++i) {
			auto it = lookup.find("f_rest_" + std::to_string(i));
			if (it == lookup.end()) {
				break;
			}
			shRest.push_back(&properties[it->second]);
		}
		if (lookup.find("f_rest_" + std::to_string(SH_REST_FLOAT_COUNT)) != lookup.end()) {
			throw std::runtime_error(
				"PLY has more spherical harmonic coefficients than this loads");
		}

		if (shRest.size() % SH_CHANNEL_COUNT != 0) {
			throw std::runtime_error(
				"PLY f_rest_* properties must contain the same count per RGB channel");
		}

		const std::size_t restCountPerChannel = shRest.size() / SH_CHANNEL_COUNT;
		if (restCountPerChannel == 0)
		{
			throw std::runtime_error(
				"Failed to parse - restCountPerChannel was zero!"
			);
		}
		std::vector<unsigned char> row(rowStride);

		
		std::vector<KDopLeaf> leaves;
		for (std::size_t i = 0; i < vertexCount; ++i) {
			file.read(reinterpret_cast<char*>(row.data()), static_cast<std::streamsize>(row.size()));
			if (!file) {
				throw std::runtime_error("PLY ended before all vertex records were read");
			}

			glm::vec3 center(
				plyDetail::readAsFloat(row, x),
				plyDetail::readAsFloat(row, y),
				plyDetail::readAsFloat(row, z)
			);
			std::array<float, SH_FLOAT_COUNT> sphericalHarmonics = {};


			sphericalHarmonics[0] = plyDetail::readAsFloat(row, dcR);
			sphericalHarmonics[1] = plyDetail::readAsFloat(row, dcG);
			sphericalHarmonics[2] = plyDetail::readAsFloat(row, dcB);
			for (std::size_t channel = 0; channel < SH_CHANNEL_COUNT; ++channel) {
				for (std::size_t coeff = 0; coeff < restCountPerChannel; ++coeff) {
					std::size_t plyRestIndex = channel * restCountPerChannel + coeff;
					std::size_t splatCoeff = coeff + 1;
					std::size_t splatIndex = splatCoeff * SH_CHANNEL_COUNT + channel;
					sphericalHarmonics[splatIndex] =
						plyDetail::readAsFloat(row, *shRest[plyRestIndex]);
				}
			}
			float raw_alpha = plyDetail::readAsFloat(row, opacity);
			float alpha = 1.0 / (1.0 + std::exp(-raw_alpha));
			glm::vec3 scale(
				std::exp(plyDetail::readAsFloat(row, scale0)),
				std::exp(plyDetail::readAsFloat(row, scale1)),
				std::exp(plyDetail::readAsFloat(row, scale2))
			);

			glm::vec3 invScale2(
				1.f / (scale.x * scale.x),
				1.f / (scale.y * scale.y),
				1.f / (scale.z * scale.z)
			);

			glm::quat rotation(
				plyDetail::readAsFloat(row, rot0),
				plyDetail::readAsFloat(row, rot1),
				plyDetail::readAsFloat(row, rot2),
				plyDetail::readAsFloat(row, rot3)
			);
			float len = glm::length(rotation);


			rotation = glm::normalize(rotation);

			glm::mat3 R = glm::mat3_cast(rotation);

			float detR = glm::determinant(R);



			float scale_mod = 1;
			glm::mat3 S(1.0f);
			S[0][0] = scale.x * scale_mod;
			S[1][1] = scale.y * scale_mod;
			S[2][2] = scale.z * scale_mod;

			float eps = 0.4;
			float k = std::sqrt(-2.0 * std::log(eps)); // radius of ellipsoid in "S" units
			//std::cout << "Alpha: " << alpha << "\n";
			// ellipsoid semi-axes in world space = k * s_x, k * s_y, k * s_z along R's columns
			glm::vec3 axis0 = k * scale.x * glm::vec3(R[0]); // R's basis vectors (columns)
			glm::vec3 axis1 = k * scale.y * glm::vec3(R[1]);
			glm::vec3 axis2 = k * scale.z * glm::vec3(R[2]);

			// TODO: these AABBs are probably too big
			// AABB half-extents = sum of |component| along each world axis
			glm::vec3 halfExtent(
				std::abs(axis0.x) + std::abs(axis1.x) + std::abs(axis2.x),
				std::abs(axis0.y) + std::abs(axis1.y) + std::abs(axis2.y),
				std::abs(axis0.z) + std::abs(axis1.z) + std::abs(axis2.z)
			);
			
			SphericalHarmonic shMat{
				.degree =  SH_REST_FLOAT_COUNT / (int) restCountPerChannel,
			};
			for (int i = 0; i < SH_COUNT; i++)
			{
				int shIndex = i * SH_CHANNEL_COUNT;
				shMat.sh[i] = glm::vec3(sphericalHarmonics[shIndex], sphericalHarmonics[shIndex + 1], sphericalHarmonics[shIndex + 2]);
			}
			int shIndex = shMats.size();
			shMats.push_back(shMat);

			// doing it this way because it means less new code on the GPU side
			// TODO: change how these work so less wasted memory
			Material mat{
				.color = glm::vec4(0),
				.metallicOrIor = 0,
				.emissiveColor = glm::vec4(0),
				.SHIndex = shIndex
			};
			unsigned int matIndex = materials.size();

			glm::mat3x3 identity(1);

			materials.push_back(mat);
			GaussianSplat gs{
				.center = center,
				.rotation = R,
				.invScale2 = invScale2,
				.alpha = alpha,
				.materialIndex = matIndex,
				.halfExtent = halfExtent
			};
			K14Dop dop = kdopFromGaussianSplat(center, R, scale, k);

			/*
			leaves.push_back(LeafItem{
				.min = center - halfExtent,
				.max = center + halfExtent,
				.center = center,
				.type = PrimType::GAUSSIAN_SPLAT,
				.index = (int) splats.size()
				});

			*/
			leaves.push_back(KDopLeaf{
				.type = PrimType::GAUSSIAN_SPLAT,
				.index = (int) splats.size(),
				.center = center,
				.kDop = dop
				});
			splats.push_back(gs);
		}

		K14Dop outDop;
		return build14DOP(leaves, 0, leaves.size() - 1, outDop);
	}

	void validateBVHNode(int index,	std::unordered_set<int>& visiting, std::unordered_set<int>& visited)
	{
		if (index < 0 || index >= static_cast<int>(bvhNodes.size()))
		{
			throw std::runtime_error(
				"BVH index out of range: " +
				std::to_string(index));
		}

		if (visiting.contains(index))
		{
			throw std::runtime_error(
				"BVH CYCLE DETECTED at node " +
				std::to_string(index));
		}

		if (visited.contains(index))
			return;

		visiting.insert(index);

		const bvhNode& node = bvhNodes[index];

		auto visitChild = [&](const bvhChild& child)
			{
				if (child.type == PrimType::BVH_NODE)
				{
					validateBVHNode(
						child.index,
						visiting,
						visited);
				}
				else if (child.type == PrimType::TRANSFORM)
				{
					int tIndex = transforms[child.index].childIndex;
					assert(transforms[child.index].childPrim == PrimType::BVH_NODE);
					validateBVHNode(
						tIndex,
						visiting,
						visited
					);
				}
				else if (child.type == PrimType::EMPTY)
				{
					if (child.index != -1)
					{
						throw std::runtime_error(
							"EMPTY child has index " +
							std::to_string(child.index));
					}
				}
			};

		visitChild(node.left);
		visitChild(node.right);

		visiting.erase(index);
		visited.insert(index);
	}

	void validateBVH(int root, int depth_to_display)
	{
		std::unordered_set<int> visiting;
		std::unordered_set<int> visited;

		validateBVHNode(root, visiting, visited);

		std::cout
			<< "BVH " << root
			<< " validated: "
			<< visited.size()
			<< " reachable nodes\n";
		printBVH(root, depth_to_display);
		
	}

	void validateWideKDop(int root)
	{
		std::vector<int> stack;
		stack.push_back(root);

		while (!stack.empty())
		{
			int index = stack.back();
			stack.pop_back();

			if (index < 0 || index >= (int)kdopHotNodes.size())
			{
				throw std::runtime_error(
					"Invalid wide KDOP index: " +
					std::to_string(index));
			}

			const auto& node = kdopHotNodes[index];

			for (int i = 0; i < KDOP_WIDTH; ++i)
			{
				uint32_t packed =
					std::bit_cast<uint32_t>(node.min_packed[i].w);

				PrimType type = unpackType(packed);
				int child = unpackIndex(packed);

				if (type == PrimType::KDOP_NODE)
				{
					if (child < 0 ||
						child >= (int)kdopHotNodes.size())
					{
						std::cerr
							<< "INVALID WIDE CHILD\n"
							<< "parent = " << index << "\n"
							<< "slot = " << i << "\n"
							<< "child = " << child << "\n";

						throw std::runtime_error(
							"Invalid KDOP child");
					}

					stack.push_back(child);
				}
			}
		}
	}

	void printBVH(int root, int depth_to_display)
	{
		if (depth_to_display < 0) return;
		bvhNode node = bvhNodes[root];
		std::cout << "Node: " << root << " has children: \n" <<
			"left: " << node.left.index << " (type: " << node.left.type << ")\n" <<
			"right: " << node.right.index << " (type: " << node.right.type << ")\n";

		if (node.left.index >= 0)
		{
			int index = node.left.index;
			Transform t;
			switch (node.left.type)
			{
				case BVH_NODE:
					printBVH(index, depth_to_display - 1);
					break;
				case TRANSFORM:
					t = transforms[index];
					if (t.childPrim == BVH_NODE)
						printBVH(t.childIndex, depth_to_display - 1);
					break;
				default:
					break;
			}

		}
		if (node.right.index >= 0)
		{
			int index = node.right.index;
			Transform t;
			switch (node.right.type)
			{
			case BVH_NODE:
				printBVH(index, depth_to_display - 1);
				break;
			case TRANSFORM:
				t = transforms[index];
				if (t.childPrim == BVH_NODE)
					printBVH(t.childIndex, depth_to_display - 1);
				break;
			default:
				break;
			}

		}
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
		shaderData.camDir = glm::normalize(camera.direction);
		shaderData.bounceCount = bounces;
		const bool firstBounce = bounces == 0;
		const bool finalBounce = bounces == maxBounces - 1;

		shaderData.resetRays = firstBounce;
		++bounces;
		
		ShaderData uploadedShaderData = shaderData;
		memcpy(shaderDataBuffers[frameIndex].allocationInfo.pMappedData, &shaderData, sizeof(ShaderData));

		if (finalBounce)
		{
			++shaderData.frameCount;
			bounces = 0;
		}
		// historyReadIndex is either 0 or 1, so this is either 1 or 0
		int historyWriteIndex = 1 - historyReadIndex;
		updateHistoryDescriptor();
		


		// Build command buffer
		auto cb = commandBuffers[frameIndex];
		chk(vkResetCommandBuffer(cb, 0));

		VkCommandBufferBeginInfo cbBI{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT 
		};
		chk(vkBeginCommandBuffer(cb, &cbBI));

		// shader data
		VkBufferMemoryBarrier2 shaderDataBarrier{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
			.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT,

			.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
							VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
							VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,

			.buffer = shaderDataBuffers[frameIndex].buffer,
			.offset = 0,
			.size = sizeof(ShaderData)
		};

		VkDependencyInfo shaderDataDependency{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &shaderDataBarrier
		};

		vkCmdPipelineBarrier2(cb, &shaderDataDependency);


		// swapchain image
		VkImageMemoryBarrier2 frameImageToAttachment{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = frameImageLayout,
			.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.image = frameImage,
			.subresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1
			}
		};

		VkDependencyInfo frameImageToAttachmentDI{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &frameImageToAttachment
		};

		vkCmdPipelineBarrier2(cb, &frameImageToAttachmentDI);
		frameImageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

		// history read
		VkImageMemoryBarrier2 historyReadBarrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
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
		};

		VkDependencyInfo historyReadDI{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &historyReadBarrier
		};

		vkCmdPipelineBarrier2(cb, &historyReadDI);

		historyImageLayouts[historyReadIndex] =	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// depth
		VkImageMemoryBarrier2 depthBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
				.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
				.image = depthImage,
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
		};
		VkDependencyInfo depthDI{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &depthBarrier
		};
		vkCmdPipelineBarrier2(cb, &depthDI);

		// rendering
		VkRenderingAttachmentInfo colorAttachmentInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = frameImageView,
			.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue{.color{0.f, 0.f, 0.f, 1.f}}
		};
		VkRenderingAttachmentInfo depthAttachmentInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = depthImageView,
			.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.clearValue{.depthStencil{1.f, 0}}
		};

		VkRenderingInfo renderingInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea{
				.extent{
					.width = static_cast<uint32_t>(windowSize.x),
					.height = static_cast<uint32_t>(windowSize.y)
				}
			},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentInfo,
			.pDepthAttachment = &depthAttachmentInfo
		};

		vkCmdBeginRendering(cb, &renderingInfo);
		VkViewport vp{
			.width = static_cast<float>(windowSize.x), 
			.height = static_cast<float>(windowSize.y), 
			.minDepth = 0.0f, 
			.maxDepth = 1.0f
		};
		vkCmdSetViewport(cb, 0, 1, &vp);
		VkRect2D scissor{
			.extent{
				.width = static_cast<uint32_t>(windowSize.x), 
				.height = static_cast<uint32_t>(windowSize.y) 
			}
		};
		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdSetScissor(cb, 0, 1, &scissor);
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
		VkDeviceSize vOffset{ 0 };
		vkCmdBindVertexBuffers(cb, 0, 1, &vBuffer, &vOffset);
		vkCmdBindIndexBuffer(cb, vBuffer, vBufSize, VK_INDEX_TYPE_UINT16);
		vkCmdPushConstants(
			cb, 
			pipelineLayout, 
			VK_SHADER_STAGE_FRAGMENT_BIT, 
			0, 
			sizeof(ShaderData),  // <- this use to be VKPointer
			&uploadedShaderData // <- used to be pointer to buffer
		);
		vkCmdDrawIndexed(cb, indexCount, 1, 0, 0, 0);
		vkCmdEndRendering(cb);

		// transition frame image
		VkImageMemoryBarrier2 frameImageToTransfer{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT |
							VK_PIPELINE_STAGE_2_COPY_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.image = frameImage,
			.subresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1
			}
		};

		VkDependencyInfo frameImageToTransferDI{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &frameImageToTransfer
		};

		vkCmdPipelineBarrier2(cb, &frameImageToTransferDI);

		frameImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		// transition swapchain to transfer dst
		VkImageMemoryBarrier2 swapchainToTransfer{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
			.srcAccessMask = VK_ACCESS_2_NONE,
			.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.oldLayout = swapchainImageLayouts[imageIndex],
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.image = swapchainImages[imageIndex],
			.subresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1
			}
		};

		VkDependencyInfo swapchainToTransferDI{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &swapchainToTransfer
		};

		vkCmdPipelineBarrier2(cb, &swapchainToTransferDI);

		swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		// blit swapchain and frameImage
		VkImageBlit blitRegion{
			.srcSubresource{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1
			},
			.srcOffsets = {
				{0, 0, 0},
				{
					static_cast<int32_t>(windowSize.x),
					static_cast<int32_t>(windowSize.y),
					1
				}
			},

			.dstSubresource{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1
			},
			.dstOffsets = {
				{0, 0, 0},
				{
					static_cast<int32_t>(windowSize.x),
					static_cast<int32_t>(windowSize.y),
					1
				}
			}
		};

		vkCmdBlitImage(
			cb, 
			frameImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blitRegion, VK_FILTER_NEAREST
		);

		if (finalBounce)
		{
			VkImageMemoryBarrier2 historyWriteToTransfer{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.oldLayout = historyImageLayouts[historyWriteIndex],
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.image = historyImages[historyWriteIndex],
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			};

			VkDependencyInfo historyWriteToTransferDI{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &historyWriteToTransfer
			};

			vkCmdPipelineBarrier2(cb, &historyWriteToTransferDI);
			historyImageLayouts[historyWriteIndex] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			// copy
			VkImageCopy historyCopyRegion{
				.srcSubresource{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.layerCount = 1
				},
				.srcOffset{0, 0, 0},
				.dstSubresource{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.layerCount = 1
				},
				.dstOffset{0, 0, 0},
				.extent{
					.width = static_cast<uint32_t>(windowSize.x),
					.height = static_cast<uint32_t>(windowSize.y),
					.depth = 1
				}
			};

			vkCmdCopyImage( cb,
				frameImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				historyImages[historyWriteIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &historyCopyRegion
			);
			VkImageMemoryBarrier2 historyReady{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.image = historyImages[historyWriteIndex],
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			};

			VkDependencyInfo historyReadyDI{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &historyReady
			};

			vkCmdPipelineBarrier2(cb, &historyReadyDI);

			historyImageLayouts[historyWriteIndex] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			historyReadIndex = historyWriteIndex;
		}
		
		VkImageMemoryBarrier2 presentBarrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
			.dstAccessMask = VK_ACCESS_2_NONE,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.image = swapchainImages[imageIndex],
			.subresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1
			}
		};

		VkDependencyInfo presentDI{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &presentBarrier
		};

		vkCmdPipelineBarrier2(cb, &presentDI);

		swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		

		chk(vkEndCommandBuffer(cb));
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &imageAcquiredSemaphores[frameIndex],
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &cb,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &renderCompleteSemaphores[imageIndex]
		};

		chk(vkQueueSubmit(queue, 1, &submitInfo, fences[frameIndex]));
		VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &renderCompleteSemaphores[imageIndex],
			.swapchainCount = 1,
			.pSwapchains = &swapchain,
			.pImageIndices = &imageIndex
		};

		chkSwapchain(vkQueuePresentKHR(queue, &presentInfo));

		frameIndex = (frameIndex + 1) % maxFramesInFlight;
		
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
			bounces = 0;
		}

		if (updateSwapchain) {
			bounces = 0;
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
			swapchainImageLayouts.assign(imageCount, VK_IMAGE_LAYOUT_UNDEFINED);
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

			destroyFrameImage();
			initFrameImage();

			resizeRaysBuffer(windowSize);

			// TODO: destroy and reinit 

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
	};
};