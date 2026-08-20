implementing scene;

__include shortstackKDOP_header;

float2 kdop_bounds_diag(Ray ray, float4 diagMin, float4 diagMax, float tNear, float tFar)
{
    float tEntry = tNear;
    float tExit = tFar;
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float denom = dot(ray.direction, KDOP_DIRECTIONS[i + 3]);
        float o = dot(ray.origin, KDOP_DIRECTIONS[i + 3]);
        if (abs(denom) < EPSILON)
        {
            if (o < diagMin[i] || o > diagMax[i])
            {
                return float2(FLT_MAX);
            }
            continue;
        }
        float tA = (diagMin[i] - o) / denom;
        float tB = (diagMax[i] - o) / denom;
        tEntry = max(tEntry, min(tA, tB));
        tExit = min(tExit, max(tA, tB));
        if (tEntry > tExit || tExit < EPSILON) return float2(FLT_MAX);
    }

    float tHit = max(EPSILON, tEntry);

    return float2(tHit, tExit);
}

uint intersectKDopChildren(
    uint nodeIndex,
    KDopNodeHot hot,
    float3 rayOrigin,
    float3 rayDirection,
    float tMax,
    float tScale,
    out float tHit[KDOP_WIDTH])
{
    Ray localRay;
    localRay.origin = rayOrigin;
    localRay.direction = rayDirection;

    // phase 1: cheap 3-axis AABB test across all slots
    uint aabbMask = 0u;
    float2 aabbBounds[KDOP_WIDTH];

    [unroll]
    for (uint slot = 0; slot < KDOP_WIDTH; ++slot)
    {
        tHit[slot] = FLT_MAX;

        PrimType childType = unpackType(bit_cast<uint>(hot.min_packed[slot].w));
        if (childType == PrimType::EMPTY)
            continue;

        float2 b = aabb_bounds(localRay, hot.min_packed[slot].xyz, hot.max[slot].xyz);
        aabbBounds[slot] = b;

        if (b.x != FLT_MAX && (tScale * b.x) < tMax)
            aabbMask |= (1u << slot);
    }

    if (aabbMask == 0u)
        return 0u; // nothing survived phase 1 -- skip the cold fetch entirely

    // phase 2: only now pull the diagonal-plane data, and only refine
    // the slots that actually need it
    K14DopNodeCold cold = kdopColdNodes[nodeIndex];

    uint validMask = 0u;

    [unroll]
    for (uint slot = 0; slot < KDOP_WIDTH; ++slot)
    {
        if ((aabbMask & (1u << slot)) == 0u)
            continue;

        float2 ab = aabbBounds[slot];
        float2 refined = kdop_bounds_diag(localRay, cold.min[slot], cold.max[slot], ab.x, ab.y);

        if (refined.x == FLT_MAX)
            continue; // diagonal planes rejected it

        float t = tScale * refined.x;
        if (t < tMax)
        {
            tHit[slot] = t;
            validMask |= (1u << slot);
        }
    }

    return validMask;
}

uint sortChildrenByDistance(uint validMask, float tHit[KDOP_WIDTH], out uint sortedSlots[KDOP_WIDTH])
{
    uint count = 0;

    [unroll]
    for (uint slot = 0; slot < KDOP_WIDTH; ++slot)
    {
        if (validMask & (1u << slot))
        {
            sortedSlots[count] = slot;
            count++;
        }
    }

    for (uint i = 1; i < count; ++i)
    {
        uint key = sortedSlots[i];
        float keyT = tHit[key];
        int j = (int)i - 1;
        while (j >= 0 && tHit[sortedSlots[j]] > keyT)
        {
            sortedSlots[j + 1] = sortedSlots[j];
            j--;
        }
        sortedSlots[j + 1] = key;
    }

    return count;
}

