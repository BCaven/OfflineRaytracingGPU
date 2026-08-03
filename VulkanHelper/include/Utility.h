#pragma once

#include "VulkanHelper.h"

struct CameraWrapper
{
	glm::vec3 origin = glm::vec3(-1, 1, 0);
	glm::vec3 direction = glm::vec3(1, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);
};


struct Camera
{
	glm::mat3x3 lookAtMatrix;
	glm::vec3 origin;
};

struct Sphere
{
	glm::vec3 center;
	float radius;
	glm::vec4 color = glm::vec4(1.0);
};
struct ShaderData 
{
	glm::vec2 windowMax;
	int numSpheres;
	int frameCount = 0;
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

struct ResourceBinding { 
	uint32_t binding; 
	uint32_t set; 
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