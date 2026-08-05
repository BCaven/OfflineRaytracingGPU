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
	std::uniform_int_distribution<> rand(0, 3);


	VK_Wrap wrapper;

	glm::vec4 pastel_orange = glm::vec4(252, 187, 67, 255) / 255.0f;
	glm::vec4 pastel_green = glm::vec4(143, 237, 82, 255) / 255.0f;
	glm::vec4 pastel_blue = glm::vec4(83, 236, 228, 255) / 255.0f;
	glm::vec4 pastel_purple = glm::vec4(181, 150, 243, 255) / 255.0f;
	glm::vec4 pastel_grey = glm::vec4(139, 157, 180, 255) / 255.f;
	glm::vec4 sky = glm::vec4(232, 217, 234, 255) / 255.f;



	wrapper.spheres = {
		Sphere(glm::vec3(0, -1000.5, 0), 1000, 4),
	};
	float radius = 3;
	for (float r = 0; r < PI * 2; r+=PI / 8)
	{
		wrapper.spheres.push_back(
			Sphere{ glm::vec3(std::sin(r) * radius, 0, std::cos(r) * radius), 0.5, (unsigned int) rand(gen)}
		);
	}

	wrapper.materials = std::vector<Material>{
			Material{ pastel_orange, 1},
			Material{ pastel_green, -1.5 },
			Material{ pastel_blue, 0.5 },
			Material{ pastel_purple, 0},
			Material{ pastel_grey, 0 }

	};

	wrapper.camera = CameraWrapper{
		.origin = glm::vec3(0, 0.75, 1),
		.fov = 20
	};

	wrapper.shaderData.backgroundColor = sky;


	wrapper.init();


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

