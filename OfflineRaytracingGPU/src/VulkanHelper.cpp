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

PackedRef suzannes_no_collectionsNxN(VK_Wrap& wrapper, int n)
{
	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 0},				// 00
	};
	PackedRef suzanneInstance = wrapper.loadObj("assets/suzanne.obj", 0);

	float step = 3;
	std::vector<PackedRef> suzannes;
	for (float i = -n; i < n; i += step) for (float j = -n; j < n; j += step)
	{
		suzannes.push_back(
			wrapper.loadTransform(
				glm::vec3(i, 0, j), glm::vec3(0, 0, 0), glm::vec3(1), suzanneInstance));

	}
	return wrapper.loadCollection(suzannes);
}
PackedRef suzannes_row_instancesNxN(VK_Wrap& wrapper, int n)
{
	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 0},				// 00
	};
	PackedRef suzanneInstance = wrapper.loadObj("assets/suzanne.obj", 0);

	float step = 3;
	std::vector<PackedRef> suzanneRow;
	std::vector<PackedRef> suzanneCols;
	for (float i = -n; i < n; i += step) 
	{
		suzanneRow.push_back(
			wrapper.loadTransform(
				glm::vec3(i, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1), suzanneInstance));

	}
	auto suzanneRowInstance = wrapper.loadCollection(suzanneRow);
	for (float j = -n; j < n; j += step)
	{
		suzanneCols.push_back(
			wrapper.loadTransform(
				glm::vec3(0, 0, j), glm::vec3(0, 0, 0), glm::vec3(1), suzanneRowInstance)
		);
	}
	
	return wrapper.loadCollection(suzanneCols);
}

PackedRef suzannes_no_collectionsNxNxN(VK_Wrap& wrapper, int n)
{
	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 0},				// 00
	};
	PackedRef suzanneInstance = wrapper.loadObj("assets/suzanne.obj", 0);

	float step = 3;
	std::vector<PackedRef> suzannes;
	for (float i = -n; i < n; i += step) for (float j = -n; j < n; j += step) for (float k = -n * 2; k < 0; k += step)
	{
		suzannes.push_back(
			wrapper.loadTransform(
				glm::vec3(i, k, j), glm::vec3(0, 0, 0), glm::vec3(1), suzanneInstance));

	}
	return wrapper.loadCollection(suzannes);
}
PackedRef suzannes_row_instancesNxNxN(VK_Wrap& wrapper, int n)
{
	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 0},				// 00
	};
	PackedRef suzanneInstance = wrapper.loadObj("assets/suzanne.obj", 0);

	float step = 3;
	std::vector<PackedRef> suzanneRow;
	std::vector<PackedRef> suzanneCols;
	for (float i = -n; i < n; i += step)
	{
		suzanneRow.push_back(
			wrapper.loadTransform(
				glm::vec3(i, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1), suzanneInstance));

	}
	auto suzanneRowInstance = wrapper.loadCollection(suzanneRow);
	for (float j = -n; j < n; j += step)
	{
		suzanneCols.push_back(
			wrapper.loadTransform(
				glm::vec3(0, 0, j), glm::vec3(0, 0, 0), glm::vec3(1), suzanneRowInstance)
		);
	}
	auto suzannePlaneInstance = wrapper.loadCollection(suzanneCols);
	std::vector<PackedRef> suzannePlanes;
	for (float k = -n * 2; k < 0; k += step)
	{
		suzannePlanes.push_back(
			wrapper.loadTransform(
				glm::vec3(0, k, 0), glm::vec3(0, 0, 0), glm::vec3(1), suzannePlaneInstance)
		);
	}
	return wrapper.loadCollection(suzannePlanes);
}

PackedRef suzannes_cube_instancesNxNxN(VK_Wrap& wrapper, int n)
{
	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 0},				// 00
	};
	PackedRef suzanneInstance = wrapper.loadObj("assets/suzanne.obj", 0);

	std::vector<PackedRef> suzanneCube;

	float cubeSize = n / 20;
	float step = 3;

	for (float i = 0; i < cubeSize * step; i += step) for (float j = 0; j < cubeSize * step; j += step) for (float k = 0; k < cubeSize * step; k += step)
	{
		suzanneCube.push_back(
			wrapper.loadTransform(
				glm::vec3(i, j, k), glm::vec3(0, 0, 0), glm::vec3(1), suzanneInstance)
		);
	}
	auto suzanneCubeInstance = wrapper.loadCollection(suzanneCube);
	std::cout << "Cube instance built\n";
	std::vector<PackedRef> suzanneCollection;
	for (float i = -n; i < n; i += step * cubeSize)	for (float j = ( - n * 2) - (step * cubeSize); j < -(step * cubeSize); j += step * cubeSize)	for (float k = -n; k < n; k += step * cubeSize)

	{
		suzanneCollection.push_back(
			wrapper.loadTransform(
				glm::vec3(i, j, k), glm::vec3(0, 0, 0), glm::vec3(1), suzanneCubeInstance));

	}
	
	return wrapper.loadCollection(suzanneCollection);
}

