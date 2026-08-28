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

constexpr int KDOP_AXIS_COUNT = 7;
constexpr int KDOP_WIDTH = 8;
constexpr float KDOP_EPSILON = 0.01;
constexpr float invSqrt2 = 0.707106781187;
constexpr float invSqrt3 = 0.57735026919;
static constexpr glm::vec3 KDOP_DIRECTIONS[7] =
{
	{ 1, 0, 0 },
	{ 0, 1, 0 },
	{ 0, 0, 1 },
	{invSqrt3, invSqrt3, invSqrt3},
	{invSqrt3, invSqrt3,-invSqrt3 },
	{invSqrt3,-invSqrt3, invSqrt3 },
	{ invSqrt3,-invSqrt3,-invSqrt3 }
};

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

using PackedRef = unsigned int;

// TODO: later this will be removed since the BVH will exclusively be on the GPU
enum PrimType : unsigned int
{
	SPHERE,
	TRIANGLE,
	GAUSSIAN_SPLAT,
	TRANSFORM,
	BVH_NODE,
	KDOP_NODE,
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
struct K14Dop
{
	float min[7];
	float max[7];
};
struct KDopNode // the version that will eventually get loaded on the GPU
{
	unsigned int packedIndexType_left;
	unsigned int packedIndexType_right;

