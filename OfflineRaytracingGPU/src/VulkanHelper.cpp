// VulkanHelper.cpp : Defines the entry point for the application.
//


#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "VulkanWrapper.h"
#include <random>


using namespace std;

int main()
{
	std::cout << "Hello World!\n";

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> rand(0, 5);


	VK_Wrap wrapper;

	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	glm::vec4 pastel_green = glm::vec4(143, 237, 82, 255) / 255.0f;
	glm::vec4 pastel_blue = glm::vec4(83, 236, 228, 255) / 255.0f;
	glm::vec4 pastel_purple = glm::vec4(181, 150, 243, 255) / 255.0f;
	glm::vec4 pastel_grey = glm::vec4(139, 157, 180, 255) / 255.f;
	glm::vec4 light_white = glm::vec4(10);
	glm::vec4 light_purple = glm::vec4(128, 0, 255, 255) / 255.f;
	glm::vec4 sky = 10.f * (glm::vec4(4, 4, 4, 255) / 255.f);

	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 1},
			Material{ pastel_green, -1.5 },
			Material{ pastel_blue, 0.5 },
			Material{ pastel_purple, 0, light_white},
			Material{ pastel_purple, 0, light_purple},
			Material{ pastel_orange, 0},
			Material{ pastel_grey, 0 }

	};

	wrapper.spheres = {
		//Sphere(glm::vec3(0, -1001.5, 0), 1000, 5),
	};
	float radius = 13;
	float offset = radius / 2;
	for (float r = 0; r < PI * 2; r+=PI / 8)
	{
		wrapper.spheres.push_back(
			Sphere{  glm::vec3( offset + (std::sin(r) * radius), -3, offset + (std::cos(r) * radius)), 2, (unsigned int) rand(gen)}
		);
	}

	wrapper.loadObj("assets/suzanne.obj", 5);

	wrapper.loadSplat("assets/readingroom_20x_180.ply");

	std::cout << "Number of splats: " << wrapper.splats.size() << "\n";

	wrapper.camera = CameraWrapper{
		.origin = glm::vec3(0, 0.75, 1),
		.fov = 20
	};

	wrapper.shaderData.backgroundColor = sky;


	// build bvh
	/*
	struct bvhNode
	{
		glm::vec3 min;
		glm::vec3 max;
		uint32_t mortonCode;
		PrimType rightPrim;
		PrimType leftPrim;
		int leftIndex;
		int rightIndex;
	}
	*/
	// get scene bounds
	glm::vec3 sceneMin(FLT_MAX);
	glm::vec3 sceneMax(-FLT_MAX);

	for (const auto& sphere : wrapper.spheres)
	{
		sceneMin = make_aabb_min(sceneMin, sphere.center - sphere.radius);
		sceneMax = make_aabb_max(sceneMax, sphere.center + sphere.radius);
	}
	for (const auto& tri : wrapper.triangles)
	{
		glm::vec3 triMin = make_aabb_min(
			make_aabb_min(tri.v0, tri.v1),
			tri.v2);

		glm::vec3 triMax = make_aabb_max(
			make_aabb_max(tri.v0, tri.v1),
			tri.v2);

		sceneMin = make_aabb_min(sceneMin, triMin);
		sceneMax = make_aabb_max(sceneMax, triMax);
	}
	for (const auto& splat : wrapper.splats)
	{
		glm::vec3 splatMin = splat.center - splat.halfExtent;
		glm::vec3 splatMax = splat.center + splat.halfExtent;
		sceneMin = make_aabb_min(sceneMin, splatMin);
		sceneMax = make_aabb_max(sceneMax, splatMax);
	}

	glm::vec3 invSceneExtent = 1.f / make_aabb_max(sceneMax - sceneMin, glm::vec3(1e-8));


	std::vector<bvhNode>& bvhNodes = wrapper.bvhNodes;
	for (size_t i = 0; i < wrapper.spheres.size(); i+=2)
	{
		if (i + 1 < wrapper.spheres.size())
		{
			const auto& s0 = wrapper.spheres[i];
			const auto& s1 = wrapper.spheres[i + 1];

			glm::vec3 min = make_aabb_min(s0.center - s0.radius, s1.center - s1.radius);
			glm::vec3 max = make_aabb_max(s0.center + s0.radius, s1.center + s1.radius);

			glm::vec3 center = (min + max) * 0.5f;
			

			bvhNodes.push_back(bvhNode{
				.min = min,
				.max = max,
				.mortonCode = compute_morton_code(center, sceneMin, invSceneExtent),
				.rightPrim = PrimType::SPHERE,
				.leftPrim = PrimType::SPHERE,
				.leftIndex = (int) i,
				.rightIndex = (int) i + 1,
				});
		}
		else
		{
			const auto& s0 = wrapper.spheres[i];

			glm::vec3 min = s0.center - s0.radius;
			glm::vec3 max = s0.center + s0.radius;
			glm::vec3 center = (min + max) * 0.5f;

			bvhNodes.push_back(bvhNode{
				.min = min,
				.max = max,
				.mortonCode = compute_morton_code(center, sceneMin, invSceneExtent),
				.rightPrim = PrimType::EMPTY,
				.leftPrim = PrimType::SPHERE,
				.leftIndex = (int) i,
				.rightIndex = -1,
				});
		}
	}
	for (size_t i = 0; i < wrapper.triangles.size(); i+=2)
	{
		if (i + 1 < wrapper.triangles.size())
		{
			const auto& t0 = wrapper.triangles[i];
			const auto& t1 = wrapper.triangles[i + 1];

			glm::vec3 max = make_aabb_max(make_aabb_max(t0.v0, t0.v1), t0.v2);
			glm::vec3 min = make_aabb_min(make_aabb_min(t0.v0, t0.v1), t0.v2);
			max = make_aabb_max(max, make_aabb_max(make_aabb_max(t1.v0, t1.v1), t1.v2));
			min = make_aabb_min(min, make_aabb_min(make_aabb_min(t1.v0, t1.v1), t1.v2));

			max += 0.001f;
			min -= 0.001f;
			glm::vec3 center = (min + max) * 0.5f;

			bvhNodes.push_back(bvhNode{
				.min = min,
				.max = max,
				.mortonCode = compute_morton_code(center, sceneMin, invSceneExtent),
				.rightPrim = PrimType::TRIANGLE,
				.leftPrim = PrimType::TRIANGLE,
				.leftIndex = (int) i,
				.rightIndex = (int) i + 1,
				});
		}
		else
		{
			const auto& t0 = wrapper.triangles[i];

			glm::vec3 max = make_aabb_max(make_aabb_max(t0.v0, t0.v1), t0.v2);
			glm::vec3 min = make_aabb_min(make_aabb_min(t0.v0, t0.v1), t0.v2);
			
			max += 0.001f;
			min -= 0.001f;
			glm::vec3 center = (min + max) * 0.5f;

			bvhNodes.push_back(bvhNode{
				.min = min,
				.max = max,
				.mortonCode = compute_morton_code(center, sceneMin, invSceneExtent),
				.rightPrim = PrimType::EMPTY,
				.leftPrim = PrimType::TRIANGLE,
				.leftIndex = (int)i,
				.rightIndex = -1,
				});
		}
	}
	for (size_t i = 0; i < wrapper.splats.size(); i += 2)
	{
		if (i + 1 < wrapper.splats.size())
		{
			const auto& s0 = wrapper.splats[i];
			const auto& s1 = wrapper.splats[i + 1];

			glm::vec3 min = make_aabb_min(s0.center - s0.halfExtent, s1.center - s1.halfExtent);
			glm::vec3 max = make_aabb_max(s0.center + s0.halfExtent, s1.center + s1.halfExtent);

			glm::vec3 center = (min + max) * 0.5f;


			bvhNodes.push_back(bvhNode{
				.min = min,
				.max = max,
				.mortonCode = compute_morton_code(center, sceneMin, invSceneExtent),
				.rightPrim = PrimType::GAUSSIAN_SPLAT,
				.leftPrim = PrimType::GAUSSIAN_SPLAT,
				.leftIndex = (int)i,
				.rightIndex = (int)i + 1,
				});
		}
		else
		{
			const auto& s0 = wrapper.splats[i];

			glm::vec3 min = s0.center - s0.halfExtent;
			glm::vec3 max = s0.center + s0.halfExtent;
			glm::vec3 center = (min + max) * 0.5f;

			bvhNodes.push_back(bvhNode{
				.min = min,
				.max = max,
				.mortonCode = compute_morton_code(center, sceneMin, invSceneExtent),
				.rightPrim = PrimType::EMPTY,
				.leftPrim = PrimType::GAUSSIAN_SPLAT,
				.leftIndex = (int)i,
				.rightIndex = -1,
				});
		}
	}

	// sort them by morton code
	std::sort(
		bvhNodes.begin(),
		bvhNodes.end(),
		[](const bvhNode& a, const bvhNode& b)
		{
			return a.mortonCode < b.mortonCode;
		}
	);

	

	// build parent nodes
	int root = BuildBVHRecursive(bvhNodes, 0, bvhNodes.size());

	std::cout << "Bvh root index: " << root << " total size: " << wrapper.bvhNodes.size() << "\n";
	/*
	for (const auto& node : bvhNodes)
	{
		std::cout <<
			"Bvh Node l_index: " << node.leftIndex <<
			" r_index: " << node.rightIndex <<
			" l_type: " << node.leftPrim <<
			" r_type: " << node.rightPrim <<
			"\n";
	}
	*/
	wrapper.init();
	wrapper.shaderData.bvhRoot = root;

	bool running = true;
	while (running)
	{
		if (wrapper.draw())
		{
			running = false;
		}

	}

	return 0;
}

