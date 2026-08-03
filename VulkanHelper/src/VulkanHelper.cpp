// VulkanHelper.cpp : Defines the entry point for the application.
//


#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "VulkanWrapper.h"


using namespace std;

int main()
{
	std::cout << "Hello World!\n";

	VK_Wrap wrapper;

	wrapper.spheres = {
		Sphere{glm::vec3(-1, 0, -1), 0.5, glm::vec4(1, 0, 0, 1)},
		Sphere{glm::vec3(0, 0, -1), 0.5, glm::vec4(1, 1, 0, 1)},
		Sphere{glm::vec3(1, 0, -1), 0.5, glm::vec4(1, 1, 1, 1)},
		Sphere{glm::vec3(0, 0, 0), 0.5, glm::vec4(0, 0, 1, 1)},
		Sphere{glm::vec3(0, 0, 1), 0.5, glm::vec4(0, 0, 1, 1)},

		Sphere(glm::vec3(0, -1002, 0), 1000, glm::vec4(0, 1, 0, 1)),

	};
	wrapper.camera = CameraWrapper{
		.origin = glm::vec3(0, 0.75, 1),
		.direction = glm::vec3(0, -0.5, -1),
		.up = glm::vec3(0, 1, 0)
	};

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

