// slang file mixed threaded + stack traversal

/*

Need to mix threaded and normal stack (so then it can be a short stack)

Plan:
Small stack w threaded instances


*/

implementing scene;

Intersection intersect_scene_mixed(Ray startRay, uint rootIndex, PrimType rootType)
{
    Intersection closest = no_intersection();
    closest.t = FLT_MAX;

    TransformStackNode identityTransform;
    identityTransform.tScale = 1.f;
    identityTransform.rayDirection = startRay.direction;
    identityTransform.rayOrigin = startRay.origin;
    identityTransform.transformDepth = 0;
    identityTransform.nodesUsingThis = 1;
    TransformStackNode transformStack[TRANSFORM_STACK_SIZE];
    int transformStackPtr = 0;
    transformStack[transformStackPtr++] = identityTransform;

    TraversalNode root;
    root.packedIndexType = packChild(rootType, rootIndex);
    root.tEntry = 0;
    root.transformStackIndex = 0;
    TraversalNode stack[STACK_SIZE];
    int stackPtr = 0;

    stack[stackPtr++] = root;

    uint traversals = 0;

    while (stackPtr > 0 && traversals < MAX_TRAVERSALS)
    {
        TraversalNode current = stack[--stackPtr];
        // early release if there is already a closer node
        float tEntry = current.tEntry;
        if (tEntry >= closest.t)
        {
            releaseTransformEntry(transformStack, transformStackPtr, current.transformStackIndex);
            continue;
        }

        uint nodeIndex = unpackIndex(current.packedIndexType);
        PrimType currentType = unpackType(current.packedIndexType);

        TransformStackNode transformNode = transformStack[current.transformStackIndex];

        Ray intersectionRay;
        intersectionRay.direction = transformNode.rayDirection;
        intersectionRay.origin = transformNode.rayOrigin;

        if (currentType == PrimType::KDOP_NODE)
        {
            KDopNodeHot hot = kdopHotNodes[nodeIndex];
            float tHit[KDOP_WIDTH];
            uint validMask = intersectKDopChildren(nodeIndex, hot, intersectionRay.origin, intersectionRay.direction, closest.t, transformNode.tScale, tHit);

            if (validMask == 0u)
            {
                releaseTransformEntry(transformStack, transformStackPtr, current.transformStackIndex);
                continue;
            }

            uint sortedSlots[KDOP_WIDTH];
            uint count = sortChildrenByDistance(validMask, tHit, sortedSlots);

            for (int i = (int)count - 1; i >= 0; --i)
            {
                uint slot = sortedSlots[i];
                uint index = unpackIndex(bit_cast<uint>(hot.min_packed[slot].w));
                PrimType type = unpackType(bit_cast<uint>(hot.min_packed[slot].w));

                processChild(
                    stack, 
                    stackPtr, 
                    transformStack, 
                    transformStackPtr, 
                    current, 
                    closest, 
                    index, 
                    type,
                             tHit[slot]);
            }
        }
        else if (currentType == PrimType::BVH_NODE)
        {
            float leftT = FLT_MAX;
            float rightT = FLT_MAX;
            const bvhPacked currentBVHNode = bvhNodes[nodeIndex];
            float3 leftMin = currentBVHNode.leftMin_leftPacked.xyz;
            float3 leftMax = currentBVHNode.leftMax_rightPacked.xyz;
            float3 rightMin = currentBVHNode.rightMin_reserved0.xyz;
            float3 rightMax = currentBVHNode.rightMax_reserved1.xyz;
            uint leftIndex = unpackIndex(bit_cast<uint>(currentBVHNode.leftMin_leftPacked.w));
            PrimType leftType = unpackType(bit_cast<uint>(currentBVHNode.leftMin_leftPacked.w));
            uint rightIndex = unpackIndex(bit_cast<uint>(currentBVHNode.leftMax_rightPacked.w));
            PrimType rightType = unpackType(bit_cast<uint>(currentBVHNode.leftMax_rightPacked.w));
            if (leftType != PrimType::EMPTY)
            {
                leftT = transformNode.tScale * intersect_aabb(intersectionRay, leftMin, leftMax);
            }
            if (rightType != PrimType::EMPTY)
            {
                rightT = transformNode.tScale * intersect_aabb(intersectionRay, rightMin, rightMax);
            }

            if (leftT < rightT)
            {
                // push far child first
                if (rightT < closest.t)
                {
                    processChild(stack, stackPtr, transformStack, transformStackPtr, current, closest, rightIndex, rightType, rightT);
                }
                if (leftT < closest.t)
                {
                    processChild(stack, stackPtr, transformStack, transformStackPtr, current, closest, leftIndex, leftType, leftT);
                }
            }
            else
            {
                if (leftT < closest.t)
                {
                    processChild(stack, stackPtr, transformStack, transformStackPtr, current, closest, leftIndex, leftType, leftT);
                }
                if (rightT < closest.t)
                {
                    processChild(stack, stackPtr, transformStack, transformStackPtr, current, closest, rightIndex, rightType, rightT);
                }
            }
        }
        else
        {
            // primitive or a transform -> transform chain
            processChild(stack, stackPtr, transformStack, transformStackPtr, current, closest, nodeIndex, currentType, current.tEntry);
        }
        releaseTransformEntry(transformStack, transformStackPtr, current.transformStackIndex);
    }

    return closest;
}