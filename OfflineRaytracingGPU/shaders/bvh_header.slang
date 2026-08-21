implementing scene;

// NOTE: this should be as small as possible :)
static const uint MAX_TRANSFORM_DEPTH = 16;
struct TraversalNode
{
    uint packedIndexType;
    float tEntry;
    uint transformStackIndex;
}

struct TransformStackNode
{
    float tScale;
    float3 rayOrigin;
    float3 rayDirection;
    uint transformDepth = 0;
    uint transformChain[MAX_TRANSFORM_DEPTH];
    uint nodesUsingThis = 0;
}

