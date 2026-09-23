// image_readback.cpp
#include "ImageReadback.h"
#include <glm/gtc/packing.hpp>
#include <vector>
#include <cstdio>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

bool ImageReadback::init(VmaAllocator allocator, uint32_t w, uint32_t h, VkFormat format)
{
    size_t bpp;
    if (format == VK_FORMAT_R32G32B32A32_SFLOAT) bpp = 16;
    else if (format == VK_FORMAT_R16G16B16A16_SFLOAT) bpp = 8;
    else return false;

    m_allocator = allocator; m_width = w; m_height = h; m_format = format;

    VkBufferCreateInfo bci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bci.size = VkDeviceSize(w) * h * bpp;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_AUTO;
    aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo info{};
    if (vmaCreateBuffer(allocator, &bci, &aci, &m_buffer, &m_alloc, &info) != VK_SUCCESS)
        return false;
    m_mapped = info.pMappedData;
    return m_mapped != nullptr;
}

void ImageReadback::destroy()
{
    if (m_buffer) vmaDestroyBuffer(m_allocator, m_buffer, m_alloc);
    m_buffer = VK_NULL_HANDLE; m_alloc = nullptr; m_mapped = nullptr;
}

void ImageReadback::record(VkCommandBuffer cmd, VkImage image)
{
    VkBufferImageCopy region{
        .bufferRowLength = 0,       // tightly packed
        .bufferImageHeight = 0,
        .imageSubresource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .imageExtent{ m_width, m_height, 1 }
    };
    vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        m_buffer, 1, &region);

    VkBufferMemoryBarrier2 toHost{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
        .dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = m_buffer,
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };
    VkDependencyInfo di{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &toHost
    };
    vkCmdPipelineBarrier2(cmd, &di);
}

bool ImageReadback::writeEXR(const char* path, float scale) const
{
    vmaInvalidateAllocation(m_allocator, m_alloc, 0, VK_WHOLE_SIZE);

    const size_t n = size_t(m_width) * m_height;
    std::vector<float> rgb(n * 3);

    if (m_format == VK_FORMAT_R32G32B32A32_SFLOAT) {
        const float* s = static_cast<const float*>(m_mapped);
        for (size_t i = 0; i < n; ++i)
            for (int c = 0; c < 3; ++c)
                rgb[i * 3 + c] = s[i * 4 + c] * scale;
    }
    else {
        const uint16_t* s = static_cast<const uint16_t*>(m_mapped);
        for (size_t i = 0; i < n; ++i)
            for (int c = 0; c < 3; ++c)
                rgb[i * 3 + c] = glm::unpackHalf1x16(s[i * 4 + c]) * scale;
    }

    const char* err = nullptr;
    int rc = SaveEXR(rgb.data(), int(m_width), int(m_height), 3,
        /*save_as_fp16=*/0, path, &err);
    if (rc != TINYEXR_SUCCESS) {
        fprintf(stderr, "EXR write failed: %s\n", err ? err : "unknown");
        if (err) FreeEXRErrorMessage(err);
        return false;
    }
    return true;
}