	K14Dop kDop_left;
	K14Dop kDop_right;
};
struct KDopLeaf // stored primitive with pre-calculated kDop
{
	PrimType type;
	int index;
	glm::vec3 center;
	K14Dop kDop;
};

struct K14DopNodeCold
{
	glm::vec4 min[KDOP_WIDTH], max[KDOP_WIDTH];
};
struct KDopNodeHot
{
	glm::vec4 min_packed[KDOP_WIDTH]; // w is packed index/type
	glm::vec4 max[KDOP_WIDTH]; // w reserved
};


struct bvhPacked
{
	glm::vec4 leftMin_leftPacked; // xyz left.min, w = packed left.type / left.index
	glm::vec4 leftMax_rightPacked; // xyz left.max, w = packed right.type / right.index
	glm::vec4 rightMin_reserved0; // xyz right.min, w unused
	glm::vec4 rightMax_reserved1; // same
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
	PackedRef sceneRoot;
	unsigned int resetRays;
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

K14Dop makeEmptyKDop()
{
	K14Dop kdop;
	for (int i = 0; i < KDOP_AXIS_COUNT; ++i)
	{
		kdop.min[i] = FLT_MAX;
		kdop.max[i] = -FLT_MAX;
	}
	return kdop;
}

K14Dop mergeKDop(const K14Dop& a, const K14Dop& b)
{
	K14Dop result;

	for (int i = 0; i < KDOP_AXIS_COUNT; ++i)
	{
		result.min[i] = std::min(a.min[i], b.min[i]);
		result.max[i] = std::max(a.max[i], b.max[i]);
	}
	return result;
}

static inline float surfaceArea(const glm::vec3& extent)
{
	glm::vec3 e = make_aabb_max(extent, glm::vec3(0.f));
	return 2.f * (e.x * e.y + e.y * e.z + e.z * e.x);
}

static inline void kdopGetAABB(const K14Dop& kdop, glm::vec3& outMax, glm::vec3& outMin)
{
	for (int i = 0; i < 8; ++i)
	{
		outMax = make_aabb_max(outMax, KDOP_DIRECTIONS[i] * kdop.max[i]);
		outMin = make_aabb_min(outMin, KDOP_DIRECTIONS[i] * kdop.min[i]);
	}
}
inline float dot_n3(const glm::vec3& v) { return  v.x + v.y + v.z; } // (1,1,1)
inline float dot_n4(const glm::vec3& v) { return  v.x + v.y - v.z; } // (1,1,-1)
inline float dot_n5(const glm::vec3& v) { return  v.x - v.y + v.z; } // (1,-1,1)
inline float dot_n6(const glm::vec3& v) { return  v.x - v.y - v.z; } // (1,-1,-1)

static inline float kdopSAHCost(const K14Dop& kdop)
{
	glm::vec3 aabbMax(-FLT_MAX), aabbMin(FLT_MAX);
	kdopGetAABB(kdop, aabbMax, aabbMin);
	glm::vec3 diag = aabbMax - aabbMin;
	float aabbSA = surfaceArea(diag);
	/*
	kdop indicies
	0 : x
	1 : y
	2 : z
	3-7 diag
	
	
	*/
	float d[8];
	glm::vec3 kdop_aabb_max(kdop.max[0], kdop.max[1], kdop.max[2]);
	glm::vec3 kdop_aabb_min(kdop.min[0], kdop.min[1], kdop.min[2]);

	d[0] = kdop.min[3] - dot_n3(kdop_aabb_min);
	d[1] = dot_n3(kdop_aabb_max) - kdop.max[3];
	d[2] = kdop.min[4] - dot_n4(kdop_aabb_min);
	d[3] = dot_n4(kdop_aabb_max) - kdop.max[4];
	d[4] = kdop.min[5] - dot_n5(kdop_aabb_min);
	d[5] = dot_n5(kdop_aabb_max) - kdop.max[5];
	d[6] = kdop.min[6] - dot_n6(kdop_aabb_min);
	d[7] = dot_n6(kdop_aabb_max) - kdop.max[6];

	constexpr float kCornerConst = 1.9019237886466845f; // (9 - 3*sqrt(3)) / 2
	constexpr float kCorrConst = 0.2679491924311227f; // 2 - sqrt(3)
	constexpr float kSqrt3 = 1.7320508075688772f;


	float cornerArea = 0.0f;
	for (int i = 0; i < 8; ++i) cornerArea += d[i] * d[i];
	
	cornerArea *= kCornerConst;


	// Eq. 3/4: correction for pairwise overlap of cuts along each of the 12 AABB edges.
	float s[12];
	s[0] = std::max(0.0f, d[0] + d[7] - diag.x);
	s[1] = std::max(0.0f, d[1] + d[6] - diag.x);
	s[2] = std::max(0.0f, d[2] + d[5] - diag.x);
	s[3] = std::max(0.0f, d[3] + d[4] - diag.x);
	s[4] = std::max(0.0f, d[0] + d[4] - diag.y);
	s[5] = std::max(0.0f, d[1] + d[5] - diag.y);
	s[6] = std::max(0.0f, d[2] + d[6] - diag.y);
	s[7] = std::max(0.0f, d[3] + d[7] - diag.y);
	s[8] = std::max(0.0f, d[0] + d[2] - diag.z);
	s[9] = std::max(0.0f, d[1] + d[3] - diag.z);
	s[10] = std::max(0.0f, d[4] + d[6] - diag.z);
	s[11] = std::max(0.0f, d[5] + d[7] - diag.z);

	float cornerCorrection = 0.0f;
	for (int i = 0; i < 12; ++i) cornerCorrection += s[i] * s[i];
	cornerCorrection *= kCorrConst;

	return aabbSA - cornerArea + cornerCorrection;
}

static inline K14Dop kdopFromGaussianSplat(const glm::vec3& center, const glm::mat3& rotation, const glm::vec3& scale, float kSigma)
{
	K14Dop dop;
	for (int i = 0; i < KDOP_AXIS_COUNT; ++i)
	{
		const glm::vec3& n = KDOP_DIRECTIONS[i];
		glm::vec3 localN = glm::transpose(rotation) * n;
		float extent = kSigma * glm::length(scale * localN);
		float centerProj = glm::dot(center, n);
		dop.min[i] = centerProj - extent;
		dop.max[i] = centerProj + extent;
	}
	return dop;
}

static inline K14Dop kdopFromTriangle(const Triangle& t)
{
	K14Dop dop;
	for (int i = 0; i < KDOP_AXIS_COUNT; ++i)
	{
		const glm::vec3& n = KDOP_DIRECTIONS[i];
		float p0 = glm::dot(n, t.v0);
		float p1 = glm::dot(n, t.v1);
		float p2 = glm::dot(n, t.v2);
		float minDop = std::min(p0, std::min(p1, p2));
		float maxDop = std::max(p0, std::max(p1, p2));
		dop.max[i] = maxDop + KDOP_EPSILON;
		dop.min[i] = minDop - KDOP_EPSILON;
	}
	return dop;
}

static inline K14Dop kdopFromAABB(
	const glm::vec3& min,
	const glm::vec3& max)
{
	K14Dop dop;

	for (int i = 0; i < KDOP_AXIS_COUNT; ++i)
	{
		const glm::vec3& n = KDOP_DIRECTIONS[i];

		glm::vec3 center = (min + max) * 0.5f;
		glm::vec3 extent = (max - min) * 0.5f;

		float centerProj = glm::dot(n, center);

		// Assuming KDOP_DIRECTIONS are unit vectors.
		float radius =
			std::abs(n.x) * extent.x +
			std::abs(n.y) * extent.y +
			std::abs(n.z) * extent.z;

		dop.min[i] = centerProj - radius;
		dop.max[i] = centerProj + radius;
	}

	return dop;
}

static inline K14Dop kdopFromSphere(const Sphere& s)
{
	K14Dop dop;
	// for now:
	dop = kdopFromAABB(s.center - s.radius, s.center + s.radius);
	for (int i = 0; i < KDOP_AXIS_COUNT; ++i)
	{
		// TODO: actually do this
	}

	return dop;
}


static inline PackedRef packChild(PrimType type, int index)
{
	return (unsigned int(type) << 28) | (unsigned int(index) & 0x0FFFFFFFu);
}

int unpackIndex(unsigned int packed)
{
	return int(packed & 0x0FFFFFFFu);
}
PrimType unpackType(unsigned int packed)
{
	return PrimType(packed >> 28);
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


// reflection api validation:
static inline void checkField(
	slang::VariableLayoutReflection* field,
	size_t expectedOffset,
	size_t expectedCount,
	size_t expectedElementSize)
{
	using namespace slang;

	const size_t offset =
		field->getOffset(ParameterCategory::Uniform);

	if (offset != expectedOffset)
	{
		throw std::runtime_error(
			std::string("field offset mismatch: ") + field->getName());
	}

	auto* typeLayout = field->getTypeLayout();
	auto* type = typeLayout->getType();

	if (type->getKind() != TypeReflection::Kind::Array)
		throw std::runtime_error(
			std::string("expected array field: ") + field->getName());

	if (type->getElementCount() != expectedCount)
		throw std::runtime_error(
			std::string("array count mismatch: ") + field->getName());

	auto* elementLayout = typeLayout->getElementTypeLayout();

	if (elementLayout->getSize(ParameterCategory::Uniform) != expectedElementSize)
		throw std::runtime_error(
			std::string("array element size mismatch: ") + field->getName());

	if (typeLayout->getStride(ParameterCategory::Uniform) != expectedElementSize)
		throw std::runtime_error(
			std::string("array stride mismatch: ") + field->getName());
}

static inline void printField(
	slang::VariableLayoutReflection* field,
	size_t expectedOffset,
	size_t expectedCount,
	size_t expectedElementSize)
{
	using namespace slang;

	auto* typeLayout = field->getTypeLayout();
	auto* type = typeLayout->getType();

	std::cout << "\nFIELD: " << field->getName() << "\n";
	std::cout << "  offset: "
		<< field->getOffset(ParameterCategory::Uniform) << "\n";

	std::cout << "  kind: "
		<< int(type->getKind()) << "\n";

	std::cout << "  size: "
		<< typeLayout->getSize(ParameterCategory::Uniform) << "\n";

	std::cout << "  stride: "
		<< typeLayout->getStride(ParameterCategory::Uniform) << "\n";

	std::cout << "  element size: "
		<< typeLayout->getElementTypeLayout()->getSize(
			ParameterCategory::Uniform)
		<< "\n";

	std::cout << "  element stride: "
		<< typeLayout->getElementTypeLayout()->getStride(
			ParameterCategory::Uniform)
		<< "\n";

	std::cout << "  element count: "
		<< type->getElementCount()
		<< "\n";
}

static inline void validateKDopNodeHotLayout(slang::TypeLayoutReflection* layout)
{
	using namespace slang;

	auto uniform = ParameterCategory::Uniform;

	assert(std::strcmp(layout->getName(), "KDopNodeHot") == 0);

	const size_t expectedSize = sizeof(KDopNodeHot);

	if (layout->getSize(uniform) != expectedSize)
		throw std::runtime_error("KDopNodeHot size mismatch");

	if (layout->getAlignment(uniform) != alignof(KDopNodeHot))
		throw std::runtime_error("KDopNodeHot alignment mismatch");

	if (layout->getFieldCount() != 2)
		throw std::runtime_error("KDopNodeHot field count mismatch");

	auto* minField = layout->getFieldByIndex(0);
	auto* maxField = layout->getFieldByIndex(1);

	if (std::strcmp(minField->getName(), "min_packed") != 0)
		throw std::runtime_error("KDopNodeHot field 0 mismatch");

	if (std::strcmp(maxField->getName(), "max") != 0)
		throw std::runtime_error("KDopNodeHot field 1 mismatch");

	printField(
		minField,
		offsetof(KDopNodeHot, min_packed),
		KDOP_WIDTH,
		sizeof(glm::vec4));

	printField(
		maxField,
		offsetof(KDopNodeHot, max),
		KDOP_WIDTH,
		sizeof(glm::vec4));

	if (layout->getSize(uniform) != sizeof(KDopNodeHot))
		throw std::runtime_error("KDopNodeHot final size mismatch");
}