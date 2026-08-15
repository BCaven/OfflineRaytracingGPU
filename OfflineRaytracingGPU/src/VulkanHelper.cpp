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
	glm::vec4 red = glm::vec4(1, 0, 0, 1);
	glm::vec4 green = glm::vec4(0, 1, 0, 1);
	glm::vec4 white = glm::vec4(1);

	glm::vec4 light_white = glm::vec4(10);
	glm::vec4 light_purple = glm::vec4(128, 0, 255, 255) / 255.f;
	glm::vec4 sky = 10.f * (glm::vec4(4, 4, 4, 255) / 255.f);

	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 1},				// 0
			Material{ pastel_green, -1.5 },				// 1
			Material{ pastel_blue, 0.5 },				// 2
			Material{ pastel_purple, 0, light_white},	// 3
			Material{ pastel_purple, 0, light_purple},	// 4
			Material{ pastel_orange, 1},				// 5
			Material{ pastel_grey, 0 },					// 6
			Material{ red, 0},							// 7
			Material{ green, 0},						// 8
			Material{ white, 0}							// 9
	};

	float radius = 13;
	float offset = radius / 2;
	for (float r = 0; r < PI * 2; r+=PI / 8)
	{
		wrapper.spheres.push_back(
			Sphere{  glm::vec3( offset + (std::sin(r) * radius), -3, offset + (std::cos(r) * radius)), 2, (unsigned int)rand(gen)}
		);
	}
	

	int wallIndex_white = wrapper.loadObj("assets/plane.obj", 9);
	int wallIndex_green = wrapper.loadObj("assets/plane.obj", 8);
	int wallIndex_red = wrapper.loadObj("assets/plane.obj", 7);

	int suzanneIndex = wrapper.loadObj("assets/suzanne.obj", 5);
	//int beholderIndex = wrapper.loadObj("assets/beholder.obj", 5);
	//wrapper.validateBVH(beholderIndex);
	//wrapper.loadTransform(glm::vec3(0), glm::vec3(0), glm::vec3(1), PrimType::BVH_NODE, beholderIndex);
	//int readingroomIndex = wrapper.loadSplat("assets/readingroom_20x_180.ply");

	//int tomatoIndex = wrapper.loadSplat("assets/tomatoes_10x_180.ply");
	//std::cout << "Tomato BVH (root node: " << tomatoIndex << ")\n";
	//wrapper.printBVH(tomatoIndex, 2);
	//wrapper.validateBVH(tomatoIndex);
	wrapper.loadTransform(glm::vec3(0, 5, 0), glm::vec3(0, 0, 0), glm::vec3(1), PrimType::BVH_NODE, suzanneIndex);


	// floor
	wrapper.loadTransform(glm::vec3(0, -5, 0), glm::vec3(0, 0, 0), glm::vec3(1), PrimType::BVH_NODE, wallIndex_white);
	// ceiling
	wrapper.loadTransform(glm::vec3(0, 25, 0), glm::vec3(0, 0, 0), glm::vec3(1), PrimType::BVH_NODE, wallIndex_white);
	// walls
	wrapper.loadTransform(glm::vec3(0, 0, 15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), PrimType::BVH_NODE, wallIndex_red);
	wrapper.loadTransform(glm::vec3(0, 0, -15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), PrimType::BVH_NODE, wallIndex_green);
	wrapper.loadTransform(glm::vec3(15, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), PrimType::BVH_NODE, wallIndex_white);
	wrapper.loadTransform(glm::vec3(-15, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), PrimType::BVH_NODE, wallIndex_white);

	for (float r = 0; r < PI * 2; r += PI / 6)
	{
		//wrapper.loadTransform(glm::vec3(offset + (std::sin(r) * radius), 3, offset + (std::cos(r) * radius)), glm::vec3(0, 0, 0), glm::vec3(1), PrimType::BVH_NODE, suzanneIndex);
	}

	//wrapper.loadTransform(glm::vec3(0), glm::vec3(0), glm::vec3(1), PrimType::BVH_NODE, readingroomIndex);
	//wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(1), PrimType::BVH_NODE, tomatoIndex);
	//wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(1), PrimType::BVH_NODE, tomatoIndex);


	std::cout << "Number of splats: " << wrapper.splats.size() << "\n";

	wrapper.camera = CameraWrapper{
		.origin = glm::vec3(0, 0.75, 1),
		.fov = 20
	};

	wrapper.shaderData.backgroundColor = sky;

	int root = wrapper.loadBVH();
	std::cout << "TLAS BVH (root node: " << root << ")\n";

	//wrapper.printBVH(root, 3);

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

