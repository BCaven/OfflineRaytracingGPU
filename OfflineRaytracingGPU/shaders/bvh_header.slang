implementing scene;



public struct TraversalNode
{
    public uint packedIndexType;
    public float tEntry;
}

static const TraversalNode DEAD_NODE = { INVALID_NODE, 0.f };

groupshared TraversalNode rayStack[GROUP_SIZE * STACK_SIZE];
//groupshared TransformStackNode transformStack[GROUP_SIZE * TRANSFORM_STACK_SIZE];
groupshared CompactTransform transformStack[GROUP_SIZE * TRANSFORM_STACK_SIZE];

public struct TraversalStack
{
    public uint lane;
    public uint pt;

    [mutating] bool push(TraversalNode node) 
    {
        if (pt < STACK_SIZE)
        {
            rayStack[pt++ * GROUP_SIZE + lane] = node;
            return true;
        }
        return false;
    }
    [mutating] TraversalNode pop() 
    {
        if (pt > 0)
        {
            return rayStack[--pt * GROUP_SIZE + lane];
        }
        else
        {
            // empty!
            return DEAD_NODE;
        }
    }
    bool empty() { return pt == 0; }
}

public struct TransformStackNode
{
    public float tScale;
    public float3 rayOrigin;
    public float3 rayDirection;
    uint transformDepth = 0;
    uint transformChain[MAX_TRANSFORM_DEPTH];
    uint nodesUsingThis = 0;
}
/*
static const TransformStackNode DEAD_TRANSFORM = { 1, 0.f, { 1.f, 0, 0 }};

public struct TransformStack
{
    public uint lane;
    public uint pt;

    [mutating] bool push(TransformStackNode node)
    {
        if (pt < TRANSFORM_STACK_SIZE)
        {
            transformStack[pt++ * GROUP_SIZE + lane] = node;
            return true;
        }
        return false;
    }
    [mutating] TransformStackNode pop()
    {
        if (pt > 0)
        {
            return transformStack[--pt * GROUP_SIZE + lane];
        }
        else
        {
            // empty!
            return DEAD_TRANSFORM;
        }
    }

    [mutating] void releaseTransformEntry(uint idx)
    {
        transformStack[idx * GROUP_SIZE + lane].nodesUsingThis--;
        while (pt > 0 && transformStack[pt - 1].nodesUsingThis == 0)
        {
            pt--;
        }
    }

    [mutating] TransformStackNode get(uint idx)
    {
        return transformStack[idx * GROUP_SIZE + lane];
    }

    [mutating] void increaseNodesUsing(uint idx, int amount)
    {
        transformStack[idx * GROUP_SIZE + lane].nodesUsingThis += amount;
    }

    bool empty() { return pt == 0; }
}
*/
public struct CompactTransform
{
    public uint transformIdx;
    public uint stackStartIdx;
}

public struct CompactTransformStack
{
    public uint lane;
    public uint pt;
    public float3x3 cumulativeTransform = identity3x3();

    [mutating] bool push(uint transformPtr, uint traversalStackIndex, inout Ray currentRay, inout float tScale)
    {
        if (pt < TRANSFORM_STACK_SIZE)
        {
            Transform t = transforms[transformPtr];
            float3 localOrigin = mul(t.invMatrix, float4(currentRay.origin, 1.f)).xyz;
            float3 localDirection = mul((float3x3)t.invMatrix, currentRay.direction);
            float directionLength = length(localDirection);
            if (directionLength < EPSILON)
            {
                return false; // transform invalid, not applied
            }
            // apply transform to cumulative
            cumulativeTransform = mul((float3x3)t.invMatrix, cumulativeTransform);

            // apply transform to current ray
            currentRay.origin = localOrigin;
            currentRay.direction = localDirection / directionLength;
            tScale = tScale / directionLength;

            CompactTransform node = { transformPtr, traversalStackIndex };
            transformStack[pt++ * GROUP_SIZE + lane] = node;
            return true;
        }
        return false;
    }

    // 
    [mutating] CompactTransform pop()
    {
        if (pt > 0)
        {
            return transformStack[--pt * GROUP_SIZE + lane];
        }
        else
        {
            // empty!
            return { 0, 0 };
        }
    }

    CompactTransform peekTop()
    {
        if (pt > 0)
        {
            return transformStack[(pt - 1) * GROUP_SIZE + lane];
        }
        else
        {
            // empty!
            return { 0, 0 };
        }
    }

    [mutating] bool apply(uint transformPtr, inout Ray ray, inout float tScale)
    {
        Transform t = transforms[transformPtr];
        float3 o = mul(t.invMatrix, float4(ray.origin, 1.f)).xyz;
        float3 d = mul((float3x3)t.invMatrix, ray.direction);
        float len = length(d);
        if (len < EPSILON) return false;
        cumulativeTransform = mul((float3x3)t.invMatrix, cumulativeTransform);
        ray.origin = o; ray.direction = d / len; tScale /= len;
        return true;
    }

    // maybe this should be something the traversal algorithm handles.
    [mutating] void checkTransform(uint traversalStackIndex, Ray worldRay, inout Ray currentRay, inout float tScale)
    {
        bool needsReconstruction = false;
        while (pt > 0 && traversalStackIndex < peekTop().stackStartIdx)
        {
            pt--;
            needsReconstruction = true;
        }

        if (!needsReconstruction) return;

        currentRay = worldRay;
        tScale = 1;
        cumulativeTransform = identity3x3();
        for (uint i = 0; i < pt; i++)
        {
            apply(transformStack[i * GROUP_SIZE + lane].transformIdx, currentRay, tScale);
        }
    }

    bool empty() { return pt == 0; }
}