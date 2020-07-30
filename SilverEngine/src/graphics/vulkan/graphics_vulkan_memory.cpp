#include "core.h"

#define VMA_IMPLEMENTATION
#include "graphics_vulkan_memory.h"
#include "graphics_vulkan.h"

namespace _sv {

	void MemoryManager::GetMappingData(ui32 size, VkBuffer& buffer, void** data, ui32& offset)
	{
		// 1 megabytes
		constexpr ui32 BLOCK_SIZE = 1000000;

		if (m_CurrentStagingBuffer.buffer == VK_NULL_HANDLE) {
			m_CurrentStagingBufferOffset = 0u;
			m_CurrentStagingBuffer = CreateStagingBuffer(std::max(BLOCK_SIZE, size));
		}

		if (m_CurrentStagingBuffer.size - m_CurrentStagingBufferOffset < size) {
			
			m_ActiveStaggingBuffers.push_back(m_CurrentStagingBuffer);
			m_CurrentStagingBufferOffset = 0u;

			bool found = false;

			if (size <= BLOCK_SIZE) {
				for (ui32 i = 0; i < m_StaggingBuffers.size(); ++i) {
					StagingBuffer& b = m_StaggingBuffers[i];
					if (b.size >= size) {
						found = true;
						m_CurrentStagingBuffer = b;
						m_StaggingBuffers.erase(m_StaggingBuffers.begin() + i);
						break;
					}
				}
			}
			else {
				for (i32 i = m_StaggingBuffers.size() - 1; i >= 0; --i) {
					StagingBuffer& b = m_StaggingBuffers[i];
					if (b.size >= size) {
						found = true;
						m_CurrentStagingBuffer = b;
						m_StaggingBuffers.erase(m_StaggingBuffers.begin() + i);
						break;
					}
				}
			}
			
			if (!found) {
				m_CurrentStagingBuffer = CreateStagingBuffer(std::max(size, BLOCK_SIZE));
			}

		}

		offset = m_CurrentStagingBufferOffset;
		buffer = m_CurrentStagingBuffer.buffer;
		*data = (ui8*)(m_CurrentStagingBuffer.mapData) + m_CurrentStagingBufferOffset;
		m_CurrentStagingBufferOffset += size;
	}

	void MemoryManager::Reset()
	{
		ResetStagingBuffers();
	}

	void MemoryManager::Clear()
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();

		Reset();
		if (m_CurrentStagingBuffer.buffer != VK_NULL_HANDLE) {
			m_StaggingBuffers.push_back(m_CurrentStagingBuffer);
		}
		for (ui32 i = 0; i < m_StaggingBuffers.size(); ++i) {
			const StagingBuffer& b = m_StaggingBuffers[i];
			vmaDestroyBuffer(gfx.GetAllocator(), b.buffer, b.allocation);
		}
		m_StaggingBuffers.clear();
	}

	void MemoryManager::ResetStagingBuffers()
	{
		if (m_CurrentStagingBuffer.buffer != VK_NULL_HANDLE) {
			m_ActiveStaggingBuffers.push_back(m_CurrentStagingBuffer);
			m_CurrentStagingBuffer = {};
		}
		m_CurrentStagingBufferOffset = 0u;

		m_StaggingBuffers.insert(m_StaggingBuffers.end(), m_ActiveStaggingBuffers.begin(), m_ActiveStaggingBuffers.end());
		m_ActiveStaggingBuffers.clear();
		std::sort(m_StaggingBuffers.begin(), m_StaggingBuffers.end(), [](const StagingBuffer& b0, const StagingBuffer& b1) {
			if (b0.size == b1.size) return b0.buffer < b1.buffer;
			return b0.size < b1.size;
		});

		if (!m_StaggingBuffers.empty()) {
			m_CurrentStagingBuffer = m_StaggingBuffers[0];
			m_StaggingBuffers.erase(m_StaggingBuffers.begin());
		}
	}

	StagingBuffer MemoryManager::CreateStagingBuffer(ui32 size)
	{
		StagingBuffer res;
		vkAssert(graphics_vulkan_memory_create_stagingbuffer(res.buffer, res.allocation, &res.mapData, size));
		res.size = size;
		return res;
	}



	VkResult graphics_vulkan_memory_create_stagingbuffer(VkBuffer& buffer, VmaAllocation& allocation, void** mapData, VkDeviceSize size)
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();

		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_info.queueFamilyIndexCount = 0u;
		buffer_info.pQueueFamilyIndices = nullptr;

		VmaAllocationCreateInfo alloc_info{};
		alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		vkExt(vmaCreateBuffer(gfx.GetAllocator(), &buffer_info, &alloc_info, &buffer, &allocation, nullptr));

		*mapData = allocation->GetMappedData();

		return VK_SUCCESS;
	}

}