implementing scene;

// short stack structs

public static const uint LAST_CHILD = 0xFFFFFFFFu;

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
    uint parentLevel;
}