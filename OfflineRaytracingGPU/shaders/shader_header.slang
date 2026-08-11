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
    uint numSpheres;
    uint numTris;
    int frameCount;
    float4 backgroundColor;
    Camera camera;
    uint bvhRoot;
    bool resetRays;
    float3 camDir;
};

// accumulation of previous samples
Sampler2D previousFrame;

// Shared between vert and frag shaders

struct VSOutput
{
    float4 Pos : SV_POSITION;
    uint2 windowMax;
    uint numSpheres;
    uint numTris;
    int frameCount;
    float4 backgroundColor;
    Camera camera;
    uint bvhRoot;
    bool resetRays;
    float3 camDir;
};