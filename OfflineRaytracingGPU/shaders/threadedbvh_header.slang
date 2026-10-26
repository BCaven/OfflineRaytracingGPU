implementing scene;


public struct tbvhNode
{
    float3 min;
    float3 max;
    uint packedChild;
    uint packedMiss;
    uint packedSibling;
}

public struct kdopThreaded
{
    float min[7];
    float max[7];
    uint packedChild;
    uint packedMiss;
    uint packedSibling;
};