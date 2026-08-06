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

// TODO: later this will be removed since the BVH will exclusively be on the GPU
enum PrimType
{
	MATERIAL,
	SPHERE,
	TRIANGLE,
	BVH_NODE,
	EMPTY
};

struct bvhNode
{
	glm::vec3 min;
	glm::vec3 max;
	uint64_t mortonCode;
	PrimType rightPrim;
	PrimType leftPrim;
	int leftIndex;
	int rightIndex;
};

struct Material
{
	glm::vec4 color;
	float metallicOrIor;
	glm::vec4 emissiveColor = glm::vec4(0);
};

struct Sphere
{
	glm::vec3 center;
	float radius;
	unsigned int materialIndex;
};
struct Triangle
{
	glm::vec3 v0;
	glm::vec3 v1;
	glm::vec3 v2;
	glm::vec3 normal;
	float d;
	unsigned int materialIndex;
};

struct ShaderData 
{
	glm::uvec2 windowMax;
	unsigned int numSpheres;
	unsigned int numTris;
	int frameCount = 0;
	glm::vec4 backgroundColor = glm::vec4(0.3, 0.5, 1.0, 1.0);
	Camera camera;
	unsigned int bvhRoot;
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


static inline uint64_t separate_bits_64(uint64_t n)
{
	n &= 0b0000000000000000000000000000000000000000001111111111111111111111ull;
	n = (n ^ (n << 32)) & 0b1111111111111111000000000000000000000000000000001111111111111111ull;
	n = (n ^ (n << 16)) & 0b0000000011111111000000000000000011111111000000000000000011111111ull;
	n = (n ^ (n << 8)) & 0b1111000000001111000000001111000000001111000000001111000000001111ull;
	n = (n ^ (n << 4)) & 0b0011000011000011000011000011000011000011000011000011000011000011ull;
	n = (n ^ (n << 2)) & 0b1001001001001001001001001001001001001001001001001001001001001001ull;
	return n;
};

static inline uint64_t morton64_encode(glm::vec3 p)
{
	constexpr uint64_t Scale = (1ull << 21) - 1;

	glm::u64vec3 q = glm::u64vec3(p * float(Scale));

	return separate_bits_64(q.x)
		| (separate_bits_64(q.y) << 1)
		| (separate_bits_64(q.z) << 2);
}

static inline uint64_t compute_morton_code(const glm::vec3& center, const glm::vec3& sceneMin, const glm::vec3& invSceneExtent)
{
	glm::vec3 normCenter = (center - sceneMin) * invSceneExtent;
	normCenter = glm::clamp(
		normCenter,
		glm::vec3(0.f),
		glm::vec3(0.99999f)
	);
	return morton64_encode(normCenter);
}

static inline glm::vec3 make_aabb_max(glm::vec3 p1, glm::vec3 p2)
{
	return glm::vec3(
		p1.x > p2.x ? p1.x : p2.x,
		p1.y > p2.y ? p1.y : p2.y,
		p1.z > p2.z ? p1.z : p2.z
	);
}
static inline glm::vec3 make_aabb_min(glm::vec3 p1, glm::vec3 p2)
{
	return glm::vec3(
		p1.x < p2.x ? p1.x : p2.x,
		p1.y < p2.y ? p1.y : p2.y,
		p1.z < p2.z ? p1.z : p2.z
	);
}

static inline int BuildBVHRecursive(std::vector<bvhNode>& nodes, int begin, int end)
{
	if (end - begin == 1)
		return begin;

	int mid = (begin + end) / 2;
	int left = BuildBVHRecursive(nodes, begin, mid);
	int right = BuildBVHRecursive(nodes, mid, end);

	bvhNode parent{
		.min = make_aabb_min(nodes[left].min, nodes[right].min),
		.max = make_aabb_max(nodes[left].max, nodes[right].max),
		.rightPrim = PrimType::BVH_NODE,
		.leftPrim = PrimType::BVH_NODE,
		.leftIndex = left,
		.rightIndex = right
	};
	nodes.push_back(parent);
	return nodes.size() - 1;
}