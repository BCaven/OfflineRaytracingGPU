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

    TransformStackNode identityTransform;
    identityTransform.tScale = 1.f;
    identityTransform.rayDirection = startRay.direction;
    identityTransform.rayOrigin = startRay.origin;
    identityTransform.transformDepth = 0;
    identityTransform.nodesUsingThis = 1;
    TransformStack transformStack = { gi, 0 };
    transformStack.push(identityTransform);

    TraversalNode root;
    root.packedIndexType = packChild(rootType, rootIndex);
    root.tEntry = 0;
    root.transformStackIndex = 0;

    TraversalStack stack = { gi, 0 };

    stack.push(root);

    uint traversals = 0;

    while (!stack.empty() && traversals < MAX_TRAVERSALS)
    {
        traversals++;
        TraversalNode current = stack.pop();
        // early release if there is already a closer node
        float tEntry = current.tEntry;
        if (tEntry >= closest.t)
        {
            transformStack.releaseTransformEntry(current.transformStackIndex);
            continue;
        }

        uint nodeIndex = unpackIndex(current.packedIndexType);
        PrimType currentType = unpackType(current.packedIndexType);

        TransformStackNode transformNode = transformStack.get(current.transformStackIndex);

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
                transformStack.releaseTransformEntry(current.transformStackIndex);
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
                float rawL = intersect_aabb(intersectionRay, leftMin, leftMax);
                if (rawL < FLT_MAX)
                {
                    leftT = transformNode.tScale * rawL;
                }
            }
            if (rightType != PrimType::EMPTY)
            {
                float rawR = intersect_aabb(intersectionRay, rightMin, rightMax);
                if (rawR < FLT_MAX)
                {
                    rightT = transformNode.tScale * rawR;
                }
            }

            if (leftT < rightT)
            {
                // push far child first
                if (rightT < closest.t)
                {
                    processChild(stack, transformStack, current, closest, rightIndex, rightType, rightT);
                }
                if (leftT < closest.t)
                {
                    processChild(stack, transformStack, current, closest, leftIndex, leftType, leftT);
                }
            }
            else
            {
                if (leftT < closest.t)
                {
                    processChild(stack, transformStack, current, closest, leftIndex, leftType, leftT);
                }
                if (rightT < closest.t)
                {
                    processChild(stack, transformStack, current, closest, rightIndex, rightType, rightT);
                }
            }
        }
        else if (currentType == PrimType::THREADED_NODE)
        {
            Ray intersectionRay;
            TransformStackNode currentTransform = transformStack.get(current.transformStackIndex);
            intersectionRay.direction = currentTransform.rayDirection;
            intersectionRay.origin = currentTransform.rayOrigin;

            float3x3 normalMatrix = buildNormalMatrix(currentTransform);

            Intersection new_closest = intersect_scene_threaded(intersectionRay, nodeIndex, currentType, closest, currentTransform.tScale, normalMatrix);
            if (new_closest.t < closest.t)
            {
                closest = new_closest;
            }
        }
        else if (currentType == PrimType::TRANSFORM)
        {
            /*

            else if (childType == PrimType::TRANSFORM)
            {
                Transform t = transforms[childIndex];
                TransformStackNode transformStackNode = transformStack.get(node.transformStackIndex);
                TraversalNode resolved;
                TransformStackNode resolvedStackNode;
                node.tEntry = childT;
                // the transformed BVH node
                if (transformRay(node, transformStackNode, childIndex, t, resolved, resolvedStackNode))
                {
                    resolvedStackNode.nodesUsingThis = 1;
                    uint newTransformIndex = transformStack.pt;
                    resolved.transformStackIndex = newTransformIndex;

                    stack.push(resolved);
                    transformStack.push(resolvedStackNode);

                }
            }
            */
            // lazy processing of transforms
            Transform t = transforms[nodeIndex];
            TraversalNode resolved;
            TransformStackNode rn;
            if (transformRay(current, transformNode, nodeIndex, t, resolved, rn))
            {
                if (unpackType(resolved.packedIndexType) == PrimType::THREADED_NODE)
                {
                    // instanced BLAS: run it directly with the local copy, no stack entries at all
                    float3x3 nm = buildNormalMatrix(rn);
                    Ray r; r.origin = rn.rayOrigin; r.direction = rn.rayDirection;
                    Intersection h = intersect_scene_threaded(r, unpackIndex(resolved.packedIndexType),
                                                              PrimType::THREADED_NODE, closest, rn.tScale, nm);
                    if (h.t < closest.t) closest = h;
                }
                else
                {
                    rn.nodesUsingThis = 1;
                    resolved.transformStackIndex = transformStack.pt;
                    if (transformStack.push(rn)) stack.push(resolved);
                }
            }
        }
        else
        {
            // primitive or a transform -> transform chain
            processChild(stack, transformStack, current, closest, nodeIndex, currentType, current.tEntry);
        }
        transformStack.releaseTransformEntry(current.transformStackIndex);
    }

    return closest;
}