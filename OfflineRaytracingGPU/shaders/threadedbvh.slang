implementing scene;

__include threadedbvh_header;

public StructuredBuffer<tbvhNode> threadedNodes;

public StructuredBuffer<float4x4> transformMats;

/*

TODO: do a performance benchmark comparing a stackless traversal to full stack DFS and shortstack DFS

Appealing since it would also remove the transform stack and allow for arbitrarily long transform chains

*/

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

Intersection intersect_scene_threaded(Ray startRay, uint rootIndex, PrimType rootType)
{
    Intersection closest = no_intersection();
    closest.t = FLT_MAX;

    uint index = rootIndex;
    PrimType nodeType = rootType;

    Ray currentRay = startRay;
    float4x4 currentTransform = identity4x4();
    float4x4 accumulatedTransform = identity4x4();
    float scaleT = 1;
    while (index < INDEX_MAX)
    {
        if (nodeType != PrimType::THREADED_NODE)
        {
            // somehow we pursued a non-threaded node
            float3x3 normalMat = transpose((float3x3)accumulatedTransform);
            bool primHit = ProcessPrimitive(nodeType, index, currentRay, closest, normalMat, scaleT);
            return closest;
        }

        // process index collision
        // if it hit and is closest, process the child
        // if it missed, make the index the skip index

        tbvhNode current = threadedNodes[index];
        float hit = scaleT * intersect_aabb(currentRay, current.min, current.max);
        bool bHit = false;
        if (hit < closest.t) // hit and closest
        {
            uint childIdx = unpackIndex(current.packedChild);
            PrimType childType = unpackType(current.packedChild);
            bHit = true;

            if (childType != PrimType::THREADED_NODE)
            {
                // temporary frame: live ray/accumulated stay in this node's box space
                Ray r = currentRay; float s = scaleT; float4x4 acc = accumulatedTransform;
                if (current.hitTransform >= 0)
                {
                    float4x4 T = transformMats[current.hitTransform];
                    transformRay(r, T, s);
                    acc = mul(T, acc);
                }
                ProcessPrimitive(childType, childIdx, r, closest, transpose((float3x3)acc), s);
                bHit = false; // treat as end of chain -> miss edge
            }
            else
            {
                if (current.hitTransform >= 0)
                {
                    currentTransform = transformMats[current.hitTransform];
                    accumulatedTransform = mul(currentTransform, accumulatedTransform);
                    transformRay(currentRay, currentTransform, scaleT);
                }
                index = childIdx; nodeType = childType;
            }
        }
        if (!bHit) // hit and not closest or missed
        {
            // modify current ray if needed and go to miss node
            if (current.missTransform >= 0)
            {
                // need to actually load that transform
                currentTransform = transformMats[current.missTransform];
                accumulatedTransform = mul(currentTransform, accumulatedTransform);
                transformRay(currentRay, currentTransform, scaleT);
            }
            index = unpackIndex(current.packedMiss);
            nodeType = unpackType(current.packedMiss);        
        }
    }
    return closest;	
}