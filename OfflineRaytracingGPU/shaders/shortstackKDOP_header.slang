implementing scene;

// short stack structs

struct RestartTrailEntry
{
    uint nodeIndex;
    uint childMask; // nodes that need to be explored
}

struct ShortStackNode
{
    uint packedIndexType;
    float tEntry;
    uint transformStackIndex;
    uint parentSlot;
}