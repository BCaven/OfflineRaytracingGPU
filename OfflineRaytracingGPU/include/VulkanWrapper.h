#pragma once

#include "VulkanHelper.h"
#include "Utility.h"
#include "KeyInputs.h"
#include "ImageReadback.h"
#include <unordered_set>
#include <bit>
#include <execution>
#include <limits>
#include <future>

constexpr uint32_t maxBounces{ 10 };
constexpr uint32_t maxFramesInFlight{ 2 };
constexpr uint32_t numHistoryFrames{ 2 };
constexpr uint32_t objectTypes{ 15 };
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
	VkPipeline graphicsPipeline{ VK_NULL_HANDLE };
	VkPipeline computePipeline{ VK_NULL_HANDLE };
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

	std::unordered_map<std::string, ImageBinding> imageBindings{};

	VkBuffer countersBuffer{ VK_NULL_HANDLE };

	KeyInputs& inputs = KeyInputs::inputHandler();

	uint64_t lastTime{ SDL_GetTicks() };
	bool quit{ false };

	bool          saveRequested = false;   // set from your UI/key handler
	int           readbackFrame = -1;      // frameIndex whose command buffer holds the copy

	int writtenImages = 0;

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
	std::vector<Ray> rayQueueA;
	std::vector<Ray> rayQueueB;
	std::vector<KDopNode> kdopNodes;
	std::vector<KDopNodeHot> kdopHotNodes;
	std::vector<K14DopNodeCold> kdopColdNodes;
	std::vector< tbvhNode> threadedNodes;
	std::vector<glm::mat4x4> threadedMatrices;
	CameraWrapper camera;
	ShaderData shaderData{};

	uint32_t deviceIndex{ 0 };

	ImageReadback readback;
	std::string   savePath = "output.exr";
	int			  numFramesPerFile = 100;

	// time in the world - frames are per-moment in time
	float time = 0;
	float time_delta = 1;

	// auto render frames
	int numImagesPerSequence = 1;


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

		for (auto& [name, binding] : imageBindings)
		{
			vkDestroyImageView(device, binding.view, nullptr);
			vmaDestroyImage(allocator, binding.image, binding.imageAllocation);
		}
		imageBindings.clear();

		readback.destroy();

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
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipeline(device, computePipeline, nullptr);
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

	bool getFences();

	void clearPrimitives();

	void reloadPrimitives();

	void init();

	void initVulkan();

	void initVMA();

	void initSwapchain();

	void initShaderData();

	void initFrameImage();

	void destroyFrameImage();

	void initHistoryImages(bool firstTime = true);

	void updateHistoryDescriptor();

	void destroyHistoryImages();

	void initVertices();

	void initShaderCompiler();

	void initPipelineLayout();

	void initComputePipeline();

	void initPipeline();

	void initBindings();

	void loadStorageImage(std::string bindingName, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags extraUsage = 0);

	void updateImageDescriptors(std::string bindingName);

	template<typename T>
	void loadStructuredBuffer(std::string bindingName, const std::vector<T>& data, size_t elementCount, VkBufferUsageFlags extraUsage = 0);

	template <typename T>
	void loadStructuredBuffer(std::string bindingName, const std::vector<T>& data);

	void updateStructuredBufferDescriptors(std::string bindingName);

	void resizeRaysBuffer(glm::vec2 windowSize);

	void transformAABB(const glm::mat4 M, const glm::vec3& min, const glm::vec3& max, glm::vec3& outMin, glm::vec3& outMax);

	void getChildMinMax(const Transform& transform, glm::vec3& min, glm::vec3& max);

	void getTransformMinMax(const Transform& transform, glm::vec3& min, glm::vec3& max);

	void getSceneBounds(glm::vec3& sceneMin, glm::vec3& sceneMax);

	int flattenKDop(int rootIndex);

	void packBvhNodes();

	PackedRef loadBVH();

	int build14DOPWorker(
		std::vector<KDopLeaf>& leaves,
		int start, int end,
		K14Dop& outKDop,
		std::atomic<int>& nodeCounter,
		TaskPool& pool,
		int grainSize);

	int build14DOP_parallel(std::vector<KDopLeaf>& leaves, int start, int end, K14Dop& outKDop);

	int build14DOP(std::vector<KDopLeaf>& leaves, int start, int end, K14Dop& outKDop);

	int buildSAH(std::vector<LeafItem>& leaves, int start, int end, glm::vec3& outMin, glm::vec3& outMax);

	int buildChildBVH(std::vector<LeafItem>& leaves);

	ThreadedWrapper buildThreadedBVH(PackedRef root, int transformIndex = -1);

	PackedRef loadCollection(std::vector<PackedRef> references);

	PackedRef loadTransform(glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale, PackedRef childRef);

	PackedRef loadObj(std::string filepath, unsigned int materialIndex);

	PackedRef loadSplat(std::string filepath);

	PackedRef loadSplat2(std::string filepath);

	void validateBVHNode(PackedRef childRef, std::unordered_set<int>& visiting, std::unordered_set<int>& visited);

	void validateBVH(PackedRef rootRef, int depth_to_display);

	void validateWideKDop(int root);

	void printBVH(int root, int depth_to_display);

	VkBufferMemoryBarrier2 bufferBarrier(
		VkBuffer buffer,
		VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
		VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess,
		VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

	bool draw();	
};