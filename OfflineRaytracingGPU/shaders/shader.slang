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
    uint2 uv = (uint2) input.Pos.xy;
    float4 historyColor = previousFrame.Sample(uv);

    float4 radianceSample = float4(bit_cast<float3>(currentFrameRGB[uv]), 1.); // this is written by the compute shader

    float weight = 1.0 / (shaderData.frameCount + 1.0);
    if (weight < 1)
    {
        weight /= 10 - shaderData.bounceCount;
    }
    
    float4 outColor = lerp(historyColor, radianceSample, weight);

    return outColor;
}

/*
Theory here:

TODO:
could have each ray keep track of its contribution (1 / the number of rays in this pixel this bounce) and have them write to
currentFrame whenever they call scatter

Compute Shader:
has access to primitives and two buffers:
rayQueueA
rayQueueB

Every frame the write queue and read queue get swapped

every ray in the read queue gets processed for one bounce.
new rays get pushed to the write queue

If the write queue is empty:
    this was the last bounce, fill the write queue with default rays

As a ray from readQueue is processed, its color gets written to the corresponding pixel index in `RWTexture2D currentFrame;`

Frag Shader:
has access to two texture2D objects:
currentFrame -> what compute shader has been writing to
historyFrame -> all images that have been completed at this location

frag shader lerps currentFrame and historyFrame and returns that color value per pixel.


Vulkan:
on the last bounce: frag shader output is written to history frame
every other bounce: frag shader output displayed as normal
*/

[shader("compute")]
[numthreads(1,1,1)] // TODO: adjust this to match max queue size
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint index = threadId.x;
    uint numStartRays = shaderData.windowMax.x * shaderData.windowMax.y;
    Ray ray;
    bool validRay = false;
    // dont have a thread that is outside the screen generate a ray
    if ((bool) shaderData.resetRays && index < numStartRays)
    {
        // generate new ray
        float3 origin = shaderData.camera.origin;
        float focus_distance = 1.0;
        float aspect_ratio = shaderData.windowMax.x / shaderData.windowMax.y;
        float fov_y = radians(shaderData.camera.fov);
        float viewport_height = 2 * tan(fov_y * 0.5) * focus_distance;
        float viewport_width = viewport_height * aspect_ratio;
        float2 offset = float2(rand_float() - 0.5, rand_float() - 0.5);

        // get screen coords from index
        float2 screenCoords = float2(index % shaderData.windowMax.x, (index - (index % shaderData.windowMax.x)) / shaderData.windowMax.x);
        float2 uv = (screenCoords + offset) / shaderData.windowMax;
        uv = (uv - float2(0.5)) * float2(viewport_width, -viewport_height);

        float3 direction = normalize(mul(shaderData.camera.rotationMatrix, float3(uv, focus_distance)));
        ray = makeRay(index, origin, direction);
        validRay = true;
    }
    else
    {
        validRay = getRay(index, shaderData.readQueueLen, ray);
    }

    // process the ray at index in readQueue
    bool firstBounce = shaderData.bounceCount == 0;
    if (validRay)
    {
        float4 retColor = ray_spectral(ray, shaderData.sceneRoot, shaderData.camDir, firstBounce, shaderData.backgroundColor); 
        // TODO: need to add retColor in accordance with ray's pdf
        // for now, lets just pretend like there is only one ray per pixel, so simply overwriting whatever is there is fine
        currentFrameRGB[ray.index] = bit_cast<uint3>(retColor.xyz);
    }
}