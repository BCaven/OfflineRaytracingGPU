implementing scene;

__include bvh_header;

void ProcessPrimitive(
    PrimType type,
    int index,
    Ray ray,
    inout Intersection closest,
    TransformStackNode transformNode)
{
    Intersection hit = no_intersection();

    // this check should be unnecessary
    if (index < 0)
    {
        return;
    }
    switch (type)
    {
    case PrimType::SPHERE:
        hit = intersect_sphere(ray, spheres[index]);
        break;
    case PrimType::TRIANGLE:
        hit = intersect_triangle(ray, triangles[index]);
        break;
    case PrimType::GAUSSIAN_SPLAT:
        hit = intersect_splat(ray, splats[index], closest.t / transformNode.tScale);
        break;
    default:
        return;
    }
    if (hit.t > 0 && hit.t * transformNode.tScale < closest.t)
    {
        // TODO: rebuild the normal transformation matrix
        float3x3 normalMatrix = identity3x3();

        for (uint i = 0; i < transformNode.transformDepth; i++)
        {
            float3x3 transformNormal = transpose((float3x3)transforms[transformNode.transformChain[i]].invMatrix);
            normalMatrix = mul(normalMatrix, transformNormal);
        }

        float3 newNormal = normalize(mul(normalMatrix, hit.normal));
        hit.normal = newNormal;
        hit.t *= transformNode.tScale;
        closest = hit;
    }
}

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
        // Assume TRANSFORMs can *only* hold BVH_NODE
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



