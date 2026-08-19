implementing scene;

// TODO: put all of the things that are the same accross BVH architectures (like primitive intersections) in here
// maybe rename it to common or something


public StructuredBuffer<Sphere> spheres;
public StructuredBuffer<Triangle> triangles;
public StructuredBuffer<GaussianSplat> splats;
public StructuredBuffer<SphericalHarmonic> shMats;
public StructuredBuffer<Material> materials;
public StructuredBuffer<bvhPacked> bvhNodes;
public StructuredBuffer<KDopNodeHot> kdopHotNodes;
public StructuredBuffer<K14DopNodeCold> kdopColdNodes;

public StructuredBuffer<Transform> transforms;
public RWStructuredBuffer<Ray> rays;

public void reset_ray(int rayIndex, float3 origin, float3 direction)
{
    rays[rayIndex] = Ray(origin, direction, float4(1), float4(0));
}

PrimType unpackType(uint packed)
{
    return PrimType(packed >> 28);
}
int unpackIndex(uint packed)
{
    return int(packed & 0x0FFFFFFFu);
}
uint packChild(PrimType type, int index)
{
    return (uint(type) << 28) | (uint(index) & 0x0FFFFFFFu);
}

public void init_rng(float2 pixel, int windowWidth, int frameCount)
{
    uint seed = (uint)(pixel.x + pixel.y * windowWidth) ^ jenkins_hash(frameCount);
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

// TODO later: Ziggurat method iff this is slow
float sample_normal_01()
{
    float r1 = max(rand_float(), 1e-8);
    float r2 = max(rand_float(), 1e-8);
    return sqrt(-2.0 * log(r1)) * cos(TWO_PI * r2);
}

float sample_normal_distribution(float mean, float std_dev)
{
    return mean + std_dev * sample_normal_01();
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

float3x3 identity3x3()
{
    return float3x3(
        float3(1, 0, 0),
        float3(0, 1, 0),
        float3(0, 0, 1)
    );
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

void scatter(int rayIndex, Intersection hit, Material material, float3 camDir)
{
    // TODO: microfacet BRDF
    // Also TODO: energy

    Ray input_ray = rays[rayIndex];

    float3 incident = normalize(input_ray.direction);
    float incident_dot_normal = dot(incident, hit.normal);
    bool is_front_face = incident_dot_normal < 0;
    float3 N = select(is_front_face, hit.normal, -hit.normal);
    float cos_theta = abs(incident_dot_normal);
    bool useSH = material.SHIndex >= 0;
    bool is_transmissive = material.metallicOrIor < 0;
    float ior = abs(material.metallicOrIor);
    float ref_ratio = select(is_front_face, 1 / ior, ior);

    // schlick approximation (1 - cos(theta))^5
    float u = 1 - cos_theta;
    float u5 = u * u * u * u * u;

    bool choose_specular = false;
    float4 attenuation = material.color;

    if (useSH)
    {
        float3 dir = camDir;
        choose_specular = false;
        is_transmissive = false;
        SphericalHarmonic shMat = shMats[material.SHIndex];
        float3 result = SH_C0 * shMat.sh[0];
        if (shMat.degree > 0)
        {
            float x = dir.x;
            float y = dir.y;
            float z = dir.z;
            result = result - SH_C1 * y * shMat.sh[1] + SH_C1 * z * shMat.sh[2] - SH_C1 * x * shMat.sh[3];

            if (shMat.degree > 1)
            {
                float xx = x * x, yy = y * y, zz = z * z;
                float xy = x * y, yz = y * z, xz = x * z;
                result = result +
                    SH_C2[0] * xy * shMat.sh[4] +
                    SH_C2[1] * yz * shMat.sh[5] +
                         SH_C2[2] * (2.0f * zz - xx - yy) * shMat.sh[6] +
                    SH_C2[3] * xz * shMat.sh[7] +
                         SH_C2[4] * (xx - yy) * shMat.sh[8];

                if (shMat.degree > 2)
                {
                    result = result +
                        SH_C3[0] * y * (3.0f * xx - yy) * shMat.sh[9] +
                        SH_C3[1] * xy * z * shMat.sh[10] +
                             SH_C3[2] * y * (4.0f * zz - xx - yy) * shMat.sh[11] +
                             SH_C3[3] * z * (2.0f * zz - 3.0f * xx - 3.0f * yy) * shMat.sh[12] +
                             SH_C3[4] * x * (4.0f * zz - xx - yy) * shMat.sh[13] +
                             SH_C3[5] * z * (xx - yy) * shMat.sh[14] +
                             SH_C3[6] * x * (xx - 3.0f * yy) * shMat.sh[15];
                }
            }
        }
        result += 0.5f;
        result = max(result, 0.f);
        attenuation = float4(result, 1);
    }
    else if (is_transmissive)
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
        // I dont think this check should be necessary
        float3 refracted = refract(incident, N, ref_ratio);
        if (length(refracted) < EPSILON)
        {
            scattered = reflect(incident, N);
        }
        else
        {
            scattered = refracted;
        }
    }
    else
    {
        scattered = sample_lamberian(N);
    }

    Ray outputRay = Ray(point_on_ray(input_ray, hit.t), scattered, input_ray.scatter * attenuation, input_ray.emission + material.emissiveColor);

    rays[rayIndex] = outputRay;
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

float4 sky_color(Ray ray, float4 backgroundColor)
{
    float t = 0.5 * (normalize(ray.direction).y + 1.0);
    return backgroundColor; //(1 - t) * float4(1) + t * backgroundColor;
}

Intersection intersect_sphere(const Ray ray, Sphere sphere)
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

Intersection intersect_triangle(const Ray ray, Triangle tri)
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

Intersection intersect_splat(const Ray ray, GaussianSplat gs, float closest_t)
{
    float3 w = ray.direction;
    float3 X = ray.origin - gs.center;

    float3x3 Rt = transpose(gs.rotation);

    float3 Xl = mul(Rt, X);
    float3 wl = mul(Rt, w);
    float3 wl2 = wl * wl;
    float3 invScale2 = gs.invScale2;
    float A = dot(wl2, invScale2);
    float B = dot(Xl * wl, invScale2);

    float mean_t = -B / A;

    // reject if past the closest collision or if behind the ray
    if (mean_t > closest_t || mean_t < EPSILON)
    {
        return no_intersection();
    }

    float3 dl = Xl + mean_t * wl;

    float sample = dot(dl * dl, invScale2);
    if (sample > SPLAT_CUTOFF) return no_intersection();

    float alpha = gs.alpha * exp(-0.5 * sample);
    float roulette = rand_float();

    if (roulette >= alpha) return no_intersection();

    float3 localGradient = dl * invScale2;
    float3 normal = normalize(mul(gs.rotation, localGradient));

    return Intersection(normal, mean_t, gs.materialIndex);
}

float2 aabb_bounds(Ray ray, float3 aabb_min, float3 aabb_max)
{
    float3 invDir = 1.f / ray.direction;
    float3 t0 = (aabb_min - ray.origin) * invDir;
    float3 t1 = (aabb_max - ray.origin) * invDir;
    float3 tMin = min(t0, t1);
    float3 tMax = max(t0, t1);

    float tNear = max(max(tMin.x, tMin.y), tMin.z);
    float tFar = min(min(tMax.x, tMax.y), tMax.z);

    float tHit = max(tNear, EPSILON);

    bool valid = tFar >= tHit;

    return valid ? float2(tHit, tFar) : float2(FLT_MAX);
}

float intersect_aabb(Ray ray, float3 min, float3 max)
{
    float2 bounds = aabb_bounds(ray, min, max);
    return bounds.x;
}

