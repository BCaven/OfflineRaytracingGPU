# Offline GPU Path Tracer

Progressive Monte Carlo GPU path tracer written in Slang and C++ using Vulkan.
![transforms](ProjectAssets/transformed_spheres.png)
![5.9m splats](ProjectAssets/SplatInterior_FirstImage.png)
![Second image](ProjectAssets/SuzanneWithSpheres.png)
![First image](ProjectAssets/FirstImage_Spheres.png)

# Project features

- Sphere primitives
- Triangle primitives + OBJ loader
- BSDF mmaterial support for ior and metallic
- CPU BVH construction and GPU traversal
- Lights
- Gaussian Splat primitives
- Progressve path tracing with temporal accumulation
- transforms (location, rotation, scale)
- instancing
- TLAS/BLAS separation
- Mixed node types (transform, primitive, binary AABB, 8-wide KDOP)
