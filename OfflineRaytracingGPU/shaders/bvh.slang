#include "bvh_header.slang"

public static float4 backgroundColor = float4(1);

public StructuredBuffer<Sphere> spheres;
public StructuredBuffer<Triangle> triangles;
public StructuredBuffer<Material> materials;
public StructuredBuffer<bvhNode> bvhNodes;

public void init_rng(float2 pixel, int windowWidth, int frameCount)
{
    uint seed = (uint) (pixel.x + pixel.y * windowWidth) ^ jenkins_hash(frameCount);
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

public float rand_float()
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
float3 sample_lamberian(float3 normal)
{
    return normal + sample_sphere() * (1 - EPSILON);
}

float schlick_f0_from_ior(float ior)
{
    let sqrt_f0 = (ior - 1) / (ior + 1);
    return sqrt_f0 * sqrt_f0;
}

float schlick_fresnel(float f0, float u5)
{
    return lerp(f0, 1, u5);
}

float4 schlick_fresnel_f4(float4 f0, float u5)
{
    return lerp(f0, 1, u5);
}

float luminance(float4 rgba)
{
    return dot(rgba, float4(0.2126, 0.7152, 0.0722, 1));
}

Scatter scatter(Ray input_ray, Intersection hit, Material material)
{
    // TODO: microfacet BRDF

    float3 incident = normalize(input_ray.direction);
    float incident_dot_normal = dot(incident, hit.normal);
    bool is_front_face = incident_dot_normal < 0;
    float3 N = select(is_front_face, hit.normal, -hit.normal);
    float cos_theta = abs(incident_dot_normal);

    bool is_transmissive = material.metallicOrIor < 0;
    float ior = abs(material.metallicOrIor);
    float ref_ratio = select(is_front_face, 1 / ior, ior);

    // schlick approximation (1 - cos(theta))^5
    float u = 1 - cos_theta;
    float u5 = u * u * u * u * u;

    bool choose_specular = false;
    float4 attenuation = material.color;

    if (is_transmissive)
    {
        bool cannot_refract = ref_ratio * ref_ratio * (1.0 - cos_theta * cos_theta) > 1;
        float f0 = schlick_f0_from_ior(ref_ratio);
        choose_specular = cannot_refract || schlick_fresnel(f0, u5) > rand_float();
        if (choose_specular)
        {
            attenuation = float4(1);
        }
    }
    else if (material.metallicOrIor > 0)
    {
        float metallic = material.metallicOrIor;
        float4 f0 = lerp(GLASS_F0, material.color, metallic);
        float4 F = schlick_fresnel_f4(f0, u5);

        float4 specular = F;
        float4 diffuse = (material.color - GLASS_F0) * (1 - metallic) * (1 - u5) / 0.5;

        float S = luminance(specular);
        float D = luminance(diffuse);
        float specular_pdf = S / (S + D);

        choose_specular = specular_pdf > rand_float();
        if (choose_specular)
        {
            attenuation = specular / specular_pdf;
        }
        else
        {
            attenuation = diffuse / (1 - specular_pdf);
        }
    }

    float3 scattered = float3(0);
    if (choose_specular)
    {
        scattered = reflect(incident, N);
    }
    else if (is_transmissive)
    {
        scattered = refract(incident, N, ref_ratio);
    }
    else
    {
        scattered = sample_lamberian(N);
    }

    Ray outputRay = Ray(point_on_ray(input_ray, hit.t), scattered);
    return Scatter(attenuation, outputRay);
};

float3 point_on_ray(Ray ray, float t)
{
    return ray.origin + t * ray.direction;
}

Intersection no_intersection()
{
    return Intersection(float3(0), -1, 0);
}
bool valid_intersection(Intersection hit)
{
    return hit.t > 0 && hit.t != FLT_MAX;
}

float4 sky_color(Ray ray)
{
    float t = 0.5 * (normalize(ray.direction).y + 1.0);
    return (1 - t) * float4(1) + t * backgroundColor;
}

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
}

Intersection intersect_triangle(Ray ray, Triangle tri)
{
    float normalDotDir = dot(tri.normal, ray.direction);
    if (abs(normalDotDir) < EPSILON) return no_intersection();

    float t = -(dot(tri.normal, ray.origin) + tri.d) / normalDotDir;
    if (t < EPSILON) return no_intersection();

    float3 v0v1 = tri.v1 - tri.v0;
    float3 v0v2 = tri.v2 - tri.v0;
    float3 v1v2 = tri.v2 - tri.v1;
    float3 v2v0 = tri.v0 - tri.v2;

    float3 P = point_on_ray(ray, t);
    float3 C;
    float3 v1p = P - tri.v1;
    C = cross(v1v2, v1p);
    float u = dot(tri.normal, C);
    if (u < 0) return no_intersection();

    float3 v2p = P - tri.v2;
    C = cross(v2v0, v2p);
    float v = dot(tri.normal, C);
    if (v < 0) return no_intersection();

    float3 v0p = P - tri.v0;
    C = cross(v0v1, v0p);
    if (dot(tri.normal, C) < 0) return no_intersection();

    return Intersection(tri.normal, t, tri.materialIndex);
}

