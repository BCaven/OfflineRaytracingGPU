#pragma once

#include "VulkanHelper.h"

constexpr float PI = 3.1415926535;

struct CameraWrapper
{
	glm::vec3 origin = glm::vec3(-1, 1, 0);
	glm::vec3 direction = glm::vec3(0, 0, 1);
	glm::vec3 right = glm::vec3(-1, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);
	float pitch = 0;
	float yaw = 0;
	float roll = 0;
	float fov = 40;
};

// TODO later: make sure the strides of all Vulkan and Slang data structures match and best fit the blocks

struct Camera
{
	glm::mat3x3 rotationMatrix;
	glm::vec3 origin;
	float fov;
};

struct Material
{
	glm::vec4 color;
	float metallicOrIor;
};

struct Sphere
{
	glm::vec3 center;
	float radius;
	unsigned int materialIndex;
};
struct ShaderData 
{
	glm::vec2 windowMax;
	int numSpheres;
	int frameCount = 0;
	glm::vec4 backgroundColor = glm::vec4(0.3, 0.5, 1.0, 1.0);
	Camera camera;
};
struct ShaderDataBuffer 
{
	VmaAllocation allocation{ VK_NULL_HANDLE };
	VmaAllocationInfo allocationInfo{};
	VkBuffer buffer{ VK_NULL_HANDLE };
	VkDeviceAddress deviceAddress{};
};
struct Texture 
{
	VmaAllocation allocation{ VK_NULL_HANDLE };
	VkImage image{ VK_NULL_HANDLE };
	VkImageView view{ VK_NULL_HANDLE };
	VkSampler sampler{ VK_NULL_HANDLE };
};

struct Vertex 
{
	glm::vec3 pos;
};

struct ResourceBinding 
{ 
	uint32_t binding; 
	uint32_t set; 
};
struct StructuredBufferBinding
{
	VmaAllocationInfo stagingAllocInfo;
	VkBufferCreateInfo bufferInfo;
	VkBuffer buffer;
	VkBuffer stagingBuffer;
	VmaAllocation bufferAllocation;
	VmaAllocation stagingAllocation;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
};



static inline void chk(VkResult result) 
{
	if (result != VK_SUCCESS) {
		std::cerr << "Vulkan call returned an error (" << result << ")\n";
		exit(result);
	}
}

static inline void chk(bool result) 
{
	if (!result) {
		std::cerr << "Call returned an error\n";
		exit(result);
	}
}