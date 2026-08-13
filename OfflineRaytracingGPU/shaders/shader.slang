/*
TODO:
- Make BVH primitive agnostic
- rewrite function headers to be better separated


*/

#include "shader_header.slang"

import bvh;



[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData *shaderData, uint instanceIndex : SV_VulkanInstanceID)
{
    VSOutput output;
    output.Pos = float4(input.Pos.xyz, 1.0);
    output.windowMax = shaderData.windowMax;
    output.numSpheres = shaderData.numSpheres;
    output.numTris = shaderData.numTris;
    output.frameCount = shaderData.frameCount;
    output.camera = shaderData.camera;
    output.backgroundColor = shaderData.backgroundColor;
    output.bvhRoot = shaderData.bvhRoot;
    output.resetRays = shaderData.resetRays;
    output.bounceCount = shaderData.bounceCount;
    output.camDir = shaderData.camDir;
    return output;
}



[shader("fragment")]
float4 main(VSOutput input)
{
    // TODO: the shader actually has no idea what the max bounce count is :)
    init_rng(input.Pos.xy, input.windowMax.x, (input.frameCount * 10) + input.bounceCount);
    backgroundColor = input.backgroundColor;
    int rayIndex = (int)(input.Pos.x + (input.Pos.y * input.windowMax.x));

    if (input.resetRays)
    {
        float3 origin = input.camera.origin;
        float focus_distance = 1.0;
        float aspect_ratio = input.windowMax.x / input.windowMax.y;
        float fov_y = radians(input.camera.fov);
        float viewport_height = 2 * tan(fov_y * 0.5) * focus_distance;
        float viewport_width = viewport_height * aspect_ratio;
        float2 offset = float2(rand_float() - 0.5, rand_float() - 0.5);

        float2 uv = (input.Pos.xy + offset) / input.windowMax;
        uv = (uv - float2(0.5)) * float2(viewport_width, -viewport_height);

        float3 direction = mul(input.camera.rotationMatrix, float3(uv, focus_distance));
        reset_ray(rayIndex, origin, direction);
    }

    bool lambertianLighting = input.resetRays && input.frameCount == 0;
    float4 radianceSample = ray_color(rayIndex, input.bvhRoot, input.camDir, lambertianLighting);
    
    // TODO remove this redundant calc
    float2 uv = input.Pos.xy / input.windowMax;
    float4 historyColor = previousFrame.Sample(uv);
    float weight = 1.0 / (input.frameCount + 1.0);

    float4 outColor = lerp(historyColor, radianceSample, weight);
    return outColor;
}