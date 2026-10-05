# Offline GPU Path Tracer

Progressive Spectral Monte Carlo GPU path tracer written in Slang and C++ using Vulkan.
![5.9m splats](ProjectAssets/SplatInterior.png)


# Project features

- Sphere primitives
- Triangle primitives + OBJ loader
- BSDF material support for ior and metallic
- CPU BVH construction and GPU traversal
- Mesh lights
- Gaussian Splat primitives + PLY loader
- Progressive path tracing with temporal accumulation
- Transforms (location, rotation, scale)
- Instancing
- TLAS/BLAS separation
- Mixed node types (transform, primitive, binary AABB, 8-wide KDOP)
- Nested instances
- Spectral sampling
- Compute shader pipeline
- Animations with variable time step

# Implementation details

This section does not cover every aspect of the engine. (e.g. Triangle, and Sphere intersections will not be covered)

## Transforms

Transforms are the easiest way to make instances of BLAS. Transforms have a single child that can be any node type (Inner node, Primitive, other transform, etc.).\
Since transforms point to an arbitrary child, it allows for construction of nested instances.

## Nested Instances

The following will use these definitions: \
Primitive: a single triangle, sphere, gaussian splat, etc. \
Instance: a collection of primitives contained in a BLAS. \
Collection: group of instances in an acceleration structure. \
Note: collections can contain other collections. This node structure means the following tree is possible:

```
root TLAS
├── transform
|       └── pointer to Collection 1
└── transform
        └── pointer to Collection 1

Collection 1 TLAS
        ├── pointer to Instance BLAS
        └── transform 
                └── pointer to Instance BLAS
```

Using this structure, an arbitrarily large number of primitives can be represented on a smaller footprint while also having a small build time.

For example: a N by N by N cube of Suzannes would contain N^3 Suzanne instances in a non-nested TLAS, but a nested TLAS can represent the same scene by repeating instances of collections of Suzannes. \
This lowers the memory footprint and build time for scenes with repeated groups of instances, \
allowing for much larger scenes in the same memory footprint and build-time constraints.

### Self referencing instances

Arbitrary nested instances also allow for procedurally generated and rendered recursive collections that are traversed and rendered as visible to the camera. \
This means fractal structures are supported by storing a single branch, and other recursive trees. This is only partially supported by stack based dfs, \
with further support planned using a stackless traversal algorithm.

Below is inital testing using a collection containing Suzanne and a reference to the collection that has been rotated and scaled, creating a spiral pattern.
![initial testing using self references](ProjectAssets/first_fractal.png)

## Gaussian Splat primitives

The PLY loader is adapted from [3D Gaussian Splatting in a Weekend](https://bfeldman.me/3dgs-weekend/).

Collisions with gaussian splat primitives are calculated using their covariance matrix and center. \
The path tracer takes advantage of its Monte Carlo architecture to stochastically hit/miss splat primitives. \
By averaging samples across many rays, the path tracer simulates alpha blending of many splats.

Read more about this type of gaussian splat ray tracing here: [Stochastic Ray Tracing of Transparent 3D Gaussians](https://arxiv.org/pdf/2504.06598)

## Time and animations

Animations are supported by accessing `wrapper.time` during scene construction. \
Manual time steps are supported as well as automated rendering of animation sequences. Timesteps (`wraper.time_delta`) can also be changed during an animation sequence, allowing for easy interpolation and modification of time in the animation. \
This results in effects such as:
- real-time to slow motion
- reversing time mid animation
- fast forward

# Building and contributing

The project uses [cmake](https://cmake.org/) and requires [vulkan SDK](https://vulkan.org/tools#download-these-essential-development-tools) and [tinyexr](https://github.com/syoyo/tinyexr).\
When installing vulkan, `volk`, `VMA`, `SDL`, and `GLM` need to be selected. \
tinyexr is included as a git submodule.

1. Install Vulkan with requirements (volk, VMA, SDL, GLM)
2. Clone repository: `git clone https://github.com/BCaven/OfflineRaytracingGPU.git`
3. Get tinyexr: `cd OfflineRaytracingGPU && git submodule update --init --recursive`
4. Build: \
   with visual studio: open `OfflineRaytracingGPU` folder in Visual Studio and select `OfflineRaytracingGPU/CmakeList.txt` when prompted \
   with cmake: `cmake OfflineRaytracingGPU/ -B OfflineRaytracingGPU/out/ && cmake --build OfflineRaytracingGPU/out/`

# References:

Sun, Xin, et al. "Stochastic Ray Tracing of Transparent 3D Gaussians." arXiv preprint arXiv:2504.06598 (2025). \
Feldman, Benjamin. (May 2026). “3D Gaussian Splatting in a Weekend”. bfeldman.me. https://bfeldman.me/3dgs-weekend/. \
Arman Uguray. "Ray Tracing: GPU Edition". https://raytracing.github.io/gpu-tracing/book/RayTracingGPUEdition.html. \
Vaidyanathan, Karthik, Sven Woop, and Carsten Benthin. "Wide BVH traversal with a short stack." Proceedings of the Conference on High-Performance Graphics. 2019.

# Gallery
![spectral scattering](ProjectAssets/IcospherePrism.png)
![first gif](ProjectAssets/prism_3.gif)
![transforms](ProjectAssets/transformed_spheres.png)
![Second image](ProjectAssets/SuzanneWithSpheres.png)
![First image](ProjectAssets/FirstImage_Spheres.png)

8 billion instances of Suzanne illuminated by sphere mesh lights:
![8b Suzanne](ProjectAssets/8b_suzannes_with_lights.png)
