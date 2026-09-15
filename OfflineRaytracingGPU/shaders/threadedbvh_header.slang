implementing scene;

public struct tbvhNode
{
    float3 min;
    float3 max;
    uint packedChild;
    uint packedMiss;
    int missTransform;
    int hitTransform;
}