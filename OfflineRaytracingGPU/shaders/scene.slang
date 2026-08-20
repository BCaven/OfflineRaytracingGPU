/*

This is what the final frag shader accesses.


*/

module scene;

__include common_types;

__include primitives;

__include shortstackKDOP;

__include bvh;

public float4 ray_color(int rayIndex, uint bvhRoot, PrimType rootType, float3 camDir, bool firstBounceDebug, float4 backgroundColor)
{
    Ray ray = rays[rayIndex];
    float4 throughputScatter = ray.scatter;
    float4 throughputEmission = ray.emission;
    float4 radianceSample = throughputScatter * throughputEmission;

    // TODO maybe: let the user pick different traversal algorithms?
    Intersection intersection = intersect_scene_wide_dfs(rayIndex, bvhRoot, rootType);
    if (!valid_intersection(intersection))
    {
        return throughputScatter * (throughputEmission + sky_color(ray, backgroundColor));
    }
    Material material = materials[intersection.materialIndex];

    scatter(rayIndex, intersection, material, camDir);
    ray = rays[rayIndex];

    if (firstBounceDebug)
    {
        // maybe do lambertian shading?
        // or depth? XD
        // float light = dot(intersection.normal, float3(0, 1, 0));
        return ray.scatter;
    }

    // TODO later: deal with emission > 1 that blow out the pixels
    return ray.emission * ray.scatter;
}