implementing scene;

__include bvh_header;

float3x3 buildNormalMatrix(TransformStackNode currentTransform)
{
    float3x3 normalMatrix = identity3x3();

    for (uint i = 0; i < currentTransform.transformDepth; i++)
    {
        float3x3 transformNormal = transpose((float3x3)transforms[currentTransform.transformChain[i]].invMatrix);
        normalMatrix = mul(normalMatrix, transformNormal);
    }
    return normalMatrix;
}

bool transformRay(TraversalNode node, TransformStackNode transformNode, uint transformIdx, Transform t, out TraversalNode result, out TransformStackNode resultTransformStack)
{
    float3 localOrigin = mul(t.invMatrix, float4(transformNode.rayOrigin, 1.f)).xyz;
    float3 localDirection = mul((float3x3) t.invMatrix, transformNode.rayDirection);

    float directionLength = length(localDirection);
    if (directionLength < EPSILON)
    {
        // bad transform, this should never happen. Is here for debugging
        result = TraversalNode();
        resultTransformStack = TransformStackNode();
        return false;
    }
    float3 dir = localDirection / directionLength;
    result.packedIndexType = packChild(t.childPrim, t.childIndex);
    resultTransformStack.rayOrigin = localOrigin;
    resultTransformStack.rayDirection = localDirection / directionLength;
    resultTransformStack.tScale = transformNode.tScale / directionLength;
    result.tEntry = node.tEntry;

    resultTransformStack.transformChain = transformNode.transformChain;
    if (transformNode.transformDepth < MAX_TRANSFORM_DEPTH)
    {
        resultTransformStack.transformChain[transformNode.transformDepth] = transformIdx; // since this is the traversal node pointing to a transform
    }
    resultTransformStack.transformDepth = min(transformNode.transformDepth + 1, MAX_TRANSFORM_DEPTH);
    return true;
}

void processChild(
    inout TraversalStack stack,
    inout TransformStack transformStack,
    TraversalNode node, 
    inout Intersection closest, 
    uint childIndex,
    PrimType childType,
    float childT
)
{
    if (childType == PrimType::BVH_NODE || childType == PrimType::KDOP_NODE || childType == PrimType::THREADED_NODE || childType == PrimType::TRANSFORM)
    {
        // Dont push a new transform chain to the transform stack since we are just using the same one from before
        TraversalNode childNode;
        childNode.packedIndexType = packChild(childType, childIndex);
        childNode.tEntry = childT;
        childNode.transformStackIndex = node.transformStackIndex;
        transformStack.increaseNodesUsing(node.transformStackIndex, 1);

        stack.push(childNode);
    }
    else
    {
        Ray intersectionRay;
        TransformStackNode currentTransform = transformStack.get(node.transformStackIndex);
        intersectionRay.direction = currentTransform.rayDirection;
        intersectionRay.origin = currentTransform.rayOrigin;

        float3x3 normalMatrix = identity3x3();

        for (uint i = 0; i < currentTransform.transformDepth; i++)
        {
            float3x3 transformNormal = transpose((float3x3)transforms[currentTransform.transformChain[i]].invMatrix);
            normalMatrix = mul(normalMatrix, transformNormal);
        }

        ProcessPrimitive(childType, childIndex, intersectionRay, closest, normalMatrix, currentTransform.tScale);
    }
}

Intersection intersect_scene_wide_dfs(uint gi, Ray startRay, uint rootIndex, PrimType rootType)
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
        else
        {
            // primitive or a transform -> transform chain
            processChild(stack, transformStack, current, closest, nodeIndex, currentType, current.tEntry);
        }
        transformStack.releaseTransformEntry(current.transformStackIndex);
    }

    return closest;
}