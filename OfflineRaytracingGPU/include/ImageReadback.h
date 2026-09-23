#pragma once

#include "VulkanHelper.h"
#include <cstdint>

class ImageReadback {
public:
    bool init(VmaAllocator allocator, uint32_t w, uint32_t h, VkFormat format);
    void destroy();

    // `image` must already be in TRANSFER_SRC_OPTIMAL with prior writes made
    // visible to transfer reads. Layout is left untouched.
    void record(VkCommandBuffer cmd, VkImage image);

    // Only call after the fence of the submit that contained record() has signaled.
    bool writeEXR(const char* path, float scale = 1.0f) const;

private:
    VmaAllocator  m_allocator = nullptr;
    VkBuffer      m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_alloc = nullptr;
    void* m_mapped = nullptr;
    uint32_t      m_width = 0, m_height = 0;
    VkFormat      m_format = VK_FORMAT_UNDEFINED;
};