#pragma once

struct VSInput
{
    float3 Pos;
};

struct Camera
{
    float3x3 rotationMatrix;
    float3 origin;
    float fov;
};

// Scene data

struct ShaderData
{
    uint2 windowMax;
    int frameCount;
    float4 backgroundColor;
    Camera camera;
    uint sceneRoot;
    uint resetRays;
    float3 camDir;
    int bounceCount;
};

// accumulation of previous samples
Sampler2D previousFrame;

// Shared between vert and frag shaders

struct VSOutput
{
    float4 Pos : SV_POSITION;
};