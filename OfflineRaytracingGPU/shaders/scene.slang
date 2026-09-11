/*

This is what the final frag shader accesses.


*/

module scene;

__include common_types;

__include primitives;

__include shortstackKDOP;

__include bvh;


float gaussian_linear(float lambda, float mu, float sigma)
{
    float t = (lambda - mu) / sigma;
    return exp(-0.5 * t * t);
}

float gaussian_log(float lambda, float mu, float sigma)
{
    float t = (log(lambda) - log(mu)) / sigma;
    return exp(-0.5 * t * t);
}

float3 cie_matching_functions(float lambda)
{
    // Wyman, Sloan & Shirley 2013, single-lobe fit to CIE 1931 (Eq. 2)
    float x = 1.065 * gaussian_linear(lambda, 595.8, 33.33)
            + 0.366 * gaussian_linear(lambda, 446.8, 19.44);
    float y = 1.014 * gaussian_log(lambda, 556.3, 0.075);
    float z = 1.839 * gaussian_log(lambda, 449.8, 0.051);
    return float3(x, y, z);
}

float3 spectral_sample_to_RGB(float value, float lambda, float lambda_pdf)
{
    const float CIE_Y_INTEGRAL = 106.86;
    float3 xyz_response = cie_matching_functions(lambda);

    return (value / lambda_pdf) * xyz_response / CIE_Y_INTEGRAL;
}

public float4 ray_spectral(Ray ray, uint sceneRoot, float3 camDir, bool firstBounceDebug, float4 backgroundColor)
{
    uint rootIndex = unpackIndex(sceneRoot);
    PrimType rootType = unpackType(sceneRoot);
    float lambda_pdf = 1 / (730.f - 380.f);
    static const float3x3 XYZ_TO_SRGB = float3x3(
     3.2406, -1.5372, -0.4986,
    -0.9689, 1.8758, 0.0415,
     0.0557, -0.2040, 1.0570
    );

    Intersection intersection = intersect_scene_wide_dfs(ray, rootIndex, rootType);
    if (!valid_intersection(intersection))
    {
        float skySpectral = rgb_to_spectral(sky_color(ray, backgroundColor), ray.wavelength);
        float finalRadiance = ray.emission + ray.scatter * skySpectral;

        return float4(mul(XYZ_TO_SRGB, spectral_sample_to_RGB(finalRadiance, ray.wavelength, lambda_pdf)), 1);
    }
    Material material = materials[intersection.materialIndex];

    float2 spectral = scatter_spectral(ray, intersection, material, camDir);


    // write spectral to currentFrameSpectral
    float3 retColor = spectral_sample_to_RGB(spectral.y, ray.wavelength, lambda_pdf);
    
    if (firstBounceDebug)
    {
        retColor = intersection.normal; //spectral_sample_to_RGB(spectral.x, ray.wavelength, lambda_pdf);
    }

    
    
    return float4(mul(XYZ_TO_SRGB, retColor), 1);
}
