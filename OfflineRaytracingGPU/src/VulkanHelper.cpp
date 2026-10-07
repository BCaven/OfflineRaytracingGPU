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

	std::string prefix = "renderedFrames/suzanneCube_";
	wrapper.savePath = prefix + std::to_string(wrapper.time) + ".exr";

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> rand(-1000, 1000);

	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	glm::vec4 pastel_green = glm::vec4(143, 237, 82, 255) / 255.0f;
	glm::vec4 pastel_blue = glm::vec4(83, 236, 228, 255) / 255.0f;
	glm::vec4 pastel_purple = glm::vec4(181, 150, 243, 255) / 255.0f;
	glm::vec4 pastel_grey = glm::vec4(139, 157, 180, 255) / 255.f;
	glm::vec4 red = glm::vec4(1, 0, 0, 1);
	glm::vec4 green = glm::vec4(0, 1, 0, 1);
	glm::vec4 white = glm::vec4(1);

	glm::vec4 light_white = glm::vec4(10);
	glm::vec4 light_purple = 5.f * (glm::vec4(128, 0, 255, 255) / 255.f);
	glm::vec4 sky = 1.f * (glm::vec4(4, 4, 4, 255) / 255.f);

	wrapper.materials = std::vector<Material>{
		Material{ pastel_orange, 0},				// 0
		Material{ pastel_blue, 0.5, 0, pastel_blue * 10.f },				// 2
		Material{ pastel_purple, 0, 0, pastel_purple * 10.f},	// 3
		Material{ pastel_green, -1.8, 25, pastel_green * 10.f},				// 1
		Material{ green, 0, 0, green * 10.f},						// 8
		Material{ pastel_purple, 0, 0, light_purple},	// 4
		Material{ pastel_orange, 0, 0, pastel_orange * 10.f},				// 5
		Material{ pastel_grey, 0, 0, light_white },					// 6
		Material{ red, 0, 0, red * 10.f},							// 7
		Material{ white, 0, 0, white * 10.f}						// 9
	};
	
	for (unsigned int i = 1; i < wrapper.materials.size(); i++)
	{
		wrapper.spheres.push_back(Sphere{
			.center = glm::vec3(0),
			.radius = 1,
			.materialIndex = i
			});
	}

	Collection lightSpheres;
	unsigned int sphereIndex = 0;
	for (int i = 0; i < 1000; i++)
	{
		lightSpheres.push_back(wrapper.loadTransform(glm::vec3(rand(gen), -45, rand(gen)), glm::vec3(0), glm::vec3(1), packChild(SPHERE, sphereIndex)));
		sphereIndex++;
		sphereIndex = sphereIndex % (wrapper.spheres.size() - 1);
	}
	
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
	wrapper.shaderData.backgroundColor = sky;
	wrapper.camera.origin = glm::vec3(0, 5, 0);
	suzanneCollection.insert(suzanneCollection.end(), lightSpheres.begin(), lightSpheres.end());
	return wrapper.loadCollection(suzanneCollection);
}

