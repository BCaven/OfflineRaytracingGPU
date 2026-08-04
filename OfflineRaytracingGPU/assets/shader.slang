/*

*/

static const float FLT_MAX = 3.40282346638528859812e+38;
static const float EPSILON = 1e-3;
static const uint MAX_PATH_LENGTH = 20;
static const float TWO_PI = 6.2831853;

struct VSInput 
{
    float3 Pos;
};

struct Camera
{
    float3x3 lookAtMatrix;
    float3 origin;
};

// RNG
// https://raytracing.github.io/gpu-tracing/book/RayTracingGPUEdition.html
struct Rng {
    uint state;
}
static Rng rng;

void init_rng(float2 pixel, VSOutput sceneData)
{
    uint seed = (pixel.x + pixel.y * sceneData.windowMax.x) ^ jenkins_hash(sceneData.frameCount);
    rng.state = jenkins_hash(seed);
}

uint jenkins_hash(uint i)
{
    uint x = i;
    x += x << 10u;
    x ^= x >> 6u;
    x += x << 3u;
    x ^= x >> 11u;
    x += x << 15u;
    return x;
}

uint xorshift()
{
    uint x = rng.state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng.state = x;
    return x;
}

float rand_float()
{
    return bit_cast<float>(0x3f800000u | (xorshift() >> 9u)) - 1.0;
}

float3 sample_sphere()
{
    float r0 = rand_float();
    float r1 = rand_float();
    float y = 1 - (2 * r0);
    float xz_r = sqrt(1 - y * y);
    float phi = TWO_PI * r1;
    return float3(xz_r * cos(phi), y, xz_r * sin(phi));
}

// Raytracing items

struct Scatter 
{
    float4 attenuation;
    Ray ray;
};
float3 sample_lamberian(float3 normal)
{
    return normal + sample_sphere() * (1 - EPSILON);
}

Scatter scatter(Ray input_ray, Intersection hit, Material material)
{
    float3 incident = normalize(input_ray.direction);
    float incident_dot_normal = dot(incident, hit.normal);
    bool is_front_face = incident_dot_normal < 0;
    float3 N = select(is_front_face, hit.normal, -hit.normal);
    float cos_theta = abs(incident_dot_normal);

    float3 scattered = float3(0);
    if (material.ior > 0)
    {
        float ior = material.ior;
        float ref_ratio = select(is_front_face, 1 / ior, ior);
        bool cannot_refract = ref_ratio * ref_ratio * (1.0 - cos_theta * cos_theta) > 1;
        if (cannot_refract)
        {
            scattered = reflect(incident, N);
        }
        else
        {
            scattered = refract(incident, N, ref_ratio);
        }
    } 
    else if (material.specular == 1)
    {
        scattered = reflect(incident, N);
    } 
    else 
    {
        scattered = sample_lamberian(N);
    }

    Ray outputRay = Ray(point_on_ray(input_ray, hit.t), scattered);
    float4 attenuation = material.color;
    return Scatter(attenuation, outputRay);
};

struct Ray 
{
    float3 origin;
    float3 direction;
};
float3 point_on_ray(Ray ray, float t)
{
    return ray.origin + t * ray.direction;
};

struct Intersection
{
    float3 normal;
    float t;
    uint materialIndex;
};
Intersection no_intersection()
{
    return Intersection(float3(0), -1, 0);
};
bool valid_intersection(Intersection hit)
{
    return hit.t > 0 && hit.t != FLT_MAX;
}

// Materials
struct Material
{
    float4 color;
    float specular;
    float ior;
};

// Primitives

struct Sphere
{
    float3 center;
    float radius;
    uint materialIndex;
};

// Intersection algorithms
Intersection intersect_scene(Ray ray, VSOutput sceneData)
{
    Intersection intersection = no_intersection();
    intersection.t = FLT_MAX;
    for (int i = 0; i < sceneData.numSpheres; i++)
    {
        Intersection hit = intersect_sphere(ray, spheres[i]);
        if (hit.t > 0 && hit.t < intersection.t)
        {
            intersection = hit;
        }
    }
    return intersection;
};

Intersection intersect_sphere(Ray ray, Sphere sphere)
{
    float3 v = sphere.center - ray.origin;
    float a = dot(ray.direction, ray.direction);
    float b = dot(v, ray.direction);
    float c = dot(v, v) - sphere.radius * sphere.radius;
    float d = b * b - a * c;
    if (d < 0.0)
    {
        return no_intersection();
    }
    float sqrt_d = sqrt(d);
    float recip_a = 1 / a;
    float t0 = (b - sqrt_d) * recip_a;
    float t1 = (b + sqrt_d) * recip_a;
    float t = select(t0 > EPSILON, t0, t1);
    if (t <= EPSILON)
    {
        return no_intersection();
    }
    float3 p = point_on_ray(ray, t);
    float3 N = (p - sphere.center) / sphere.radius;
    return Intersection(N, t, sphere.materialIndex);
};


// Scene data

struct ShaderData 
{
    float2 windowMax;
    int numSpheres;
    int frameCount;
    Camera camera;
};

StructuredBuffer<Sphere> spheres;
StructuredBuffer<Material> materials;

// accumulation of previous samples
Sampler2D previousFrame;

// Shared between vert and frag shaders

struct VSOutput 
{
    float4 Pos : SV_POSITION;
    float2 windowMax;
    float numSpheres;
    int frameCount;
    Camera camera;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData *shaderData, uint instanceIndex : SV_VulkanInstanceID) 
{
    VSOutput output;
    output.Pos = float4(input.Pos.xyz, 1.0);
    output.windowMax = shaderData.windowMax;
    output.numSpheres = shaderData.numSpheres;
    output.frameCount = shaderData.frameCount;
    output.camera = shaderData.camera;
    return output;
}

float4 sky_color(Ray ray)
{
    float t = 0.5 * (normalize(ray.direction).y + 1.0);
    return (1 - t) * float4(1) + t * float4(0.3, 0.5, 1.0, 1.0); 
}


[shader("fragment")]
float4 main(VSOutput input)
{
    init_rng(input.Pos.xy, input);
    float3 origin = input.camera.origin;
    float focus_distance = 1.0;
    float aspect_ratio = input.windowMax.x / input.windowMax.y;

    float2 offset = float2(rand_float() - 0.5, rand_float() - 0.5);
   
    float2 uv = (input.Pos.xy + offset) / input.windowMax;
    uv = (2.0 * uv - float2(1)) * float2(aspect_ratio, -1);


    float3 direction = mul(input.camera.lookAtMatrix, float3(uv, focus_distance));
    Ray ray = Ray(origin, direction);

    float4 throughput = float4(1);
    float4 radianceSample = float4(0);

    uint pathLength = 0;
    while (pathLength < MAX_PATH_LENGTH)
    {
        Intersection intersection = intersect_scene(ray, input);
        if (!valid_intersection(intersection))
        {
            radianceSample += throughput * sky_color(ray);
            break;
        }
        Material material = materials[intersection.materialIndex];
        Scatter scattered = scatter(ray, intersection, material);
        throughput *= scattered.attenuation;
        ray = scattered.ray;
        pathLength += 1;
    }
    
    // TODO remove this redundant calc
    uv = input.Pos.xy / input.windowMax;
    float4 historyColor = previousFrame.Sample(uv);
    float weight = 1.0 / (input.frameCount + 1.0);

    float4 outColor = lerp(historyColor, radianceSample, weight);
    return outColor;
    
   
}