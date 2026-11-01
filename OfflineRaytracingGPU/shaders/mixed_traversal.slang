// slang file mixed threaded + stack traversal

/*

Need to mix threaded and normal stack (so then it can be a short stack)

Plan:
Small stack w threaded instances


*/

implementing scene;

Intersection intersect_scene_mixed(uint gi, Ray startRay, uint rootIndex, PrimType rootType)
{
    Intersection closest = no_intersection();
    closest.t = FLT_MAX;

    float tScale = 1.f;
    Ray currentRay = startRay;
    CompactTransformStack transformStack = { gi, 0 };

    TraversalNode root;
    root.packedIndexType = packChild(rootType, rootIndex);
    root.tEntry = 0;

    TraversalStack stack = { gi, 0 };

    stack.push(root);

    uint traversals = 0;

    while (!stack.empty() && traversals < MAX_TRAVERSALS)
    {
        traversals++;
        TraversalNode current = stack.pop();
        transformStack.checkTransform(stack.pt, startRay, currentRay, tScale);

        // early release if there is already a closer node
        float tEntry = current.tEntry;
        if (tEntry >= closest.t)
        {
            continue;
        }

        uint nodeIndex = unpackIndex(current.packedIndexType);
        PrimType currentType = unpackType(current.packedIndexType);



        if (currentType == PrimType::KDOP_NODE)
        {
            KDopNodeHot hot = kdopHotNodes[nodeIndex];
            float tHit[KDOP_WIDTH];
            uint validMask = intersectKDopChildren(nodeIndex, hot, currentRay.origin, currentRay.direction, closest.t, tScale, tHit);

            if (validMask == 0u)
            {
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
                    transformStack, 
                    current, 
                    closest, 
                    index, 
                    type,
                    tHit[slot],
                    currentRay,
                    tScale
                );
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
                float rawL = intersect_aabb(currentRay, leftMin, leftMax);
                if (rawL < FLT_MAX)
                {
                    leftT = tScale * rawL;
                }
            }
            if (rightType != PrimType::EMPTY)
            {
                float rawR = intersect_aabb(currentRay, rightMin, rightMax);
                if (rawR < FLT_MAX)
                {
                    rightT = tScale * rawR;
                }
            }

            if (leftT < rightT)
            {
                // push far child first
                if (rightT < closest.t)
                {
                    processChild(stack, transformStack, current, closest, rightIndex, rightType, rightT, currentRay, tScale);
                }
                if (leftT < closest.t)
                {
                    processChild(stack, transformStack, current, closest, leftIndex, leftType, leftT, currentRay, tScale);
                }
            }
            else
            {
                if (leftT < closest.t)
                {
                    processChild(stack, transformStack, current, closest, leftIndex, leftType, leftT, currentRay, tScale);
                }
                if (rightT < closest.t)
                {
                    processChild(stack, transformStack, current, closest, rightIndex, rightType, rightT, currentRay, tScale);
                }
            }
        }
        else if (currentType == PrimType::THREADED_NODE)
        {
            Intersection new_closest = intersect_scene_threaded(currentRay, nodeIndex, currentType, closest, tScale, transpose(transformStack.cumulativeTransform));
            if (new_closest.t < closest.t)
            {
                closest = new_closest;
            }
        }
        else if (currentType == PrimType::TRANSFORM)
        {
            Transform t = transforms[nodeIndex];
            if (transformStack.push(nodeIndex, stack.pt, currentRay, tScale))
            {
                TraversalNode child;
                child.packedIndexType = packChild(t.childPrim, t.childIndex);
                child.tEntry = current.tEntry;
                stack.push(child);
            }
        }
        else
        {
            // primitive or a transform -> transform chain
            processChild(stack, transformStack, current, closest, nodeIndex, currentType, current.tEntry, currentRay, tScale);
        }
    }

    return closest;
}