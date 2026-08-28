implementing scene;

__include bvh_header;


void push_stack(inout TraversalNode stack[STACK_SIZE],
                inout uint stackPtr,
                TraversalNode item)
{
    if (stackPtr < STACK_SIZE)
    {
        stack[stackPtr++] = item;
    }
}

void push_transform_stack(inout TransformStackNode stack[TRANSFORM_STACK_SIZE], inout uint transformStackPtr, TransformStackNode item)
{
    if (transformStackPtr < TRANSFORM_STACK_SIZE)
    {
        stack[transformStackPtr++] = item;
    }
}

void releaseTransformEntry(inout TransformStackNode transformStack[TRANSFORM_STACK_SIZE], inout uint transformStackPtr, uint idx)
{
    transformStack[idx].nodesUsingThis--;
    while (transformStackPtr > 0 && transformStack[transformStackPtr - 1].nodesUsingThis == 0)
    {
        transformStackPtr--;
    }
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
    inout TraversalNode stack[STACK_SIZE], 
    inout uint stackPtr,
    inout TransformStackNode transformStack[TRANSFORM_STACK_SIZE],
    inout uint transformStackPtr,
    TraversalNode node, 
    inout Intersection closest, 
    uint childIndex,
    PrimType childType,
    float childT
)
{
    if (childType == PrimType::BVH_NODE || childType == PrimType::KDOP_NODE)
    {
        // Dont push a new transform chain to the transform stack since we are just using the same one from before
        TraversalNode childNode;
        childNode.packedIndexType = packChild(childType, childIndex);
        childNode.tEntry = childT;
        childNode.transformStackIndex = node.transformStackIndex;
        transformStack[node.transformStackIndex].nodesUsingThis += 1;

        push_stack(stack, stackPtr, childNode);
    }
    else if (childType == PrimType::TRANSFORM)
    {
        Transform t = transforms[childIndex];
        TransformStackNode transformStackNode = transformStack[node.transformStackIndex];
        TraversalNode resolved;
        TransformStackNode resolvedStackNode;
        node.tEntry = childT;
        // the transformed BVH node
        if (transformRay(node, transformStackNode, childIndex, t, resolved, resolvedStackNode))
        {
            resolvedStackNode.nodesUsingThis = 1;
            uint newTransformIndex = transformStackPtr;
            resolved.transformStackIndex = newTransformIndex;

            push_stack(stack, stackPtr, resolved);
            push_transform_stack(transformStack, transformStackPtr, resolvedStackNode);
            
        }        
    }
    else
    {
        Ray intersectionRay;
        TransformStackNode currentTransform = transformStack[node.transformStackIndex];
        intersectionRay.direction = currentTransform.rayDirection;
        intersectionRay.origin = currentTransform.rayOrigin;
        ProcessPrimitive(childType, childIndex, intersectionRay, closest, currentTransform);
    }
}

Intersection intersect_scene_wide_dfs(int rayIndex, uint rootIndex, PrimType rootType)
{
    Ray startRay = rays[rayIndex];
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
            uint validMask = intersectKDopChildren(nodeIndex, hot, intersectionRay.origin, intersectionRay.direction, closest.t, 1., tHit);

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
            processChild(stack, stackPtr, transformStack, transformStackPtr, current, closest, nodeIndex, currentType, closest.t);
        }
        releaseTransformEntry(transformStack, transformStackPtr, current.transformStackIndex);
    }

    return closest;
}