PackedRef nested_collections(VK_Wrap& wrapper)
{

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> rand(0, 10);

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
	glm::vec4 sky = 1.f * (glm::vec4(4, 4, 4, 255) / 255.f);

	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 1},					// 0
			Material{ pastel_green, -1.5, 50 },				// 1
			Material{ pastel_blue, 0.5 },					// 2
			Material{ pastel_purple, 0, 0, pastel_blue},	// 3
			Material{ pastel_purple, 0, 0, red},			// 4
			Material{ white, 0, 0, light_white},			// 5
			Material{ pastel_grey, -1.7, 30 },				// 6
			Material{ red, 0},								// 7
			Material{ green, 0},							// 8
			Material{ white, 0, 0, light_white},			// 9
			Material{ white, 0},							// 10
			Material{										// 11
				.color = white,
				.metallicOrIor = -1.5,
				.abbe = 50,
			}
	};

	float radius = 8.5;
	float offset = 0; //radius / 2;
	unsigned int matIndex = 0;
	for (float r = 0; r < PI * 2; r += PI / 8)
	{
		wrapper.spheres.push_back(
			Sphere{ glm::vec3(offset + (std::sin(r) * radius), 0, offset + (std::cos(r) * radius)), 1, matIndex }
		);

		matIndex++;
		matIndex = matIndex % 11;
	}

	std::vector<PackedRef> sphereRing;
	for (int i = 0; i < wrapper.spheres.size(); i++)
	{
		PackedRef p = packChild(PrimType::SPHERE, i);
		//float r = (rand(gen) + 1) / 5;
		float r = 1;
		sphereRing.push_back(wrapper.loadTransform(glm::vec3(0, r - 4.5, 0), glm::vec3(0, 0, 0), glm::vec3(1, r + 0.5, 1), p));
	}
	auto sphereCollection = wrapper.loadCollection(sphereRing);
	PackedRef wallIndex_white = wrapper.loadObj("assets/plane.obj", 10);
	PackedRef wallIndex_ceiling = wrapper.loadObj("assets/plane.obj", 10);
	PackedRef wallIndex_green = wrapper.loadObj("assets/plane.obj", 8);
	PackedRef wallIndex_red = wrapper.loadObj("assets/plane.obj", 7);

	PackedRef halfSuzanneIndex = wrapper.loadObj("assets/half_suzanne.obj", 10);
	PackedRef mirrorSuzanneIndex = wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(-1, 1, 1), halfSuzanneIndex);

	PackedRef suzanneIndex = wrapper.loadCollection({ halfSuzanneIndex, mirrorSuzanneIndex });

	//PackedRef suzanneIndex = wrapper.loadObj("assets/suzanne.obj", 5);
	//PackedRef icosphereIndex = wrapper.loadObj("assets/icosphere.obj", 5);
	//int beholderIndex = wrapper.loadObj("assets/beholder.obj", 5);
	//wrapper.loadTransform(glm::vec3(0), glm::vec3(0), glm::vec3(1), PrimType::BVH_NODE, beholderIndex);
	//PackedRef readingroomIndex = wrapper.loadSplat("assets/readingroom_20x_180.ply");

	//PackedRef tomatoIndex = wrapper.loadSplat2("assets/tomatoes_10x_180.ply");

	//wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, -1, 1), suzanneIndex);
	//wrapper.loadTransform(glm::vec3(0, 3, 0), glm::vec3(0, 0, 0), glm::vec3(1), icosphereIndex);


	// floor
	auto floor = wrapper.loadTransform(glm::vec3(0, -5, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_white);
	// ceiling
	auto ceiling = wrapper.loadTransform(glm::vec3(0, 25, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_ceiling);
	// walls
	auto wall_red = wrapper.loadTransform(glm::vec3(0, 0, 15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_red);
	auto wall_green = wrapper.loadTransform(glm::vec3(0, 0, -15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_green);
	auto wall_front = wrapper.loadTransform(glm::vec3(30, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto wall_back = wrapper.loadTransform(glm::vec3(-15, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto cornellBox = wrapper.loadCollection({ floor, ceiling, wall_red, wall_green, wall_front, wall_back });

	radius = 4;
	std::vector<PackedRef> suzanneRings;
	for (float ring = 1; ring < 2; ring++) for (float r = 0; r < PI * 2; r += PI / (3 * ring))
	{
		suzanneRings.push_back(wrapper.loadTransform(glm::vec3(offset + (std::sin(r) * (ring * radius)), -4, offset + (std::cos(r) * (ring * radius))), glm::vec3(0, PI / 2, 0), glm::vec3(1), suzanneIndex));
	}
	auto suzanneCollection = wrapper.loadCollection(suzanneRings);
	//wrapper.loadTransform(glm::vec3(0), glm::vec3(0), glm::vec3(1), readingroomIndex);	
	//auto splatCollection = wrapper.loadTransform(glm::vec3(0, -5, 0), glm::vec3(0), glm::vec3(2), tomatoIndex);
	//wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(1), PrimType::BVH_NODE, tomatoIndex);

	PackedRef prism = wrapper.loadObj("assets/icosphere2.obj", 11);
	Collection prisms;
	radius = 6;
	for (float r = 0; r < PI * 2; r += PI / 8)
	{
		prisms.push_back(
			wrapper.loadTransform(glm::vec3(offset + (std::sin(r) * radius), -4, offset + (std::cos(r) * radius)), glm::vec3(0, wrapper.time / PI, 0), glm::vec3(1), prism)
		);
	}
	auto prismCollection = wrapper.loadCollection(prisms);


	wrapper.camera = CameraWrapper{
		.origin = glm::vec3(0, 0.75, 1),
		.fov = 20
	};

	wrapper.shaderData.backgroundColor = sky;

	return wrapper.loadCollection({ cornellBox, sphereCollection, prismCollection, /*splatCollection,*/ suzanneCollection});
}

PackedRef flat_BLAS_TLAS(VK_Wrap& wrapper)
{
	return packChild(EMPTY, 0);
}

PackedRef large_splat_demo(VK_Wrap& wrapper)
{

	std::string prefix = "renderedFrames/largesplat_demo_";
	wrapper.savePath = prefix + std::to_string(wrapper.time) + ".exr";

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> rand(0, 10);

	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	glm::vec4 pastel_green = glm::vec4(143, 237, 82, 255) / 255.0f;
	glm::vec4 pastel_blue = glm::vec4(83, 236, 228, 255) / 255.0f;
	glm::vec4 pastel_purple = glm::vec4(181, 150, 243, 255) / 255.0f;
	glm::vec4 pastel_grey = glm::vec4(139, 157, 180, 255) / 255.f;
	glm::vec4 red = glm::vec4(1, 0, 0, 1);
	glm::vec4 green = glm::vec4(0, 1, 0, 1);
	glm::vec4 white = glm::vec4(1);

	glm::vec4 light_white = glm::vec4(10);
	glm::vec4 light_purple = 5.f * (glm::vec4(128, 0, 255, 255) / 255.f);
	glm::vec4 sky = 20.f * (glm::vec4(4, 4, 4, 255) / 255.f);

	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 1},				// 0
			Material{ pastel_blue, 0.5 },				// 2
			Material{ pastel_purple, 0, 0, light_white},	// 3
			Material{ pastel_green, -1.8, 25},				// 1
			Material{ green, 0},						// 8
			Material{ pastel_purple, 0, 0, light_purple},	// 4
			Material{ pastel_orange, 0},				// 5
			Material{ pastel_grey, 0, 0, light_white },					// 6
			Material{ red, 0},							// 7
			Material{ white, 0},						// 9
			Material{ white, 0, 0, light_white}			// 10
	}; 

	// TODO: maybe split this into chunks to easy traversal cost
	//Collection readingroomCollection = wrapper.loadSplatCollection("assets/readingroom_1x_180.ply");
	PackedRef readingroomSplat = wrapper.loadSplat("assets/readingroom_1x_180.ply");
	float radius = 3;
	float offset = 0; //radius / 2;
	float rot = wrapper.time;
	unsigned int matIndex = 0;
	for (float r = 0 + rot; r < (PI * 2) + rot; r += PI / 8)
	{
		matIndex++;
		matIndex = matIndex % 9;
		wrapper.spheres.push_back(
			Sphere{ glm::vec3(offset + (std::sin(r) * radius), 0, offset + (std::cos(r) * radius)), 0.5, matIndex }
		);
	}

	for (int i = 0; i < wrapper.spheres.size(); i++)
	{
		PackedRef p = packChild(PrimType::SPHERE, i);
		float r = 1; //(rand(gen) + 1);
		//readingroomCollection.push_back(wrapper.loadTransform(glm::vec3(3, 0, 3), glm::vec3(0, 0, 0), glm::vec3(1, r, 1), p));
	}

	wrapper.camera = CameraWrapper{
	.origin = glm::vec3(0, 0.75, 1),
	.fov = 20
	};

	wrapper.shaderData.backgroundColor = sky;

	//std::cout << "Size of readingroomCollection: " << readingroomCollection.size() << "\n";

	return readingroomSplat;//wrapper.loadCollection(readingroomCollection);

}

PackedRef prism_demo(VK_Wrap& wrapper)
{
	std::string prefix = "renderedFrames/prism_4_";
	wrapper.savePath = prefix + std::to_string(wrapper.time) + ".exr";


	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> rand(-10, 10);

	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	glm::vec4 pastel_green = glm::vec4(143, 237, 82, 255) / 255.0f;
	glm::vec4 pastel_blue = glm::vec4(83, 236, 228, 255) / 255.0f;
	glm::vec4 pastel_purple = glm::vec4(181, 150, 243, 255) / 255.0f;
	glm::vec4 pastel_grey = glm::vec4(139, 157, 180, 255) / 255.f;
	glm::vec4 red = glm::vec4(1, 0, 0, 1);
	glm::vec4 green = glm::vec4(0, 1, 0, 1);
	glm::vec4 white = glm::vec4(1);

	glm::vec4 light_white = 200.f * glm::vec4(1);
	glm::vec4 light_purple = glm::vec4(128, 0, 255, 255) / 255.f;
	glm::vec4 sky = 1.f * (glm::vec4(4, 4, 4, 255) / 255.f);

	wrapper.materials = std::vector<Material>{
			Material{ red, 0},							// 0
			Material{ green, 0},						// 1
			Material{ white, 0},						// 2
			Material{ white, 0, 0, light_white},		// 3
			Material{									// 4
				.color = white,
				.metallicOrIor = -1.8,
				.abbe = 10,
			},
			Material{ white, 0, 0, sky * 2.f},			// 5
			Material{ glm::vec4(0), 0}					// 6

	};

	// cornell box
	PackedRef wallIndex_white = wrapper.loadObj("assets/plane.obj", 2);
	PackedRef wallIndex_black = wrapper.loadObj("assets/plane.obj", 6);
	PackedRef wallIndex_ceiling = wrapper.loadObj("assets/plane.obj", 5);
	PackedRef wallIndex_green = wrapper.loadObj("assets/plane.obj", 1);
	PackedRef wallIndex_red = wrapper.loadObj("assets/plane.obj", 0);
	// floor
	auto floor = wrapper.loadTransform(glm::vec3(0, -5, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_white);
	// ceiling
	auto ceiling = wrapper.loadTransform(glm::vec3(0, 25, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_ceiling);
	// walls
	auto wall_red = wrapper.loadTransform(glm::vec3(0, 0, 4), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_white);
	auto wall_green = wrapper.loadTransform(glm::vec3(0, 0, -4), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_white);
	auto wall_front = wrapper.loadTransform(glm::vec3(15, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto wall_back = wrapper.loadTransform(glm::vec3(-5, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto cornellBox = wrapper.loadCollection({ floor, ceiling, wall_red, wall_green, wall_front, wall_back });


	// prism
	PackedRef prism = wrapper.loadObj("assets/icosphere2.obj", 4);
	
	auto prismInstance = wrapper.loadTransform(glm::vec3(0, -3.5, 0), glm::vec3(0, wrapper.time, 0), glm::vec3(1.5), prism);


	wrapper.spheres.push_back(Sphere{
		.center = glm::vec3(0, 0, 0),
		.radius = 0.5,
		.materialIndex = 3
		});

	auto lightTubePrim = wrapper.loadObj("assets/tube.obj", 6);
	auto lightTube = wrapper.loadTransform(glm::vec3(14, -4, 0), glm::vec3(0, 0, PI/2), glm::vec3(0.51, 13, 0.51), lightTubePrim);
	float lightBoxWidth = 0.02;
	float offset = 0.5;
	auto leftLightWall = wrapper.loadTransform(glm::vec3(14, -5 + offset, 0.5), glm::vec3(PI / 2, 0, 0), glm::vec3(0.5, lightBoxWidth * 3, lightBoxWidth * 3), wallIndex_black);
	auto rightLightWall = wrapper.loadTransform(glm::vec3(14, -5 + offset, -0.5), glm::vec3(PI / 2, 0, 0), glm::vec3(0.5, lightBoxWidth * 3, lightBoxWidth * 3), wallIndex_black);
	auto topLightWall = wrapper.loadTransform(glm::vec3(14, -3.5 + offset, 0), glm::vec3(0), glm::vec3(0.5, lightBoxWidth, lightBoxWidth), wallIndex_black);
	auto bottomLightWall = wrapper.loadTransform(glm::vec3(14, -4.5 + offset, 0), glm::vec3(0), glm::vec3(0.5, lightBoxWidth, lightBoxWidth), wallIndex_black);
	auto lightInstance = wrapper.loadTransform(glm::vec3(14, -4 + offset, 0), glm::vec3(0, 0, 0), glm::vec3(1), packChild(SPHERE, 0));

	return wrapper.loadCollection({ cornellBox, prismInstance, lightInstance, leftLightWall, rightLightWall, topLightWall});
}

PackedRef demo_video(VK_Wrap& wrapper)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> rand(0, 10);

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
	glm::vec4 sky = 1.f * (glm::vec4(4, 4, 4, 255) / 255.f);

	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 1},					// 0
			Material{ pastel_green, -1.5, 50 },				// 1
			Material{ pastel_blue, 0.5 },					// 2
			Material{ pastel_purple, 0},	// 3
			Material{ pastel_purple, 0},			// 4
			Material{ white, 0},			// 5
			Material{ pastel_grey, -1.7, 30 },				// 6
			Material{ red, 0},								// 7
			Material{ green, 0},							// 8
			Material{ white, 0, 0, light_white},			// 9
			Material{ white, 0},							// 10
			Material{										// 11
				.color = white,
				.metallicOrIor = -1.9,
				.abbe = 7,
			}
	};

	float radius = 8.5;
	float offset = 0; //radius / 2;
	unsigned int matIndex = 0;
	for (float r = 0; r < PI * 2; r += PI / 8)
	{
		wrapper.spheres.push_back(
			Sphere{ glm::vec3(offset + (std::sin(r) * radius), 0, offset + (std::cos(r) * radius)), 1, matIndex }
		);

		matIndex++;
		matIndex = matIndex % 8;
	}
	wrapper.spheres.push_back(
		Sphere{ glm::vec3(0, 0, 0), 1, 9}
	);

	std::vector<PackedRef> sphereRing;
	for (int i = 0; i < wrapper.spheres.size() - 1; i++)
	{
		PackedRef p = packChild(PrimType::SPHERE, i);
		//float r = (rand(gen) + 1) / 5;
		float r = 1;
		sphereRing.push_back(wrapper.loadTransform(glm::vec3(0, r - 3, 0), glm::vec3(0, 0, 0), glm::vec3(1, r + 0.5, 1), p));
	}
	sphereRing.push_back(wrapper.loadTransform(glm::vec3(0, -4.5, 0), glm::vec3(0), glm::vec3(0.5), packChild(PrimType::SPHERE, wrapper.spheres.size() - 1)));
	auto sphereCollection = wrapper.loadCollection(sphereRing);
	PackedRef wallIndex_white = wrapper.loadObj("assets/plane.obj", 10);
	PackedRef wallIndex_ceiling = wrapper.loadObj("assets/plane.obj", 10);
	PackedRef wallIndex_green = wrapper.loadObj("assets/plane.obj", 8);
	PackedRef wallIndex_red = wrapper.loadObj("assets/plane.obj", 7);
	PackedRef halfSuzanneIndex = wrapper.loadObj("assets/half_suzanne.obj", 10);
	PackedRef mirrorSuzanneIndex = wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(-1, 1, 1), halfSuzanneIndex);

	PackedRef suzanneIndex = wrapper.loadCollection({ halfSuzanneIndex, mirrorSuzanneIndex });

	// floor
	auto floor = wrapper.loadTransform(glm::vec3(0, -5, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_white);
	// ceiling
	auto ceiling = wrapper.loadTransform(glm::vec3(0, 25, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_ceiling);
	// walls
	auto wall_red = wrapper.loadTransform(glm::vec3(0, 0, 15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_red);
	auto wall_green = wrapper.loadTransform(glm::vec3(0, 0, -15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_green);
	auto wall_front = wrapper.loadTransform(glm::vec3(30, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto wall_back = wrapper.loadTransform(glm::vec3(-15, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto cornellBox = wrapper.loadCollection({ floor, ceiling, wall_red, wall_green, wall_front, wall_back });

	radius = 4;
	std::vector<PackedRef> suzanneRings;
	for (float ring = 1; ring < 2; ring++) for (float r = 0; r < PI * 2; r += PI / (3 * ring))
	{
		suzanneRings.push_back(wrapper.loadTransform(glm::vec3(offset + (std::sin(r) * (ring * radius)), -4, offset + (std::cos(r) * (ring * radius))), glm::vec3(0, PI / 2, 0), glm::vec3(1), suzanneIndex));
	}
	auto suzanneCollection = wrapper.loadCollection(suzanneRings);
	//wrapper.loadTransform(glm::vec3(0), glm::vec3(0), glm::vec3(1), readingroomIndex);	
	//auto splatCollection = wrapper.loadTransform(glm::vec3(0, -5, 0), glm::vec3(0), glm::vec3(2), tomatoIndex);
	//wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(1), PrimType::BVH_NODE, tomatoIndex);

	PackedRef prism = wrapper.loadObj("assets/icosphere2.obj", 11);
	Collection prisms;
	radius = 6;
	prisms.push_back(
			wrapper.loadTransform(glm::vec3(0, -3, 0), glm::vec3(0, wrapper.time / PI, 0), glm::vec3(3), prism)
		);
	auto prismCollection = wrapper.loadCollection(prisms);


	wrapper.camera = CameraWrapper{
		.origin = glm::vec3(0, 0.75, 1),
		.fov = 20
	};

	wrapper.shaderData.backgroundColor = sky;

	return wrapper.loadCollection({ cornellBox, sphereCollection, prismCollection, /*splatCollection,*/ suzanneCollection });
}

PackedRef fractal(VK_Wrap& wrapper)
{
	std::string prefix = "renderedFrames/fractal_2_";
	wrapper.savePath = prefix + std::to_string(wrapper.time) + ".exr";

	glm::vec4 sky = glm::vec4(0.5);
	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	glm::vec4 light_1 = 1.f * glm::vec4(83, 236, 228, 255) / 255.0f;
	glm::vec4 light_2 = 1.f * glm::vec4(181, 150, 243, 255) / 255.0f;

	wrapper.shaderData.backgroundColor = sky;
	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 0},				// 00
			Material{ sky, 0 },
			Material{ sky, 0, 0, light_1 },
			Material{ sky, 0, 0, light_2 }
	};
	PackedRef suzanneInstance = wrapper.loadObj("assets/suzanne.obj", 0);

	PackedRef suzanneTransform = wrapper.loadTransform(glm::vec3(4, 0, 4), glm::vec3(0), glm::vec3(1), suzanneInstance);
	PackedRef suzanneTransform2 = wrapper.loadTransform(glm::vec3(-4, 0, -4), glm::vec3(0), glm::vec3(1), suzanneInstance);

	PackedRef recursiveTransform = wrapper.loadTransform(glm::vec3(0, 0, 0), glm::vec3(0, PI / 8, 0), glm::vec3(0.7), packChild(EMPTY, 0));

	PackedRef suzanneCollection = wrapper.loadCollection({ suzanneTransform, suzanneTransform2 });

	PackedRef recursiveCollection = wrapper.loadCollection({ suzanneCollection, recursiveTransform});
	
	wrapper.updateTransform(recursiveTransform, recursiveCollection);  // need to update transform child once tree has been built
	
	std::cout << "recursive collection index: " << unpackIndex(recursiveCollection);

	if (wrapper.transforms[unpackIndex(recursiveTransform)].childPrim != EMPTY)
	{
		std::cout << "recursive transform child was set correctly\n";
		std::cout << "New child index: " << wrapper.transforms[unpackIndex(recursiveTransform)].childIndex << "\n";
	}

	PackedRef wallIndex_white = wrapper.loadObj("assets/plane.obj", 1);
	PackedRef wallIndex_ceiling = wrapper.loadObj("assets/plane.obj", 1);
	PackedRef wallIndex_green = wrapper.loadObj("assets/plane.obj", 2);
	PackedRef wallIndex_red = wrapper.loadObj("assets/plane.obj", 1);

	wrapper.spheres.push_back(Sphere{
		Sphere{
			.center = glm::vec3(0),
			.radius = 1,
			.materialIndex = 2
		}
		});
	wrapper.spheres.push_back(Sphere{
		Sphere{
			.center = glm::vec3(0),
			.radius = 1,
			.materialIndex = 3
		}
		});

	PackedRef sphereTransform1 = wrapper.loadTransform(glm::vec3(7, 7, 0), glm::vec3(0), glm::vec3(1), packChild(SPHERE, 0));
	PackedRef sphereTransform2 = wrapper.loadTransform(glm::vec3(-7, 7, 0), glm::vec3(0), glm::vec3(1), packChild(SPHERE, 1));

	// floor
	auto floor = wrapper.loadTransform(glm::vec3(0, -1, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_white);
	// ceiling
	auto ceiling = wrapper.loadTransform(glm::vec3(0, 25, 0), glm::vec3(0, 0, 0), glm::vec3(1), wallIndex_ceiling);
	// walls
	auto wall_red = wrapper.loadTransform(glm::vec3(0, 0, 15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_red);
	auto wall_green = wrapper.loadTransform(glm::vec3(0, 0, -15), glm::vec3(PI / 2., 0, 0), glm::vec3(1), wallIndex_green);
	auto wall_front = wrapper.loadTransform(glm::vec3(30, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto wall_back = wrapper.loadTransform(glm::vec3(-15, 0, 0), glm::vec3(0, 0, PI / 2.), glm::vec3(1), wallIndex_white);
	auto cornellBox = wrapper.loadCollection({ floor, ceiling, wall_red, wall_green, wall_front, wall_back });

	wrapper.camera = CameraWrapper{
		.origin = glm::vec3(17, 9, 0),
		.direction = glm::normalize(glm::vec3(-17, -9, 0)),
		.fov = 30 - wrapper.time,
	};

	wrapper.camera.right = glm::normalize(glm::cross(wrapper.camera.direction, WORLD_UP));
	wrapper.camera.up = glm::cross(wrapper.camera.right, wrapper.camera.direction);

	return wrapper.loadCollection({ recursiveCollection, cornellBox });
}

PackedRef pickRoot(VK_Wrap& wrapper, int choice, int argc, char** argv, bool resetCamera = true)
{
	auto oldCam = wrapper.camera;
	PackedRef root = 0;
	int selection = choice;
	// build the scene
	if (argc >= 2)
	{
		std::cout << "Using command line choice: " << argv[1] << "\n";
		selection = std::atoi(argv[1]);
	}
	switch (selection)
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
	case 7:
		root = large_splat_demo(wrapper);
		break;
	case 8:
		root = prism_demo(wrapper);
		break;
	case 9:
		root = demo_video(wrapper);
		break;
	case 10:
		root = fractal(wrapper);
		break;
	default:
		root = suzannes_row_instancesNxN(wrapper, 100);
		break;
	}
	if (!resetCamera)
	{
		wrapper.camera = oldCam;
	}
	return root;
}

int main(int argc, char* argv[])
{
	std::cout << "Hello World!\n";

	VK_Wrap wrapper;
	wrapper.savePath = "renderedFrames/default.exr";

	wrapper.numFramesPerFile = 200;
	wrapper.numImagesPerSequence = 20;
	float end_time = 29;
	wrapper.time_delta =  end_time / wrapper.numImagesPerSequence;
	wrapper.time = 0;

	int choice = 4;

	PackedRef root = pickRoot(wrapper, choice, argc, argv);
	wrapper.shaderData.sceneRoot = root;

	wrapper.init();
	bool running = true;
	float prevT = wrapper.time;

	while (running)
	{
		
		wrapper.getFences();
		if (prevT != wrapper.time)
		{
			std::cout << "previous root: " << root << "\n";

			std::cout << "Building scene\n";
			// reset nodes
			wrapper.clearPrimitives();


			root = pickRoot(wrapper, choice, argc, argv, true);

			wrapper.reloadPrimitives();

			std::cout << "new root: " << root << "\n";
			wrapper.shaderData.sceneRoot = root;

		}
		prevT = wrapper.time;

		if (wrapper.draw())
		{
			running = false;
		}

	}

	return 0;
}