PackedRef nested_collections(VK_Wrap& wrapper)
{

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> rand(0, 5);

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
	glm::vec4 sky = 20.f * (glm::vec4(4, 4, 4, 255) / 255.f);

	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 1},				// 0
			Material{ pastel_green, -1.5 },				// 1
			Material{ pastel_blue, 0.5 },				// 2
			Material{ pastel_purple, 0, light_white},	// 3
			Material{ pastel_purple, 0, light_purple},	// 4
			Material{ pastel_orange, 0},				// 5
			Material{ pastel_grey, 0 },					// 6
			Material{ red, 0},							// 7
			Material{ green, 0},						// 8
			Material{ white, 0},						// 9
			Material{ white, 0, light_white}			// 10
	};

	float radius = 12;
	float offset = 0; //radius / 2;
	for (float r = 0; r < PI * 2; r += PI / 8)
	{
		wrapper.spheres.push_back(
			Sphere{ glm::vec3(offset + (std::sin(r) * radius), 0, offset + (std::cos(r) * radius)), 1, (unsigned int)rand(gen) }
		);
	}

	std::vector<PackedRef> sphereRing;
	for (int i = 0; i < wrapper.spheres.size(); i++)
	{
		PackedRef p = packChild(PrimType::SPHERE, i);
		float r = (rand(gen) + 1);
		sphereRing.push_back(wrapper.loadTransform(glm::vec3(0, r - 5, 0), glm::vec3(0, 0, 0), glm::vec3(1, r, 1), p));
	}
	auto sphereCollection = wrapper.loadCollection(sphereRing);
	PackedRef wallIndex_white = wrapper.loadObj("assets/plane.obj", 9);
	PackedRef wallIndex_ceiling = wrapper.loadObj("assets/plane.obj", 10);
	PackedRef wallIndex_green = wrapper.loadObj("assets/plane.obj", 8);
	PackedRef wallIndex_red = wrapper.loadObj("assets/plane.obj", 7);

	PackedRef halfSuzanneIndex = wrapper.loadObj("assets/half_suzanne.obj", 5);
	PackedRef mirrorSuzanneIndex = wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(-1, 1, 1), halfSuzanneIndex);

	PackedRef suzanneIndex = wrapper.loadCollection({ halfSuzanneIndex, mirrorSuzanneIndex });

	//PackedRef suzanneIndex = wrapper.loadObj("assets/suzanne.obj", 5);
	//PackedRef icosphereIndex = wrapper.loadObj("assets/icosphere.obj", 5);
	//int beholderIndex = wrapper.loadObj("assets/beholder.obj", 5);
	//wrapper.loadTransform(glm::vec3(0), glm::vec3(0), glm::vec3(1), PrimType::BVH_NODE, beholderIndex);
	//PackedRef readingroomIndex = wrapper.loadSplat("assets/readingroom_20x_180.ply");

	//PackedRef tomatoIndex = wrapper.loadSplat("assets/tomatoes_10x_180.ply");

	//wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, -1, 1), suzanneIndex);
	//wrapper.loadTransform(glm::vec3(0, 3, 0), glm::vec3(0, 0, 0), glm::vec3(1), icosphereIndex);


	// floor
	auto floor = wrapper.loadTransform(glm::vec3(0, -5, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_white);
	// ceiling
	auto ceiling = wrapper.loadTransform(glm::vec3(0, 25, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_ceiling);
	// walls
	auto wall_red = wrapper.loadTransform(glm::vec3(0, 0, 15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_red);
	auto wall_green = wrapper.loadTransform(glm::vec3(0, 0, -15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_green);
	auto wall_front = wrapper.loadTransform(glm::vec3(15, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto wall_back = wrapper.loadTransform(glm::vec3(-15, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto cornellBox = wrapper.loadCollection({ floor, ceiling, wall_red, wall_green, wall_front, wall_back });

	radius = 2;
	std::vector<PackedRef> suzanneRings;
	for (float ring = 1; ring < 6; ring++) for (float r = 0; r < PI * 2; r += PI / (4 * ring))
	{
		suzanneRings.push_back(wrapper.loadTransform(glm::vec3(offset + (std::sin(r) * (ring * radius)), -4, offset + (std::cos(r) * (ring * radius))), glm::vec3(0, 0, 0), glm::vec3(1), suzanneIndex));
	}
	auto suzanneCollection = wrapper.loadCollection(suzanneRings);
	//wrapper.loadTransform(glm::vec3(0), glm::vec3(0), glm::vec3(1), readingroomIndex);	
	//wrapper.loadTransform(glm::vec3(0, -5, 0), glm::vec3(0), glm::vec3(1), tomatoIndex);
	//wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(1), PrimType::BVH_NODE, tomatoIndex);

	wrapper.camera = CameraWrapper{
		.origin = glm::vec3(0, 0.75, 1),
		.fov = 20
	};

	wrapper.shaderData.backgroundColor = sky;

	return wrapper.loadCollection({ cornellBox, suzanneCollection, sphereCollection });
}

PackedRef flat_BLAS_TLAS(VK_Wrap& wrapper)
{
	return packChild(EMPTY, 0);
}

int main(int argc, char* argv[])
{
	std::cout << "Hello World!\n";

	VK_Wrap wrapper;

	PackedRef root = 0;

	if (argc < 2)
	{
		root = suzannes_cube_instancesNxNxN(wrapper, 1000);
	}
	else
	{
		int choice = std::atoi(argv[1]);
		switch (choice)
		{
		case 0:
			root = suzannes_no_collectionsNxN(wrapper, 1000);
			break;
		case 1:
			root = suzannes_row_instancesNxN(wrapper, 1000);
			break;
		case 2:
			root = suzannes_no_collectionsNxNxN(wrapper, 1000);
			break;
		case 3:
			root = suzannes_row_instancesNxNxN(wrapper, 1000);
			break;
		case 4:
			root = suzannes_cube_instancesNxNxN(wrapper, 1000);
			break;
		case 5:
			root = nested_collections(wrapper);
			break;
		case 6:
			root = flat_BLAS_TLAS(wrapper);
			break;
		default:
			root = suzannes_row_instancesNxN(wrapper, 100);
			break;
		}
	}

	wrapper.init();
	wrapper.shaderData.sceneRoot = root;
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

