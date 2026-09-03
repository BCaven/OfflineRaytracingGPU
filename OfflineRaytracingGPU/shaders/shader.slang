/*
TODO:
- Make BVH primitive agnostic
- rewrite function headers to be better separated


*/

#include "shader_header.slang"

import scene;


[shader("vertex")]
VSOutput main(VSInput input, uint instanceIndex : SV_VulkanInstanceID)
{
    VSOutput output;
    output.Pos = float4(input.Pos.xyz, 1.0);
    return output;
}

[[vk::push_constant]]
ShaderData shaderData;

[shader("fragment")]
float4 main(VSOutput input)
{
    // TODO: the shader actually has no idea what the max bounce count is :)
    init_rng(input.Pos.xy, shaderData.windowMax.x, (shaderData.frameCount * 10) + shaderData.bounceCount);
    int rayIndex = (int)(input.Pos.x + (input.Pos.y * shaderData.windowMax.x));

    if ((bool) shaderData.resetRays)
    {
        float3 origin = shaderData.camera.origin;
        float focus_distance = 1.0;
        float aspect_ratio = shaderData.windowMax.x / shaderData.windowMax.y;
        float fov_y = radians(shaderData.camera.fov);
        float viewport_height = 2 * tan(fov_y * 0.5) * focus_distance;
        float viewport_width = viewport_height * aspect_ratio;
        float2 offset = float2(rand_float() - 0.5, rand_float() - 0.5);

        float2 uv = (input.Pos.xy + offset) / shaderData.windowMax;
        uv = (uv - float2(0.5)) * float2(viewport_width, -viewport_height);

        float3 direction = normalize(mul(shaderData.camera.rotationMatrix, float3(uv, focus_distance)));
        reset_ray(rayIndex, origin, direction);
    }

    bool lambertianLighting = (bool) shaderData.resetRays && shaderData.frameCount == 0;
    // TODO remove this redundant calc
    float2 uv = input.Pos.xy / shaderData.windowMax;
    float4 historyColor = previousFrame.Sample(uv);
    float weight = 1.0 / (shaderData.frameCount + 1.0);
    if (weight < 1)
    {
        weight /= 10 - shaderData.bounceCount;
    }
    float4 radianceSample = ray_color(rayIndex, shaderData.sceneRoot, shaderData.camDir, lambertianLighting, shaderData.backgroundColor);
    
    float4 outColor = lerp(historyColor, radianceSample, weight);

    return outColor;
}