bool transformRayShortStack(ShortStackNode node, TransformStackNode transformNode, uint transformIdx, Transform t, out ShortStackNode result, out TransformStackNode resultTransformStack)
{
    float3 localOrigin = mul(t.invMatrix, float4(transformNode.rayOrigin, 1.f)).xyz;
    float3 localDirection = mul((float3x3)t.invMatrix, transformNode.rayDirection);

    float directionLength = length(localDirection);
    if (directionLength < EPSILON)
    {
        // bad transform, this should never happen. Is here for debugging
        result = ShortStackNode();
        resultTransformStack = TransformStackNode();
        return false;
    }
    float3 dir = localDirection / directionLength;
    result.packedIndexType = packChild(t.childPrim, t.childIndex);
    resultTransformStack.rayOrigin = localOrigin;
    resultTransformStack.rayDirection = localDirection / directionLength;
    resultTransformStack.tScale = transformNode.tScale / directionLength;
    result.tEntry = node.tEntry;
    result.parentSlot = node.parentSlot;

    resultTransformStack.transformChain = transformNode.transformChain;
    if (transformNode.transformDepth < MAX_TRANSFORM_DEPTH)
    {
        resultTransformStack.transformChain[transformNode.transformDepth] = transformIdx; // since this is the traversal node pointing to a transform
    }
    resultTransformStack.transformDepth = min(transformNode.transformDepth + 1, MAX_TRANSFORM_DEPTH);
    return true;
}

// TODO: this might be unnecessary.
int findNextParentLevel(RestartTrailEntry trail[RESTART_TRAIL_SIZE], uint level)
{
    for (int i = (int)level - 1; i >= 0; --i)
    {
        if (trail[i].childMask != 0u)
        {
            return i;
        }
    }
    return -1;
}

bool popTraversal(
    inout RestartTrailEntry trail[RESTART_TRAIL_SIZE],
    inout uint level,
    inout uint currentSlot,
    inout float currentTEntry,

    inout ShortStackNode stack[SHORT_STACK_SIZE],
    inout uint stackPtr,

    inout uint packedIndexType,

    Ray startRay,
    uint rootIndex,
    PrimType rootType,

    Intersection closest)
{

    if (level == 0)
        return true;

    uint currentParentLevel = level - 1;

    // The current child has now been completely processed.
    trail[currentParentLevel].childMask &= ~(1u << currentSlot);

    // Everything below the parent is stale.
    for (uint i = currentParentLevel + 1; i < RESTART_TRAIL_SIZE; ++i)
    {
        trail[i].nodeIndex = INVALID_NODE;
        trail[i].childMask = 0u;
    }

    while (stackPtr > 0)
    {
        ShortStackNode top = stack[--stackPtr];

        uint parentLevel = top.parentLevel;
        uint parentSlot = top.parentSlot;

        // The stack entry is now becoming the current node.
        //
        // IMPORTANT:
        // Do NOT clear its trail bit here.
        //
        // The bit means "this child still needs to be completed".
        // It remains set while this child is current, and is cleared
        // when this subtree eventually finishes.
        //
        // If the entry is culled, however, it is completely finished,
        // so clear it immediately.
        if (top.tEntry >= closest.t)
        {
            trail[parentLevel].childMask &= ~(1u << parentSlot);

            for (uint i = parentLevel + 1; i < RESTART_TRAIL_SIZE; ++i)
            {
                trail[i].nodeIndex = INVALID_NODE;
                trail[i].childMask = 0u;
            }

            continue;
        }

        // Everything below this parent belongs to the subtree we are
        // about to enter, so it must be reset.
        for (uint i = parentLevel + 1; i < RESTART_TRAIL_SIZE; ++i)
        {
            trail[i].nodeIndex = INVALID_NODE;
            trail[i].childMask = 0u;
        }

        packedIndexType = top.packedIndexType;

        currentSlot = parentSlot;
        currentTEntry = top.tEntry;
        level = parentLevel + 1;

        return false;
    }

    // stack is empty
    packedIndexType = packChild(rootType, rootIndex);
    level = 0;
    currentSlot = 0;
    currentTEntry = 0.f;

    return false;
}

void resolveCandidate(
    KDopNodeHot node, uint slot, float childT, uint parentTransformIdx,
    inout TransformStackNode transformStack[TRANSFORM_STACK_SIZE],
    inout uint transformStackPtr,
    out uint outPackedIndexType,
    out uint outTransformIdx)
{
    uint childI = unpackIndex(bit_cast<uint>(node.min_packed[slot].w));
    PrimType childT_type = unpackType(bit_cast<uint>(node.min_packed[slot].w));

    if (childT_type == PrimType::TRANSFORM)
    {
        Transform t = transforms[childI];
        TransformStackNode parentNode = transformStack[parentTransformIdx];

        ShortStackNode tmp;
        tmp.tEntry = childT;
        ShortStackNode resolved;
        TransformStackNode resolvedStackNode;
        if (transformRayShortStack(tmp, parentNode, childI, t, resolved, resolvedStackNode))
        {
            resolvedStackNode.nodesUsingThis = 0; // caller increments per use
            outTransformIdx = transformStackPtr;
            transformStack[transformStackPtr++] = resolvedStackNode;
            outPackedIndexType = resolved.packedIndexType;
            return;
        }
    }

    outPackedIndexType = packChild(childT_type, childI);
    outTransformIdx = parentTransformIdx;
}