Intersection intersect_scene(Ray ray, uint numSpheres, uint numTris)
{
    // Replace o(N) with BVH

    Intersection intersection = no_intersection();
    intersection.t = FLT_MAX;
    for (uint i = 0; i < numSpheres; i++)
    {
        Intersection hit = intersect_sphere(ray, spheres[i]);
        if (hit.t > 0 && hit.t < intersection.t)
        {
            intersection = hit;
        }
    }
    for (uint i = 0; i < numTris; i++)
    {
        Intersection hit = intersect_triangle(ray, triangles[i]);
        if (hit.t > 0 && hit.t < intersection.t)
        {
            intersection = hit;
        }
    }
    return intersection;
}

float intersect_aabb(Ray ray, bvhNode bvh)
{
    float ray_t_min = -FLT_MAX;
    float ray_t_max = FLT_MAX;
    for (uint axis = 0; axis < 3; axis++)
    {
        const float ax_min = bvh.min[axis];
        const float ax_max = bvh.max[axis];
        const float adinv = 1. / ray.direction[axis];
        float t0 = (ax_min - ray.origin[axis]) * adinv;
        float t1 = (ax_max - ray.origin[axis]) * adinv;
        if (t0 < t1)
        {
            if (t0 > ray_t_min) ray_t_min = t0;
            if (t1 < ray_t_max) ray_t_max = t1;
        }
        else
        {
            if (t1 > ray_t_min) ray_t_min = t1;
            if (t0 < ray_t_max) ray_t_max = t0;
        }

        if (ray_t_max <= ray_t_min)
        {
            return FLT_MAX;
        }
    }
    if (ray_t_min > 0)
    {
        return ray_t_min;
    }
    else if (ray_t_max > 0)
    {
        return ray_t_max;
    }
    return FLT_MAX;
}

void ProcessPrimitive(
    PrimType type,
    int index,
    Ray ray,
    inout Intersection closest)
{
    Intersection hit = no_intersection();

    // this check should be unnecessary
    if (index < 0)
    {
        return;
    }
    switch (type)
    {
    case PrimType::SPHERE:
        hit = intersect_sphere(ray, spheres[index]);
        break;
    case PrimType::TRIANGLE:
        hit = intersect_triangle(ray, triangles[index]);
        break;
    default:
        return;
    }

    if (hit.t > 0 && hit.t < closest.t)
    {
        closest = hit;
    }
}

void push_stack(inout uint stack[64],
                inout uint stackPtr,
                uint index)
{
    if (stackPtr < 64)
    {
        stack[stackPtr++] = index;
    }
}

Intersection intersect_scene_bvh(Ray ray, uint rootIndex)
{
    // TODO: try this using a Rope / threaded BVH later
    /*
    push root
    closest.t = FLT_MAX
    while stack not empty:
        node = pop()
        test left child
        test right child
        push BVH children
        test left prim
        test right prim
    return closest
    */
    uint stack[64];
    uint stackPtr = 0;

    stack[stackPtr++] = rootIndex;

    Intersection closest = no_intersection();
    closest.t = FLT_MAX;

    while (stackPtr > 0)
    {
        uint nodeIndex = stack[--stackPtr];
        const bvhNode current = bvhNodes[nodeIndex];

        float leftT = FLT_MAX;
        float rightT = FLT_MAX;

        if (current.leftPrim == PrimType::BVH_NODE)
        {
            leftT = intersect_aabb(ray, bvhNodes[current.leftIndex]);
        }
        else
        {
            ProcessPrimitive(current.leftPrim, current.leftIndex, ray, closest);
        }

        if (current.rightPrim == PrimType::BVH_NODE)
        {
            rightT = intersect_aabb(ray, bvhNodes[current.rightIndex]);
        }
        else
        {
            ProcessPrimitive(current.rightPrim, current.rightIndex, ray, closest);
        }


        if (leftT < rightT)
        {
            if (rightT < closest.t)
            {
                push_stack(stack, stackPtr, current.rightIndex);
            }

            if (leftT < closest.t)
            {
                push_stack(stack, stackPtr, current.leftIndex);
            }
        }
        else
        {
            if (leftT < closest.t)
            {
                push_stack(stack, stackPtr, current.leftIndex);
            }
            if (rightT < closest.t)
            {
                push_stack(stack, stackPtr, current.rightIndex);
            }
        }
    }

    return closest;
}

public float4 ray_color(Ray ray, uint numTris, uint numSpheres, uint bvhRoot)
{
    float4 throughputScatter = float4(1);
    float4 throughputEmission = float4(0);
    float4 radianceSample = float4(0);

    uint pathLength = 0;
    while (pathLength < MAX_PATH_LENGTH)
    {
        // TODO: obv this fails if there is no BVH
        Intersection intersection = intersect_scene_bvh(ray, bvhRoot);
        if (!valid_intersection(intersection))
        {
            radianceSample += throughputScatter * throughputEmission * sky_color(ray);
            break;
        }
        Material material = materials[intersection.materialIndex];
        Scatter scattered = scatter(ray, intersection, material);
        throughputEmission += material.emissiveColor;
        throughputScatter *= scattered.attenuation;
        ray = scattered.ray;
        pathLength += 1;
    }

    return radianceSample;
}