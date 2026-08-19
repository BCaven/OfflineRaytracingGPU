implementing scene;

/*
TODO: look at ways to reduce this memory footprint (especially frequent access items)

Most of the time only index and t entry change

Everything else could be stored in a different buffer that tracks transforms


*/
// NOTE: this should be as small as possible :)
static const uint MAX_TRANSFORM_DEPTH = 4;
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

