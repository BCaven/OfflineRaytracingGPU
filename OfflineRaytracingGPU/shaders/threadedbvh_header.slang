implementing scene;

struct tbvhNode
{
    float3 min;
    float3 max;
    uint packedLeft;
    uint packedRight;
    uint packedMiss;
}