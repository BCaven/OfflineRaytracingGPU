implementing scene;

__include shortstackKDOP_header;

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
    inout uint transformStackPtr,
    inout uint transformStackIndex,

    inout TransformStackNode transformStack[TRANSFORM_STACK_SIZE],
    inout ShortStackNode stack[SHORT_STACK_SIZE],
    inout uint stackPtr,

    inout uint packedIndexType,

    Ray startRay,
    uint rootIndex,
    PrimType rootType,

    Intersection closest)
{
    // TODO: remember to release the transform entries
    // void releaseTransformEntry(inout TransformStackNode transformStack[TRANSFORM_STACK_SIZE], inout uint transformStackPtr, uint idx)

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
        // clear the node that is getting overwritten (I think?)
        releaseTransformEntry(transformStack, transformStackPtr, transformStackIndex);
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
    transformStackIndex = 0;
    // TODO: uncertain if the transform stack will always be cleared by the time we get here...
    // need to make sure it is

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
    bool exit = false;
    uint iterations = 0;

    ShortStackNode stack[SHORT_STACK_SIZE];
    uint stackPtr = 0; // was `int` — mismatched popTraversal's `inout uint`

    RestartTrailEntry trail[RESTART_TRAIL_SIZE];
    [[unroll]]
    for (int i = 0; i < RESTART_TRAIL_SIZE; i++)
    {
        trail[i].nodeIndex = INVALID_NODE;
        trail[i].childMask = 0u;
    }

    uint packedIndexType = packChild(rootType, rootIndex);
    uint level = 0;
    uint currentSlot = 0;
    float currentTEntry = 0.f;
    uint currentTransformIndex = 0;

    Ray currentRay = rays[rayIndex];

    Intersection closest = no_intersection();
    closest.t = FLT_MAX;

    TransformStackNode identityTransform;
    identityTransform.tScale = 1.f;
    identityTransform.rayDirection = currentRay.direction;
    identityTransform.rayOrigin = currentRay.origin;
    identityTransform.transformDepth = 0;
    identityTransform.nodesUsingThis = 1;
    TransformStackNode transformStack[TRANSFORM_STACK_SIZE];
    int transformStackPtr = 0;
    transformStack[transformStackPtr++] = identityTransform;

    while (!exit && iterations < MAX_TRAVERSALS)
    {
        iterations++;

        PrimType currentType = unpackType(packedIndexType);
        int currentIndex = unpackIndex(packedIndexType);

        if (currentType == PrimType::KDOP_NODE)
        {
            KDopNodeHot hot = kdopHotNodes[currentIndex];
            float3 currentRayDir = currentRay.direction;
            float3 currentRayOrigin = currentRay.origin;

            float tHit[KDOP_WIDTH];
            uint validChildrenMask = intersectKDopChildren(
                currentIndex, hot, currentRayOrigin, currentRayDir, currentTEntry, 1, tHit);

            uint sortedIndexes[KDOP_WIDTH];
            uint lenSorted = sortChildrenByDistance(validChildrenMask, tHit, sortedIndexes);

            if (trail[level].nodeIndex != currentIndex)
            {
                // First time we've reached this node at this depth this pass.
                trail[level].nodeIndex = currentIndex;
                trail[level].childMask = validChildrenMask;
            }
            else
            {
                // Resumed via a root-restart: closest.t may have tightened
                // since we were last here, so drop anything now unreachable.
                trail[level].childMask &= validChildrenMask;
            }

            uint pendingMask = trail[level].childMask;

            if (pendingMask == 0u)
            {
                exit = popTraversal(trail, level, currentSlot, currentTEntry,
                                     transformStackPtr, currentTransformIndex, transformStack, 
                                     stack, stackPtr, packedIndexType,
                                    currentRay, rootIndex, rootType, closest);
                continue;
            }

            // Walk nearest-first; take the first still-pending slot as
            // `current`, push the rest (nearest-first) for later.
            int pickedSlot = -1;
            for (uint r = 0; r < lenSorted; ++r)
            {
                uint slot = sortedIndexes[r];
                if ((pendingMask & (1u << slot)) == 0u)
                    continue; // already finished in an earlier pass

                if (pickedSlot < 0)
                {
                    pickedSlot = int(slot);
                    continue;
                }

                if (stackPtr < SHORT_STACK_SIZE)
                {
                    ShortStackNode entry;
                    entry.packedIndexType = bit_cast<uint>(hot.min_packed[slot].w);
                    entry.tEntry = tHit[slot];
                    entry.parentLevel = level;
                    entry.parentSlot = slot;
                    stack[stackPtr++] = entry;
                }
                // else: stays set in childMask, recovered on a future
                // root-restart -- this is the whole point of the trail.
            }

            // TODO later: add transforms in once traversal works as expected
            // (route pickedSlot's child, and each pushed child, through
            // resolveCandidate() instead of unpacking directly here).
            packedIndexType = bit_cast<uint>(hot.min_packed[pickedSlot].w);
            currentTEntry = tHit[pickedSlot];
            currentSlot = uint(pickedSlot);
            level++;
        }
        else if (currentType == PrimType::BVH_NODE)
        {
            // Not implemented yet -- pop instead of falling through, so
            // reaching one doesn't hang the loop.
            exit = popTraversal(trail, level, currentSlot, currentTEntry,
                                transformStackPtr, currentTransformIndex, transformStack,
                                stack, stackPtr, packedIndexType,
                                currentRay, rootIndex, rootType, closest);
        }
        else
        {
            TransformStackNode chain = transformStack[currentTransformIndex];
            ProcessPrimitive(currentType, currentIndex, currentRay, closest, chain);

            exit = popTraversal(trail, level, currentSlot, currentTEntry,
                                transformStackPtr, currentTransformIndex, transformStack,
                                stack, stackPtr, packedIndexType,
                                currentRay, rootIndex, rootType, closest);
        }
    }

    return closest;
}

