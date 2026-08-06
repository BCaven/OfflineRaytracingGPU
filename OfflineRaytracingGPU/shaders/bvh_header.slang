#pragma once

public static const float FLT_MAX = 3.40282346638528859812e+38;
public static const float EPSILON = 1e-3;
public static const uint MAX_PATH_LENGTH = 5;
public static const float TWO_PI = 6.2831853;
public static const float4 GLASS_F0 = float4(0.04);

// RNG
// https://raytracing.github.io/gpu-tracing/book/RayTracingGPUEdition.html
struct Rng {
    uint state;
}
static Rng rng;

// Raytracing
public struct Ray
{
    public float3 origin;
    public float3 direction;
};

struct Scatter
{
    float4 attenuation;
    Ray ray;
};

struct Intersection
{
    float3 normal;
    float t;
    uint materialIndex;
};

// Materials
public struct Material
{
    float4 color;
    float metallicOrIor;
    float4 emissiveColor;
};

// Primitives

public struct Sphere
{
    float3 center;
    float radius;
    uint materialIndex;
};

public struct Triangle
{
    float3 v0;
    float3 v1;
    float3 v2;
    float3 normal;
    float d; // plane equation parameter
    uint materialIndex;
};

public enum PrimType
{
    MATERIAL,
    SPHERE,
    TRIANGLE,
    BVH_NODE,
    EMPTY
};

public struct bvhNode
{
    float3 min;
    float3 max;
    uint64_t mortonCode;
    PrimType rightPrim;
    PrimType leftPrim;
    int leftIndex;
    int rightIndex;
}