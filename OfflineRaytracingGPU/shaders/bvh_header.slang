#pragma once

public static const float FLT_MAX = 3.40282346638528859812e+38;
public static const float EPSILON = 1e-3;
public static const float TWO_PI = 6.2831853;
public static const float4 GLASS_F0 = float4(0.04);

public static const float SPLAT_CUTOFF = 8.;
public static const int SH_COUNT = 16;

public static const float SH_C0 = 0.28209479177387814f;
public static const float SH_C1 = 0.4886025119029199f;
public static const float SH_C2[] = {
    1.0925484305920792f,
    -1.0925484305920792f,
    0.31539156525252005f,
    -1.0925484305920792f,
    0.5462742152960396f
};
public static const float SH_C3[] = {
    -0.5900435899266435f,
    2.890611442640554f,
    -0.4570457994644658f,
    0.3731763325901154f,
    -0.4570457994644658f,
    1.445305721320277f,
    -0.5900435899266435f
};


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
    public float4 scatter;
    public float4 emission;
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
    int SHIndex = -1;
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

public struct GaussianSplat
{
    float3 center;
    float3x3 invSigma;
    float3x3 rotation;
    float3 invScale2;
    float alpha;
    uint materialIndex;
    float3 halfExtent;
};

public struct SphericalHarmonic
{
    float3 sh[SH_COUNT];
    int degree = 0;
};

public enum PrimType
{
    SPHERE,
    TRIANGLE,
    GAUSSIAN_SPLAT,
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