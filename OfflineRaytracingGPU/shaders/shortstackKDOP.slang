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

    // insertion sort by tHit ascending; count <= KDOP_WIDTH, so worst case
    // is 28 compares -- cheap, and the branch pattern is short/predictable
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
    inout uint transformIdx,
                  inout TransformStackNode transformStack[TRANSFORM_STACK_SIZE],
    inout uint transformStackPtr,
                  Ray startRay, uint rootIndex, PrimType rootType,
    Intersection closest)
{
    while (true)
    {
        int parentLevel = findNextParentLevel(trail, level);
        if (parentLevel < 0)
            return true; // nothing pending anywhere -- fully done

        trail[parentLevel].childMask &= ~(1u << currentSlot);
        for (uint i = parentLevel + 1; i < RESTART_TRAIL_SIZE; ++i)
        {
            trail[i].nodeIndex = INVALID_NODE;
            trail[i].childMask = 0u;
        }

        if (stackPtr == 0)
        {
            TransformStackNode freshIdentity;
            freshIdentity.tScale = 1.f;
            freshIdentity.rayDirection = startRay.direction;
            freshIdentity.rayOrigin = startRay.origin;
            freshIdentity.transformDepth = 0;
            freshIdentity.nodesUsingThis = 1;

            packedIndexType = packChild(rootType, rootIndex);
            transformIdx = 0;
            transformStack[0].nodesUsingThis++;
            level = 0;
            currentSlot = 0;
            currentTEntry = 0.f;
            return false;
        }

        ShortStackNode top = stack[--stackPtr];

        // stack culling: a closer hit may have been found while this sat
        // on the short stack -- if so, it's dead, keep popping
        if (top.tEntry >= closest.t)
        {
            releaseTransformEntry(transformStack, transformStackPtr, top.transformStackIndex);
            level = parentLevel + 1;
            currentSlot = top.parentSlot;
            continue;
        }

        packedIndexType = top.packedIndexType;
        transformIdx = top.transformStackIndex;
        currentSlot = top.parentSlot;
        currentTEntry = top.tEntry;
        level = parentLevel + 1;
        return false;
    }
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
    int iterations = 0;
    Ray startRay = rays[rayIndex];

    Intersection closest = no_intersection();
    closest.t = FLT_MAX;

    ShortStackNode stack[SHORT_STACK_SIZE];
    uint stackPtr = 0;

    RestartTrailEntry trail[RESTART_TRAIL_SIZE];
    for (uint i = 0; i < RESTART_TRAIL_SIZE; ++i)
    {
        trail[i].nodeIndex = INVALID_NODE;
        trail[i].childMask = 0u;
    }

    TransformStackNode transformStack[TRANSFORM_STACK_SIZE];
    uint transformStackPtr = 0;
    TransformStackNode identityTransform;
    identityTransform.tScale = 1.f;
    identityTransform.rayDirection = startRay.direction;
    identityTransform.rayOrigin = startRay.origin;
    identityTransform.transformDepth = 0;
    identityTransform.nodesUsingThis = 1;
    transformStack[transformStackPtr++] = identityTransform;

    uint level = 0;
    uint packedIndexType = packChild(rootType, rootIndex);
    uint transformIdx = 0;
    uint currentSlot = 0;
    float currentTEntry = 0.f;

    bool done = false;
    while (!done && iterations < MAX_TRAVERSALS)
    {
        iterations++;

        if (currentTEntry >= closest.t)
        {
            releaseTransformEntry(transformStack, transformStackPtr, transformIdx);
            done = popTraversal(trail, level, currentSlot, currentTEntry, stack, stackPtr,
                                 packedIndexType, transformIdx, transformStack, transformStackPtr,
                                 startRay, rootIndex, rootType, closest);
            continue;
        }

        uint nodeIndex = unpackIndex(packedIndexType);
        PrimType nodeType = unpackType(packedIndexType);

        TransformStackNode xf = transformStack[transformIdx];

        // TODO: remember to support binary aabb-bvh nodes too :)
        if (nodeType != PrimType::KDOP_NODE)
        {
            // this shouldnt happen
            done = popTraversal(trail, level, currentSlot, currentTEntry, stack, stackPtr,
                                 packedIndexType, transformIdx, transformStack, transformStackPtr,
                                 startRay, rootIndex, rootType, closest);
            continue;
        }

        // KDOP nodes:
        KDopNodeHot node = kdopHotNodes[nodeIndex];

        float tHit[KDOP_WIDTH];
        uint validMask = intersectKDopChildren(nodeIndex, node, xf.rayOrigin, xf.rayDirection, closest.t, xf.tScale, tHit);

        uint sortedSlots[KDOP_WIDTH];
        uint hitCount = sortChildrenByDistance(validMask, tHit, sortedSlots);

        uint candidateMask = 0u;

        for (uint s = 0; s < hitCount; ++s)
        {
            uint slot = sortedSlots[s];
            if (tHit[slot] >= closest.t) continue;

            uint childI = unpackIndex(bit_cast<uint>(node.min_packed[slot].w));
            PrimType childT = unpackType(bit_cast<uint>(node.min_packed[slot].w));

            // this is the resolve primitive check
            if (childT != PrimType::KDOP_NODE && childT != PrimType::TRANSFORM)
            {
                Ray intersectionRay;
                intersectionRay.origin = xf.rayOrigin;
                intersectionRay.direction = xf.rayDirection;

                // TODO: redo this function header to only pass what we need :)
                ProcessPrimitive(childT, childI, intersectionRay, closest, xf);
            }
            else
            {
                candidateMask |= (1u << slot);
            }
        }

        RestartTrailEntry entry = trail[level];
        if (entry.nodeIndex == nodeIndex) candidateMask &= entry.childMask;

        if (candidateMask == 0u)
        {
            releaseTransformEntry(transformStack, transformStackPtr, transformIdx);
            done = popTraversal(trail, level, currentSlot, currentTEntry, stack, stackPtr,
                                 packedIndexType, transformIdx, transformStack, transformStackPtr,
                                 startRay, rootIndex, rootType, closest);
            continue;
        }

        trail[level].nodeIndex = nodeIndex;
        trail[level].childMask = candidateMask;

        uint candSlots[KDOP_WIDTH];
        uint candCount = 0;
        for (uint s = 0; s < hitCount; ++s)
        {
            uint slot = sortedSlots[s];
            if (candidateMask & (1u << slot))
            {
                candSlots[candCount++] = slot;
            }
        }

        uint closestSlot = candSlots[0];

        for (int s = (int)candCount - 1; s >= 1; --s)
        {
            uint slot = candSlots[s];
            uint pushedPacked, pushedTransformIdx;
            resolveCandidate(node, slot, tHit[slot], transformIdx,
                              transformStack, transformStackPtr,
                              pushedPacked, pushedTransformIdx);

            if (stackPtr >= SHORT_STACK_SIZE)
            {
                releaseTransformEntry(transformStack, transformStackPtr, stack[0].transformStackIndex);
                for (uint m = 1; m < SHORT_STACK_SIZE; ++m)
                    stack[m - 1] = stack[m];
                stackPtr--;
            }

            ShortStackNode entryNode;
            entryNode.packedIndexType = pushedPacked;
            entryNode.tEntry = tHit[slot];
            entryNode.transformStackIndex = pushedTransformIdx;
            entryNode.parentSlot = slot;

            stack[stackPtr++] = entryNode;
            transformStack[pushedTransformIdx].nodesUsingThis++;
        }

        // resolve and descend into closest
        uint nextPacked, nextTransformIdx;
        resolveCandidate(node, closestSlot, tHit[closestSlot], transformIdx,
                          transformStack, transformStackPtr,
                         nextPacked, nextTransformIdx);

        transformStack[nextTransformIdx].nodesUsingThis++;
        releaseTransformEntry(transformStack, transformStackPtr, transformIdx);

        packedIndexType = nextPacked;
        transformIdx = nextTransformIdx;
        currentSlot = closestSlot;
        currentTEntry = tHit[closestSlot];
        level++;
    }
    return closest;
}