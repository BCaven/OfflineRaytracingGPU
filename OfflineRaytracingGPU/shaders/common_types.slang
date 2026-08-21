implementing scene;

public static const float FLT_MAX = 3.40282346638528859812e+38;
public static const float EPSILON = 1e-3;
public static const float TWO_PI = 6.2831853;
public static const float4 GLASS_F0 = float4(0.04);

public static const int KDOP_WIDTH = 8;

static const uint STACK_SIZE = 64;
static const uint TRANSFORM_STACK_SIZE = 32;

static const uint SHORT_STACK_SIZE = 15;
static const uint RESTART_TRAIL_SIZE = 512; // this should be the max depth of the bvh

static const uint INVALID_NODE = 0xFFFFFFFFu;
static const uint FULL_MASK = (1u << KDOP_WIDTH) - 1u; // 0xFF

public static const float SPLAT_CUTOFF = 8.;
public static const int SH_COUNT = 16;
public static const uint MAX_TRAVERSALS = 30000;

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

public struct GaussianSplat // packing shenanigans available here. Probably more important to hit fewer of these
{
    float3 center;
    float3x3 rotation; //
    float3 invScale2;
    float alpha;
    uint materialIndex;
    float3 halfExtent; // CPU only - not needed by collision algorithm
};

public struct SphericalHarmonic
{
    float3 sh[SH_COUNT];
    int degree = 0;
};

public enum PrimType : uint
{
    SPHERE,
    TRIANGLE,
    GAUSSIAN_SPLAT,
    TRANSFORM,
    BVH_NODE,
    KDOP_NODE,
    EMPTY
};

public struct bvhPacked
{
    float4 leftMin_leftPacked;  // xyz left.min, w = packed left.type / left.index
    float4 leftMax_rightPacked; // xyz left.max, w = packed right.type / right.index
    float4 rightMin_reserved0;  // xyz right.min, w unused
    float4 rightMax_reserved1;  // same
};

public static const float invSqrt3 = 0.57735026919;
public static const float3 KDOP_DIRECTIONS[7] =
{
	{ 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
    { invSqrt3, invSqrt3, invSqrt3 },
    { invSqrt3, invSqrt3, -invSqrt3 },
    { invSqrt3, -invSqrt3, invSqrt3 },
    { invSqrt3, -invSqrt3, -invSqrt3 }
};

public struct K14DopNodeCold
{
    float4 min[KDOP_WIDTH], max[KDOP_WIDTH];
};
public struct KDopNodeHot
{
    float4 min_packed[KDOP_WIDTH]; // w is packed index/type
    float4 max[KDOP_WIDTH];        // w reserved
};

public struct Transform
{
    float4x4 matrix;
    float4x4 invMatrix;
    PrimType childPrim;
    int childIndex;
}