/*

This is what the final frag shader accesses.


*/

module scene;

__include common_types;

__include primitives;

__include shortstackKDOP;

__include bvh;

public float4 ray_color(int rayIndex, uint sceneRoot, float3 camDir, bool firstBounceDebug, float4 backgroundColor)
{
    Ray ray = rays[rayIndex];
    float4 throughputScatter = ray.scatter;
    float4 throughputEmission = ray.emission;
    float4 radianceSample = throughputScatter * throughputEmission;

    uint rootIndex = unpackIndex(sceneRoot);
    PrimType rootType = unpackType(sceneRoot);

    // TODO maybe: let the user pick different traversal algorithms?
    Intersection intersection = intersect_scene_wide_dfs(rayIndex, rootIndex, rootType);
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
        return ray.scatter; // float4(0.5 + (0.5* intersection.normal), 1);
    }

    // TODO later: deal with emission > 1 that blow out the pixels
    return ray.emission * ray.scatter;
}

float gaussian_log(float lambda, float mu, float sigma)
{
    float t = (log(lambda) - log(mu)) / sigma;
    return exp(-0.5 * t * t);
}

float3 cie_matching_functions(float lambda)
{
    // TODO: check these numbers :)
    float x = 1.065 * gaussian_log(lambda, 599.8, 0.0264)
            + 0.366 * gaussian_log(lambda, 442.0, 0.0624);
    float y = 1.014 * gaussian_log(lambda, 556.3, 0.0075);
    float z = 1.839 * gaussian_log(lambda, 449.8, 0.0243);
    return float3(x, y, z);
}

float3 spectral_sample_to_RGB(float value, float lambda, float lambda_pdf)
{
    const float CIE_Y_INTEGRAL = 106.86;
    float3 xyz_response = cie_matching_functions(lambda);

    return (value / lambda_pdf) * xyz_response / CIE_Y_INTEGRAL;
}

public float4 ray_spectral(int rayIndex, uint sceneRoot, float3 camDir, bool firstBounceDebug, float4 backgroundColor)
{
    Ray ray = rays[rayIndex];

    uint rootIndex = unpackIndex(sceneRoot);
    PrimType rootType = unpackType(sceneRoot);
    float lambda_pdf = 1 / (730.f - 380.f);

    Intersection intersection = intersect_scene_wide_dfs(rayIndex, rootIndex, rootType);
    if (!valid_intersection(intersection))
    {
        float skySpectral = rgb_to_spectral(sky_color(ray, backgroundColor), ray.wavelength);
        float finalRadiance = ray.emission + ray.scatter * skySpectral;
        return float4(spectral_sample_to_RGB(finalRadiance, ray.wavelength, lambda_pdf), 1);
    }
    Material material = materials[intersection.materialIndex];

    scatter_spectral(rayIndex, intersection, material, camDir);
    ray = rays[rayIndex];

    float3 retColor;
    

    if (firstBounceDebug)
    {
        retColor = spectral_sample_to_RGB(ray.scatter, ray.wavelength, lambda_pdf);
    }
    else
    {
        retColor = spectral_sample_to_RGB(ray.emission, ray.wavelength, lambda_pdf);
    }

    return float4(retColor, 1);
}