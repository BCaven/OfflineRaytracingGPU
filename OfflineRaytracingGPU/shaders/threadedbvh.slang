implementing scene;

__include threadedbvh_header;

public StructuredBuffer<tbvhNode> threadedNodes;

public StructuredBuffer<float4x4> transformMats;

/*

TODO: do a performance benchmark comparing a stackless traversal to full stack DFS and shortstack DFS

Appealing since it would also remove the transform stack and allow for arbitrarily long transform chains

*/

bool transformRay(inout Ray ray, float4x4 invTransform, out float tScale)
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
    tScale = 1 / lenDir;
    return true;
}


Intersection intersect_scene_threaded(Ray startRay, uint rootIndex, PrimType rootType, uint threadLen)
{
    /*
    Each transform in the StructuredBuffer is world->currentSpace

    Problem: support for collections of instances means we need to handle transforms by multiplying the current state by
    a matrix that undos the transform we just left

    
        
    */
    Intersection closest = no_intersection();
    closest.t = FLT_MAX;

    uint index = rootIndex;
    PrimType nodeType = rootType;

    Ray currentRay = startRay;
    float4x4 currentTransform = identity4x4();
    float scaleT = 1;
    uint lastPackedMiss = -1;
    // otherwise traverse the threaded nodes
    bool quit = index >= threadLen;
    while (index < threadLen)
    {
        if (nodeType != PrimType::THREADED_NODE)
        {
            // somehow we pursued a non-threaded node
            return closest;
        }

        // process index collision
        // if it hit, process the child
        // if it missed, make the index the skip index

        tbvhNode current = threadedNodes[index];
        float hit = scaleT * intersect_aabb(currentRay, current.min, current.max);
        bool bHit = false;
        if (hit < closest.t) // hit and closest
        {
            bHit = true;
            if (current.hitTransform >= 0)
            {
                // load the hit transform and apply it
                currentTransform = transformMats[current.hitTransform];
                transformRay(currentRay, currentTransform, scaleT);
            }
            // update current index/type and continue
            index = unpackIndex(current.packedChild);
            nodeType = unpackType(current.packedChild);
            lastPackedMiss = current.packedMiss;

            // it might be easier to just check for primitives here
            if (nodeType != PrimType::THREADED_NODE)
            {
                // process collision and exit
                float3x3 normalMat = transpose((float3x3)currentTransform);
                bHit = ProcessPrimitive(nodeType, index, currentRay, closest, normalMat, scaleT);
            }

        }
        if (!bHit) // hit and not closest or missed
        {
            // modify current ray if needed and go to miss node
            if (current.missTransform >= 0)
            {
                // need to actually load that transform
                currentTransform = transformMats[current.missTransform];
                float4x4 invT = inverseAffine(currentTransform);
                transformRay(currentRay, invT, scaleT);
            }
            index = unpackIndex(current.packedMiss);
            nodeType = unpackType(current.packedMiss);        
        }
    }
    return closest;	
}