Intersection intersect_scene_bvh(int rayIndex, uint rootIndex, PrimType rootType)
{
    // TODO: try this using a Rope / threaded BVH later
    /*
    push root
    closest.t = FLT_MAX
    while stack not empty:
        if TRANSFORM:
            push transformed TraversalNode
        if BVH_NODE:
            check children
            potentially push children
    return closest
    */
    int iterations = 0;
    Ray startRay = rays[rayIndex];
    TraversalNode root; // = TraversalNode(rootIndex, startRay.origin, startRay.direction, 1.f, 0.f);
    root.packedIndexType = packChild(rootType, rootIndex);
    root.transformStackIndex = 0;
    root.tEntry = 0.f;
    TraversalNode stack[STACK_SIZE];
    TransformStackNode transformStack[TRANSFORM_STACK_SIZE];

    TransformStackNode identityTransform;
    identityTransform.tScale = 1.f;
    identityTransform.rayDirection = startRay.direction;
    identityTransform.rayOrigin = startRay.origin;
    identityTransform.transformDepth = 0;
    identityTransform.nodesUsingThis = 1;

    uint stackPtr = 0;
    uint transformStackPtr = 0;

    transformStack[transformStackPtr++] = identityTransform;
    stack[stackPtr++] = root;

    Intersection closest = no_intersection();
    closest.t = FLT_MAX;

    while (stackPtr > 0 && iterations < MAX_TRAVERSALS)
    {
        iterations++;
        TraversalNode node = stack[--stackPtr];
        TransformStackNode transformNode = transformStack[node.transformStackIndex];

        float tEntry = node.tEntry;

        if (tEntry >= closest.t)
        {
            releaseTransformEntry(transformStack, transformStackPtr, node.transformStackIndex);
            continue;
        }

        Ray intersectionRay;
        intersectionRay.direction = transformNode.rayDirection;
        intersectionRay.origin = transformNode.rayOrigin;
        uint nodeIndex = unpackIndex(node.packedIndexType);
        PrimType nodeType = unpackType(node.packedIndexType);

        if (nodeType == PrimType::BVH_NODE)
        {
            float leftT = FLT_MAX;
            float rightT = FLT_MAX;
            const bvhPacked current = bvhNodes[nodeIndex];
            float3 leftMin = current.leftMin_leftPacked.xyz;
            float3 leftMax = current.leftMax_rightPacked.xyz;
            float3 rightMin = current.rightMin_reserved0.xyz;
            float3 rightMax = current.rightMax_reserved1.xyz;
            uint leftIndex = unpackIndex(bit_cast<uint>(current.leftMin_leftPacked.w));
            PrimType leftType = unpackType(bit_cast<uint>(current.leftMin_leftPacked.w));
            uint rightIndex = unpackIndex(bit_cast<uint>(current.leftMax_rightPacked.w));
            PrimType rightType = unpackType(bit_cast<uint>(current.leftMax_rightPacked.w));
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
                    processChild(stack, stackPtr, transformStack, transformStackPtr, node, closest, rightIndex, rightType, rightT);
                }
                if (leftT < closest.t)
                {
                    processChild(stack, stackPtr, transformStack, transformStackPtr, node, closest, leftIndex, leftType, leftT);
                }
            }
            else
            {
                if (leftT < closest.t)
                {
                    processChild(stack, stackPtr, transformStack, transformStackPtr, node, closest, leftIndex, leftType, leftT);
                }
                if (rightT < closest.t)
                {
                    processChild(stack, stackPtr, transformStack, transformStackPtr, node, closest, rightIndex, rightType, rightT);
                }
            }
        }
        else if (nodeType == PrimType::KDOP_NODE)
        {
            const KDopNodeHot hot = kdopHotNodes[nodeIndex];

            uint packed[KDOP_WIDTH];
            float childTNear[KDOP_WIDTH];
            float childTFar[KDOP_WIDTH];
            bool validChildren[KDOP_WIDTH];
            bool anyCandidates = false;
            [unroll]
            for (int i = 0; i < KDOP_WIDTH; ++i)
            {
                packed[i] = bit_cast<uint>(hot.min_packed[i].w);
                childTNear[i] = FLT_MAX;
                childTFar[i] = -FLT_MAX;
                validChildren[i] = false;
            }

            // check aabb before requesting full node
            [unroll]
            for (int i = 0; i < KDOP_WIDTH; ++i)
            {
                if (unpackType(packed[i]) == PrimType::EMPTY) continue;

                float2 aabbLocal = aabb_bounds(intersectionRay, hot.min_packed[i].xyz, hot.max[i].xyz);

                float worldT = transformNode.tScale * aabbLocal.x;

                if (worldT >= closest.t) continue;

                childTNear[i] = aabbLocal.x;
                childTFar[i] = aabbLocal.y;

                validChildren[i] = true;

                anyCandidates = true;
            }

            if (anyCandidates)
            {
                const K14DopNodeCold cold = kdopColdNodes[nodeIndex];

                [unroll]
                for (int i = 0; i < KDOP_WIDTH; ++i)
                {


                    if (childTNear[i] == FLT_MAX) continue;

                    if (validChildren[i] == false) continue; // node is already invalid from AABB

                    float2 kdopLocal_i = kdop_bounds_diag(intersectionRay,
                        cold.min[i], cold.max[i], childTNear[i], childTFar[i]);

                    //childTNear[i] = kdopLocal_i.x;
                    //childTFar[i] = kdopLocal_i.y;

                    float worldNear_i2 = transformNode.tScale * kdopLocal_i.x;

                    if (worldNear_i2 > closest.t)
                    {
                       // validChildren[i] = false;
                    }

                }

                // push everyone who is still valid
                [unroll]
                for (int i = 0; i < KDOP_WIDTH; ++i)
                {
                    if (validChildren[i] == false) continue;

                    uint stackIndex = unpackIndex(packed[i]);
                    PrimType type = unpackType(packed[i]);
                    float worldNear_i = transformNode.tScale * childTNear[i];
                    processChild(stack, stackPtr, transformStack, transformStackPtr, node, closest, stackIndex, type, worldNear_i);
                }
             }
        }
        else
        {
            // dead node somehow?
            // I don't think this code is possible in a procedural stack with procedural nodes
            releaseTransformEntry(transformStack, transformStackPtr, node.transformStackIndex);
            continue;
        }
        
        releaseTransformEntry(transformStack, transformStackPtr, node.transformStackIndex);
    }
    return closest;
}
