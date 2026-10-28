implementing scene;

__include threadedbvh_header;

public StructuredBuffer<tbvhNode> threadedNodes;
public StructuredBuffer<kdopThreaded> threadedKDopNodes;

public StructuredBuffer<float4x4> transformMats;

/*

TODO: do a performance benchmark comparing a stackless traversal to full stack DFS and shortstack DFS

Appealing since it would also remove the transform stack and allow for arbitrarily long transform chains

*/

float2 kdop_bounds(Ray ray, float kdopmin[7], float kdopmax[7])
{
    float tEntry = -FLT_MAX;
    float tExit = FLT_MAX;
    [unroll]
    for (int i = 0; i < 7; i++)
    {
        float denom = dot(ray.direction, KDOP_DIRECTIONS[i]);
        float o = dot(ray.origin, KDOP_DIRECTIONS[i]);
        if (abs(denom) < EPSILON)
        {
            if (o < kdopmin[i] || o > kdopmax[i])
            {
                return float2(FLT_MAX);
            }
            continue;
        }
        float tA = (kdopmin[i] - o) / denom;
        float tB = (kdopmax[i] - o) / denom;
        tEntry = max(tEntry, min(tA, tB));
        tExit = min(tExit, max(tA, tB));
        if (tEntry > tExit || tExit < EPSILON) return float2(FLT_MAX);
    }

    float tHit = max(EPSILON, tEntry);

    return float2(tHit, tExit);
}


bool transformRay(inout Ray ray, float4x4 invTransform, inout float tScale)
{
    //
    float3 localOrigin = mul(invTransform, float4(ray.origin, 1)).xyz;
    float3 localDirection = mul((float3x3)invTransform, ray.direction);

    float lenDir = length(localDirection);

    if (lenDir < EPSILON)
    {
        // bad transform
        ray = Ray();
        tScale = 1;
        return false;
    }

    float3 dir = localDirection / lenDir;
    ray.origin = localOrigin;
    ray.direction = dir;
    tScale = tScale / lenDir;
    return true;
}

bool isThreaded(PrimType t)
{
    return t == PrimType::THREADED_NODE || t == PrimType::THREADED_KDOP;
}

Intersection intersect_scene_threaded(Ray startRay, uint rootIndex, PrimType rootType, Intersection startClosest, float scaleT, float3x3 sceneTransform)
{
    Intersection closest = startClosest;

    uint index = rootIndex;
    PrimType nodeType = rootType;

    Ray currentRay = startRay;
    while (index < INDEX_MAX)
    {
        if (!isThreaded(nodeType))
        {
            // somehow we pursued a non-threaded node
            bool primHit = ProcessPrimitive(nodeType, index, currentRay, closest, sceneTransform, scaleT);
            return closest;
        }
        else if (nodeType == PrimType::THREADED_KDOP)
        {
            // process index collision
            // if it hit and is closest, process the child
            // if it missed, make the index the skip index

            kdopThreaded current = threadedKDopNodes[index];
            uint nextIdx = unpackIndex(current.packedMiss);
            PrimType nextType = unpackType(current.packedMiss);

            float2 rawHit = kdop_bounds(currentRay, current.min, current.max);
   
            bool hit = rawHit.x != FLT_MAX && scaleT * rawHit.x < closest.t;

            if (hit) // hit and closest
            {
                // update closest
                //closest.t = scaleT * rawHit.x;
                // process children
                uint childIdx = unpackIndex(current.packedChild);
                PrimType childType = unpackType(current.packedChild);

                if (isThreaded(childType))
                {
                    nextIdx = childIdx; nextType = childType;
                }
                else
                {
                    Ray r = currentRay; float s = scaleT;
                    ProcessPrimitive(childType, childIdx, r, closest, sceneTransform, s);

                    // get sibling too
                    uint sibIdx = unpackIndex(current.packedSibling);
                    PrimType sibType = unpackType(current.packedSibling);

                    if (isThreaded(sibType))
                    {
                        // continue into the second child (a subtree)
                        nextIdx = sibIdx; nextType = sibType;
                    }
                    else
                    {
                        // second child is also a primitive: process it, then fall through to miss
                        Ray r = currentRay; float s = scaleT;
                        ProcessPrimitive(sibType, sibIdx, r, closest, sceneTransform, s);
                    }
                }
            }
            index = nextIdx;
            nodeType = nextType;
        }
        else
        {
            // process index collision
            // if it hit and is closest, process the child
            // if it missed, make the index the skip index

            tbvhNode current = threadedNodes[index];

            uint nextIdx = unpackIndex(current.packedMiss);
            PrimType nextType = unpackType(current.packedMiss);

            float hit = scaleT * intersect_aabb(currentRay, current.min, current.max);
            bool bHit = false;
            if (hit < closest.t) // hit and closest
            {
                uint childIdx = unpackIndex(current.packedChild);
                PrimType childType = unpackType(current.packedChild);

                if (isThreaded(childType))
                {
                    nextIdx = childIdx; nextType = childType;
                }
                else
                {
                    Ray r = currentRay; float s = scaleT;
                    ProcessPrimitive(childType, childIdx, r, closest, sceneTransform, s);

                    // get sibling too
                    uint sibIdx = unpackIndex(current.packedSibling);
                    PrimType sibType = unpackType(current.packedSibling);

                    if (isThreaded(sibType))
                    {
                        // continue into the second child (a subtree)
                        nextIdx = sibIdx; nextType = sibType;
                    }
                    else
                    {
                        // second child is also a primitive: process it, then fall through to miss
                        Ray r = currentRay; float s = scaleT;
                        ProcessPrimitive(sibType, sibIdx, r, closest, sceneTransform, s);
                    }
                }
            }
            index = nextIdx;
            nodeType = nextType;
        }
        
    }
    return closest;	
}