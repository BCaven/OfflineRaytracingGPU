implementing scene;

// NOTE: this should be as small as possible :)
static const uint MAX_TRANSFORM_DEPTH = 4;

public struct TraversalNode
{
    public uint packedIndexType;
    public float tEntry;
    public uint transformStackIndex;
}

static const TraversalNode DEAD_NODE = { INVALID_NODE, 0.f, 0 };

groupshared TraversalNode rayStack[GROUP_SIZE * STACK_SIZE];
groupshared TransformStackNode transformStack[GROUP_SIZE * TRANSFORM_STACK_SIZE];

public struct TraversalStack
{
    public uint lane;
    public uint pt;

    [mutating] void push(TraversalNode node) 
    {
        if (pt < STACK_SIZE)
        {
            rayStack[pt++ * GROUP_SIZE + lane] = node;
        }
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

static const TransformStackNode DEAD_TRANSFORM = { 1, 0.f, { 1.f, 0, 0 }};

public struct TransformStack
{
    public uint lane;
    public uint pt;

    [mutating] void push(TransformStackNode node)
    {
        if (pt < TRANSFORM_STACK_SIZE)
        {
            transformStack[pt++ * GROUP_SIZE + lane] = node;
        }
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