Intersection intersect_scene_short_stack(int rayIndex, uint rootIndex, PrimType rootType)
{
    /*
    while !exit
    if this is an internal node:
        get children's intersection times
        sort them
        if this node is the *last* node in its level:
             Q <- remove all but last entry
        else:
             Q <- remove first k entries
        if Q is empty:
             exit = pop
        else:
            pop first entry from Q
            if len(Q) == 0:
                trail[level] <- N
            else:
                mark last entry in Q as last child
                PushBackToFront(Q)
            level++
    else:
        intersectPrimitive
        exit = pop
    */

    bool exit = false;
    uint iterations = 0;

    ShortStackNode stack[SHORT_STACK_SIZE];
    int stackPtr = 0;

    uint trail[RESTART_TRAIL_SIZE];
    // set trail to 0
    [[unroll]]
    for (int i = 0; i < RESTART_TRAIL_SIZE; i++)
    {
        trail[i] = 0;
    }

    // current starts as root
    ShortStackNode current;
    current.packedIndexType = packChild(rootType, rootIndex);

    Ray currentRay = rays[rayIndex];

    Intersection closest = no_intersection();
    closest.t = FLT_MAX;

    TransformStackNode identity;
    int level = 0;

    while (!exit && iterations < MAX_TRAVERSALS)
    {
        // unpack current node
        PrimType currentType = unpackType(current.packedIndexType);
        int currentIndex = unpackIndex(current.packedIndexType);
        if (currentType == PrimType::KDOP_NODE)
        {
            // 8 wide kDOP
            // k <- trail[level]
            uint k = trail[level];
            KDopNodeHot hot = kdopHotNodes[currentIndex];
            // TODO later: add transforms in once traversal works as expected.
            float3 currentRayDir = currentRay.direction;
            float3 currentRayOrigin = currentRay.origin;
            // H = children that intersect ray
            float[KDOP_WIDTH] tHit;
            // validChildrenMask = 1 for valid child, 0 for invalid. If the entire mask is 0 no children were valid
            uint validChildrenMask = intersectKDopChildren(currentIndex, hot, currentRayOrigin, currentRayDir, current.tEntry, 1, tHit);
            
            // S = H sorted by increasing hit distance
            uint[KDOP_WIDTH] sortedIndexes;
            uint lenSorted = sortChildrenByDistance(validChildrenMask, tHit, sortedIndexes);

            // if k = N:
            //      Q = last entry in S
            // else:
            //      Q = S[k:] <- last k+1 entries in S
            float[KDOP_WIDTH] Q;
            uint lenQ = 0;
            if (k == LAST_CHILD)
            {
                Q[0] = sortedIndexes[lenSorted - 1];
                lenQ = 1;
            }
            else
            {
                for (int i = k + 1; i < lenSorted; i++)
                {
                    int indexQ = i - (k + 1);
                    Q[indexQ] = sortedIndexes[i];
                    lenQ++;
                }
            }
            // if len(Q) == 0:
            //      exit = pop(trail, level)
            // else:
            //      node <- first entry in Q
            //      remove first entry from Q
            //      if len(Q) == 0:
            //          trail[level] <- N
            //      else:
            //          mark last entry in Q as last child
            //          PushBackToFront(Q)
            //      level <- level + 1
            if (lenQ == 0)
            {
                // exit = pop(trail, level)
            }
            else
            {
                // set currrent to the node that is the first entry in Q
                if (lenQ == 1)
                {
                    // that was the only node
                    trail[level] = LAST_CHILD;
                }
                else
                {
                    // mark last entry in Q as last child
                    // also remember that PushBackToFront has to push Q[1:lenQ] since Q[0] has already been set to the current node (i.e. the top of the stack)
                    //PushBackToFront(stack, stackPtr, Q, lenQ, );
                }
                level++;

            }
        }
        else if (currentType == PrimType::BVH_NODE)
        {
            // binary AABB BVH node, to be delt with later
            // do not need to worry about them right now
        }
        else
        {
            // intersected a primitive
            ProcessPrimitive(currentType, currentIndex, currentRay, closest, identity);

            // exit = pop(trail, level)
        }
    }
}

