#pragma once

#include "VulkanHelper.h"

constexpr float PI = 3.1415926535;
constexpr int SH_COUNT = 16;
constexpr int SH_CHANNEL_COUNT = 3;
constexpr int SH_FLOAT_COUNT = SH_COUNT * SH_CHANNEL_COUNT;
constexpr int SH_REST_FLOAT_COUNT = SH_FLOAT_COUNT - SH_CHANNEL_COUNT;
constexpr int SH_PACKED_VEC4_COUNT = SH_FLOAT_COUNT / 4;

static_assert(SH_FLOAT_COUNT % 4 == 0,
	"Spherical harmonic coefficients must pack into vec4 attributes");


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

struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;
	glm::vec4 scatter;
	glm::vec4 emission;
};

// TODO: later this will be removed since the BVH will exclusively be on the GPU
enum PrimType
{
	SPHERE,
	TRIANGLE,
	GAUSSIAN_SPLAT,
	TRANSFORM,
	BVH_NODE,
	EMPTY
};

struct bvhChild
{
	glm::vec3 min;
	glm::vec3 max;
	PrimType type;
	int index;
};

struct bvhNode
{
	uint64_t mortonCode;
	bvhChild left;
	bvhChild right;
};

struct LeafItem
{
	glm::vec3 min, max, center;
	PrimType type;
	int index;
};

struct Material
{
	glm::vec4 color;
	float metallicOrIor;
	glm::vec4 emissiveColor = glm::vec4(0);
	int SHIndex = -1;
};

struct GaussianSplat
{
	glm::vec3 center;
	glm::mat3x3 rotation;
	glm::vec3 invScale2;
	float alpha;
	unsigned int materialIndex;
	glm::vec3 halfExtent;
};

struct SphericalHarmonic
{
	glm::vec3 sh[SH_COUNT];
	int degree = 0;
};

struct Transform
{
	glm::mat4 matrix;
	glm::mat4 invMatrix;
	PrimType childPrim;
	int childIndex;
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
	int frameCount = 0;
	glm::vec4 backgroundColor = glm::vec4(0.3, 0.5, 1.0, 1.0);
	Camera camera;
	unsigned int bvhRoot;
	bool resetRays;
	glm::vec3 camDir;
	int bounceCount;
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
		std::cerr << "Vulkan error: " << result << "\n";
		throw std::runtime_error("Vulkan call returned an error");
	}
}

static inline void chk(bool result) 
{
	if (!result) {
		throw std::runtime_error("Call returned error!");
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

static inline float surfaceArea(const glm::vec3& extent)
{
	glm::vec3 e = make_aabb_max(extent, glm::vec3(0.f));
	return 2.f * (e.x * e.y + e.y * e.z + e.z * e.x);
}

static inline int BuildBVHRecursive(std::vector<bvhNode>& nodes, int begin, int end)
{
	if (end - begin == 1)
		return begin;

	int mid = (begin + end) / 2;
	int left = BuildBVHRecursive(nodes, begin, mid);
	int right = BuildBVHRecursive(nodes, mid, end);

	bvhNode parent{
		// morton code doesnt matter here since interior nodes arent sorted
		.left = bvhChild{
			.min = make_aabb_min(nodes[left].left.min, nodes[left].right.min),
			.max = make_aabb_max(nodes[left].left.max, nodes[left].right.max),
			.type = PrimType::BVH_NODE,
			.index = left
		},
		.right = bvhChild{
			.min = make_aabb_min(nodes[right].left.min, nodes[right].right.min),
			.max = make_aabb_max(nodes[right].left.max, nodes[right].right.max),
			.type = PrimType::BVH_NODE,
			.index = right
		}
	};
	nodes.push_back(parent);
	return nodes.size() - 1;
}

namespace plyDetail {

	struct Property {
		std::string name;
		std::string type;
		std::size_t offset = 0;
		std::size_t size = 0;
	};

	inline std::size_t scalarSize(const std::string& type) {
		if (type == "char" || type == "uchar" || type == "int8" || type == "uint8") {
			return 1;
		}
		if (type == "short" || type == "ushort" || type == "int16" || type == "uint16") {
			return 2;
		}
		if (type == "int" || type == "uint" || type == "float" || type == "int32" || type == "uint32" ||
			type == "float32") {
			return 4;
		}
		if (type == "double" || type == "float64") {
			return 8;
		}
		throw std::runtime_error("Unsupported PLY scalar type: " + type);
	}

	template <typename T> inline T readScalar(const unsigned char* bytes) {
		T value;
		std::memcpy(&value, bytes, sizeof(T));
		return value;
	}

	inline float readAsFloat(const std::vector<unsigned char>& row, const Property& property) {
		const unsigned char* ptr = row.data() + property.offset;
		const std::string& type = property.type;

		if (type == "float" || type == "float32")
			return readScalar<float>(ptr);
		if (type == "double" || type == "float64")
			return static_cast<float>(readScalar<double>(ptr));
		if (type == "char" || type == "int8")
			return static_cast<float>(readScalar<std::int8_t>(ptr));
		if (type == "uchar" || type == "uint8")
			return static_cast<float>(readScalar<std::uint8_t>(ptr));
		if (type == "short" || type == "int16")
			return static_cast<float>(readScalar<std::int16_t>(ptr));
		if (type == "ushort" || type == "uint16")
			return static_cast<float>(readScalar<std::uint16_t>(ptr));
		if (type == "int" || type == "int32")
			return static_cast<float>(readScalar<std::int32_t>(ptr));
		if (type == "uint" || type == "uint32")
			return static_cast<float>(readScalar<std::uint32_t>(ptr));

		throw std::runtime_error("Unsupported PLY scalar type: " + type);
	}

	inline std::vector<std::string> splitWords(const std::string& line) {
		std::vector<std::string> words;
		std::string word;
		for (char c : line) {
			if (c == ' ' || c == '\t' || c == '\r') {
				if (!word.empty()) {
					words.push_back(word);
					word.clear();
				}
			}
			else {
				word.push_back(c);
			}
		}
		if (!word.empty()) {
			words.push_back(word);
		}
		return words;
	}

	inline const Property& requireProperty(const std::unordered_map<std::string, std::size_t>& lookup,
		const std::vector<Property>& properties,
		const std::string& name) {
		auto it = lookup.find(name);
		if (it == lookup.end()) {
			throw std::runtime_error("Missing required vertex property: " + name);
		}
		return properties[it->second];
	}

} // namespace plyDetail
