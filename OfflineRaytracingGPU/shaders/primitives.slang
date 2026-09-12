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
public RWStructuredBuffer<Ray> rayQueueA;
public RWStructuredBuffer<Ray> rayQueueB;
public RWStructuredBuffer<uint> counters;


public Ray makeRay(uint2 rayCoord, float3 origin, float3 direction)
{
    // ignore counter index since we are ignoring previously written rays
    // TODO: non-uniform samping
    Ray newRay = Ray(origin, direction, 1, 0, 380.f + (rand_float() * (730.f - 380.f)), rayCoord);
    return newRay;
}
public bool getRay(int rayIndex, int frameIndex, out Ray ray)
{
    int counterIndex = frameIndex % 2 == 0 ? 0 : 1; // readCounter
    if (counters[counterIndex] <= rayIndex)
    {
        ray = Ray();
        return false;
    }
    if (counterIndex == 0)
    {
        ray = rayQueueA[rayIndex];
    }
    else
    {
        ray = rayQueueB[rayIndex];
    }
    
    return true;
}

void pushRayQueue(Ray ray, int frameIndex)
{
    int counterIndex = frameIndex % 2 == 0 ? 1 : 0; // writeCounter
    uint count = 0;
    InterlockedAdd(counters[counterIndex], 1, count);
    if (count < MAX_RAY_QUEUE)
    {
        if (counterIndex == 0)
        {
            rayQueueA[count] = ray;
        }
        else
        {
            rayQueueB[count] = ray;
        }
    }
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

float rgb_to_spectral(float4 rgb, float lambda)
{
    float rResp = gaussian_log(lambda, 611.0, 0.05);
    float gResp = gaussian_log(lambda, 549.0, 0.06);
    float bResp = gaussian_log(lambda, 465.0, 0.05);

    float numerator = rgb.r * rResp + rgb.g * gResp + rgb.b * bResp;
    float denominator = rResp + gResp + bResp;
    return numerator / max(denominator, 1e-5);
}

float cauchy_ior(float n_d, float abbeNumber, float lambda_nm)
{
    static const float lambda_d = 587.6, lambda_F = 486.1, lambda_C = 656.3;
    float invSqD = 1.0 / (lambda_d * lambda_d);
    float invSqF = 1.0 / (lambda_F * lambda_F);
    float invSqC = 1.0 / (lambda_C * lambda_C);

    float B = (n_d - 1.0) / (abbeNumber * (invSqF - invSqC));
    float A = n_d - B * invSqD;

    float invSqL = 1.0 / (lambda_nm * lambda_nm);
    return A + B * invSqL;
}

float2 scatter_spectral(Ray input_ray, Intersection hit, Material material, float3 camDir, int frameIndex)
{
    // TODO: this function needs to push rays to writeQueue
    // probably easiest to return scatter and emission spectral so they can be used, since the input ray is dead

    // TODO: microfacet BRDF
    // get wavelength from ray

    float3 incident = normalize(input_ray.direction);
    float incident_dot_normal = dot(incident, hit.normal);
    bool is_front_face = incident_dot_normal < 0;
    float3 N = select(is_front_face, hit.normal, -hit.normal);
    float cos_theta = abs(incident_dot_normal);
    bool useSH = material.SHIndex >= 0;
    bool is_transmissive = material.metallicOrIor < 0;

    float lambda = input_ray.wavelength;

    float ref_ratio = 1.f;
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
    float spectral_albedo = rgb_to_spectral(attenuation, lambda);
    float attenuation_scalar = spectral_albedo;
    if (is_transmissive)
    {

        float ior = cauchy_ior(abs(material.metallicOrIor), material.abbe, lambda);
        ref_ratio = select(is_front_face, 1 / ior, ior);
        bool cannot_refract = ref_ratio * ref_ratio * (1.0 - cos_theta * cos_theta) > 1;
        float f0 = schlick_f0_from_ior(ref_ratio);
        choose_specular = cannot_refract || schlick_fresnel(f0, u5) > rand_float();
        attenuation_scalar = choose_specular ? 1.f : spectral_albedo;
    }
    else if (material.metallicOrIor > 0 && !useSH)
    {
        float metallic = material.metallicOrIor;
        float f0 = lerp(GLASS_F0_SCALAR, spectral_albedo, metallic);
        float F = schlick_fresnel(f0, u5);

        float specular = F;
        float diffuse = (spectral_albedo - GLASS_F0_SCALAR) * (1 - metallic) * (1 - u5) / 0.5;

        float S = specular;
        float D = diffuse;
        float specular_pdf = S / (S + D);

        choose_specular = specular_pdf > rand_float();

        if (choose_specular)
        {
            attenuation_scalar = specular / specular_pdf;
        }
        else
        {
            attenuation_scalar = diffuse / (1 - specular_pdf);
        }
    }


    float3 scattered = float3(0);
    if (choose_specular)
    {
        scattered = reflect(incident, N);
    }
    else if (is_transmissive)
    {
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

    Ray outputRay;
    outputRay.origin = point_on_ray(input_ray, hit.t);
    outputRay.direction = scattered;
    outputRay.scatter = input_ray.scatter * attenuation_scalar;
    outputRay.emission = input_ray.emission + input_ray.scatter * rgb_to_spectral(material.emissiveColor, lambda);
    outputRay.wavelength = input_ray.wavelength; // TODO change this to include hero sampling or something:)
    outputRay.index = input_ray.index; // they point to the same pixel
    // TODO: also need to have rays include their pdf or something
    pushRayQueue(outputRay, frameIndex);

    return float2(outputRay.scatter, outputRay.emission);
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

float2 kdop_bounds_diag(Ray ray, float4 diagMin, float4 diagMax, float tNear, float tFar)
{
    float tEntry = tNear;
    float tExit = tFar;
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float denom = dot(ray.direction, KDOP_DIRECTIONS[i + 3]);
        float o = dot(ray.origin, KDOP_DIRECTIONS[i + 3]);
        if (abs(denom) < EPSILON)
        {
            if (o < diagMin[i] || o > diagMax[i])
            {
                return float2(FLT_MAX);
            }
            continue;
        }
        float tA = (diagMin[i] - o) / denom;
        float tB = (diagMax[i] - o) / denom;
        tEntry = max(tEntry, min(tA, tB));
        tExit = min(tExit, max(tA, tB));
        if (tEntry > tExit || tExit < EPSILON) return float2(FLT_MAX);
    }

    float tHit = max(EPSILON, tEntry);

    return float2(tHit, tExit);
}

uint intersectKDopChildren(
    uint nodeIndex,
    KDopNodeHot hot,
    float3 rayOrigin,
    float3 rayDirection,
    float tMax,
                           float tScale,
    out float tHit[KDOP_WIDTH])
{
    Ray localRay;
    localRay.origin = rayOrigin;
    localRay.direction = rayDirection;

    // phase 1: cheap 3-axis AABB test across all slots
    uint aabbMask = 0u;
    float2 aabbBounds[KDOP_WIDTH];

    [unroll]
    for (uint slot = 0; slot < KDOP_WIDTH; ++slot)
    {
        tHit[slot] = FLT_MAX;

        PrimType childType = unpackType(bit_cast<uint>(hot.min_packed[slot].w));
        if (childType == PrimType::EMPTY)
            continue;

        float2 b = aabb_bounds(localRay, hot.min_packed[slot].xyz, hot.max[slot].xyz);
        aabbBounds[slot] = b;

        if (b.x != FLT_MAX && (tScale * b.x) < tMax)
            aabbMask |= (1u << slot);
    }

    if (aabbMask == 0u)
        return 0u; // nothing survived phase 1 -- skip the cold fetch entirely

    // phase 2: only now pull the diagonal-plane data, and only refine
    // the slots that actually need it
    K14DopNodeCold cold = kdopColdNodes[nodeIndex];

    uint validMask = 0u;

    [unroll]
    for (uint slot = 0; slot < KDOP_WIDTH; ++slot)
    {
        if ((aabbMask & (1u << slot)) == 0u)
            continue;

        float2 ab = aabbBounds[slot];
        float2 refined = kdop_bounds_diag(localRay, cold.min[slot], cold.max[slot], ab.x, ab.y);

        if (refined.x == FLT_MAX)
            continue; // diagonal planes rejected it

        float t = tScale * refined.x;
        if (t < tMax)
        {
            tHit[slot] = t;
            validMask |= (1u << slot);
        }
    }

    return validMask;
}

uint sortChildrenByDistance(uint validMask, float tHit[KDOP_WIDTH], out uint sortedSlots[KDOP_WIDTH])
{
    uint count = 0;

    [unroll]
    for (uint slot = 0; slot < KDOP_WIDTH; ++slot)
    {
        if ((bool) (validMask & (1u << slot)))
        {
            sortedSlots[count] = slot;
            count++;
        }
    }

    for (uint i = 1; i < count; ++i)
    {
        uint key = sortedSlots[i];
        float keyT = tHit[key];
        int j = (int)i - 1;
        while (j >= 0 && tHit[sortedSlots[j]] > keyT)
        {
            sortedSlots[j + 1] = sortedSlots[j];
            j--;
        }
        sortedSlots[j + 1] = key;
    }

    return count;
}

void ProcessPrimitive(
    PrimType type,
    int index,
    Ray ray,
    inout Intersection closest,
    TransformStackNode transformNode)
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
    case PrimType::GAUSSIAN_SPLAT:
        hit = intersect_splat(ray, splats[index], closest.t / transformNode.tScale);
        break;
    default:
        return;
    }
    if (hit.t > 0 && hit.t * transformNode.tScale < closest.t)
    {
        // TODO: rebuild the normal transformation matrix
        float3x3 normalMatrix = identity3x3();

        for (uint i = 0; i < transformNode.transformDepth; i++)
        {
            float3x3 transformNormal = transpose((float3x3)transforms[transformNode.transformChain[i]].invMatrix);
            normalMatrix = mul(normalMatrix, transformNormal);
        }

        float3 newNormal = normalize(mul(normalMatrix, hit.normal));
        hit.normal = newNormal;
        hit.t *= transformNode.tScale;
        closest = hit;
    }
}