#pragma once

#include "..//graphics_internal.h"

namespace sv {

	// DEVICE FUNCTIONS

	void graphics_vulkan_device_prepare(GraphicsDevice& device);

	Result	graphics_vulkan_initialize();
	Result	graphics_vulkan_close();
	void* graphics_vulkan_get();

	Result graphics_vulkan_create(GraphicsPrimitiveType type, const void* desc, Primitive_internal* res);
	Result graphics_vulkan_destroy(Primitive_internal* primitive);

	CommandList graphics_vulkan_commandlist_begin();
	CommandList graphics_vulkan_commandlist_last();
	u32		graphics_vulkan_commandlist_count();

	void graphics_vulkan_renderpass_begin(CommandList);
	void graphics_vulkan_renderpass_end(CommandList);

	void graphics_vulkan_swapchain_resize();

	void graphics_vulkan_gpu_wait();

	void graphics_vulkan_frame_begin();
	void graphics_vulkan_frame_end();
	void graphics_vulkan_present(GPUImage* image, const GPUImageRegion& region, GPUImageLayout layout, CommandList cmd); // Blit image into swapchain image

	void graphics_vulkan_draw(u32, u32, u32, u32, CommandList);
	void graphics_vulkan_draw_indexed(u32, u32, u32, u32, u32, CommandList);

	void graphics_vulkan_image_clear(GPUImage*, GPUImageLayout, GPUImageLayout, const Color4f&, float, u32, CommandList);
	void graphics_vulkan_image_blit(GPUImage*, GPUImage*, GPUImageLayout, GPUImageLayout, u32, const GPUImageBlit*, SamplerFilter, CommandList);
	void graphics_vulkan_buffer_update(GPUBuffer*, void*, u32, u32, CommandList);
	void graphics_vulkan_barrier(const GPUBarrier*, u32, CommandList);

	void graphics_vulkan_event_begin(const char* name, CommandList cmd);
	void graphics_vulkan_event_mark(const char* name, CommandList cmd);
	void graphics_vulkan_event_end(CommandList cmd);

}

#ifdef SV_VULKAN_IMPLEMENTATION

#include "platform/platform_impl.h"

#ifdef SV_PLATFORM_WIN
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifdef SV_PLATFORM_LINUX
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "..//src/external/vk_mem_alloc.h"

#ifdef SV_DEBUG
#define vkAssert(x) if((x) != VK_SUCCESS) SV_ASSERT(#x)
#else
#define vkAssert(x) x
#endif

#define vkCheck(x) if((x) != VK_SUCCESS) return sv::Result_UnknownError
#define vkExt(x) do{ VkResult res = (x); if(res != VK_SUCCESS) return res; }while(0)

#undef CreateSemaphore

#include "../src/external/sprv/spirv_reflect.hpp"

#include "graphics_vulkan_conversions.h"

namespace sv {

	struct Graphics_vk;

	Graphics_vk& graphics_vulkan_device_get();

	constexpr u32	VULKAN_MAX_DESCRIPTOR_SETS = 100u;
	constexpr u32	VULKAN_MAX_DESCRIPTOR_TYPES = 32u;
	constexpr u32	VULKAN_DESCRIPTOR_ALLOC_COUNT = 10u;
	constexpr float VULKAN_UNUSED_OBJECTS_TIMECHECK = 30.f;
	constexpr float VULKAN_UNUSED_OBJECTS_LIFETIME = 10.f;

	// MEMORY

	class MemoryManager {

		struct StagingBuffer {
			VkBuffer buffer = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;
			void* mapData = nullptr;
			u32 frame = 0u;
		};

		std::vector<StagingBuffer> m_StaggingBuffers;
		std::vector<StagingBuffer> m_ActiveStaggingBuffers;
		StagingBuffer m_CurrentStagingBuffer;
		u32 m_CurrentStagingBufferOffset = 0u;
		u64 m_LastFrame = UINT64_MAX;
		size_t m_BufferSize;

	public:
		void Create(size_t size);
		void GetMappingData(u32 size, VkBuffer& buffer, void** data, u32& offset);
		void Clear();

	private:
		void Reset(u32 frame);
		StagingBuffer CreateStagingBuffer(u32 currentFrame);

	};

	VkResult graphics_vulkan_memory_create_stagingbuffer(VkBuffer& buffer, VmaAllocation& allocation, void** mapData, VkDeviceSize size);

	// DESCRIPTORS

	struct VulkanDescriptorPool {
		VkDescriptorPool pool;
		u32 count[3];
		u32 sets;
	};

	struct VulkanDescriptorSet {
		std::vector<VkDescriptorSet>	sets;
		u32							used = 0u;
	};

	struct DescriptorPool {
		std::vector<VulkanDescriptorPool>						pools;
		std::map<VkDescriptorSetLayout, VulkanDescriptorSet>	sets;
	};

	struct ShaderResourceBinding {
		u32				vulkanBinding;
		u32				userBinding;
	};
	struct ShaderDescriptorSetLayout {
		VkDescriptorSetLayout setLayout;
		std::vector<ShaderResourceBinding> bindings;
		u32 count[3];
	};

	VkDescriptorSet graphics_vulkan_descriptors_allocate_sets(DescriptorPool& descPool, const ShaderDescriptorSetLayout& layout);
	void			graphics_vulkan_descriptors_reset(DescriptorPool& descPool);
	void			graphics_vulkan_descriptors_clear(DescriptorPool& descPool);

	// PIPELINE

	struct PipelineDescriptorSetLayout {
		ShaderDescriptorSetLayout layouts[ShaderType_GraphicsCount];
	};

	struct VulkanPipeline {
		VulkanPipeline& operator=(const VulkanPipeline& other)
		{
			semanticNames = other.semanticNames;
			layout = other.layout;
			pipelines = other.pipelines;
			setLayout = other.setLayout;
			return *this;
		}

		std::mutex						mutex;
		std::mutex						creationMutex;

		std::map<std::string, u32>		semanticNames;

		VkPipelineLayout				layout = VK_NULL_HANDLE;
		std::map<size_t, VkPipeline>	pipelines;
		PipelineDescriptorSetLayout		setLayout;
		VkDescriptorSet					descriptorSets[ShaderType_GraphicsCount] = {};
		float							lastUsage;
	};

	struct Shader_vk;

	size_t graphics_vulkan_pipeline_compute_hash(const GraphicsState& state);
	bool graphics_vulkan_pipeline_create(VulkanPipeline& pipeline, Shader_vk* pVertexShader, Shader_vk* pPixelShader, Shader_vk* pGeometryShader);
	bool graphics_vulkan_pipeline_destroy(VulkanPipeline& pipeline);
	VkPipeline graphics_vulkan_pipeline_get(VulkanPipeline& pipeline, GraphicsState& state, size_t hash);

	// ADAPTER

	class Adapter_vk : public Adapter {
		VkPhysicalDevice					m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties			m_Properties;
		VkPhysicalDeviceFeatures			m_Features;
		VkPhysicalDeviceMemoryProperties	m_MemoryProps;

		struct {
			u32 graphics = UINT32_MAX;

			inline bool IsComplete() const noexcept { return graphics != UINT32_MAX; }

		} m_FamilyIndex;

	public:
		Adapter_vk() {}
		Adapter_vk(VkPhysicalDevice device);

		inline VkPhysicalDevice GetPhysicalDevice() const noexcept { return m_PhysicalDevice; }
		inline const VkPhysicalDeviceProperties& GetProperties() const noexcept { return m_Properties; }
		inline const VkPhysicalDeviceFeatures& GetFeatures() const noexcept { return m_Features; }
		inline const VkPhysicalDeviceMemoryProperties GetMemoryProperties() const noexcept { return m_MemoryProps; }

		inline auto GetFamilyIndex() const noexcept { return m_FamilyIndex; }

	};

	// PRIMITIVES

	// Buffer
	struct Buffer_vk : public GPUBuffer_internal {
		VkBuffer				buffer;
		VmaAllocation			allocation;
		VkDescriptorBufferInfo	buffer_info;
		MemoryManager			memory;
	};
	// Image
	struct Image_vk : public GPUImage_internal {
		VmaAllocation			allocation;
		VkImage					image = VK_NULL_HANDLE;
		VkImageView				renderTargetView = VK_NULL_HANDLE;
		VkImageView				depthStencilView = VK_NULL_HANDLE;
		VkImageView				shaderResouceView = VK_NULL_HANDLE;
		u64						ID;
		VkDescriptorImageInfo	image_info;
	};
	// Sampler
	struct Sampler_vk : public Sampler_internal {
		VkSampler				sampler = VK_NULL_HANDLE;
		VkDescriptorImageInfo	image_info;
	};
	// Shader
	struct Shader_vk : public Shader_internal {
		VkShaderModule							module = VK_NULL_HANDLE;
		std::unordered_map<std::string, u32>	semanticNames;
		ShaderDescriptorSetLayout				layout;
		u64									ID;
	};
	// RenderPass
	struct RenderPass_vk : public RenderPass_internal {
		VkRenderPass									renderPass;
		std::vector<std::pair<size_t, VkFramebuffer>>	frameBuffers;
		VkRenderPassBeginInfo							beginInfo;
		std::mutex										mutex;
	};
	// InputLayoutState
	struct InputLayoutState_vk : public InputLayoutState_internal {
		size_t hash;
	};
	// BlendState
	struct BlendState_vk : public BlendState_internal {
		size_t hash;
	};
	// DepthStencilState
	struct DepthStencilState_vk : public DepthStencilState_internal {
		size_t hash;
	};
	// RasterizerState
	struct RasterizerState_vk : public RasterizerState_internal {
		size_t hash;
	};

	// API STRUCTS

	struct Frame {
		VkCommandPool		commandPool;
		VkCommandBuffer		commandBuffers[GraphicsLimit_CommandList];
		VkCommandPool		transientCommandPool;
		VkFence				fence;
		DescriptorPool		descPool[GraphicsLimit_CommandList];
	};

	struct SwapChain {

		VkSurfaceKHR						surface = VK_NULL_HANDLE;
		VkSwapchainKHR						swapChain = VK_NULL_HANDLE;

		VkSemaphore							semAcquireImage = VK_NULL_HANDLE;
		VkSemaphore							semPresent = VK_NULL_HANDLE;

		VkSurfaceCapabilitiesKHR			capabilities;
		std::vector<VkPresentModeKHR>		presentModes;
		std::vector<VkSurfaceFormatKHR>		formats;

		VkFormat							currentFormat;
		VkColorSpaceKHR						currentColorSpace;
		VkPresentModeKHR					currentPresentMode;
		VkExtent2D							currentExtent;

		struct Image {
			VkImage image;
			VkImageView view;
			u64 ID;
		};
		std::vector<Image> images;
	};

	struct Graphics_vk {

		VkInstance					instance = VK_NULL_HANDLE;
		VkDevice					device = VK_NULL_HANDLE;
		SwapChain					swapChain;
		VmaAllocator				allocator;

#ifdef SV_ENABLE_GFX_VALIDATION
		VkDebugUtilsMessengerEXT	debug = VK_NULL_HANDLE;
#endif

		std::vector<const char*>	extensions;
		std::vector<const char*>	validationLayers;
		std::vector<const char*>	deviceExtensions;
		std::vector<const char*>	deviceValidationLayers;

		VkQueue						queueGraphics = VK_NULL_HANDLE;

		float						lastTime = 0.f;

		// Frame Members

		std::vector<Frame>		frames;
		std::vector<VkFence>	imageFences;
		u32					frameCount;
		u32					currentFrame = 0u;
		std::mutex				mutexCMD;
		u32					activeCMDCount = 0u;

		u32					imageIndex;

		SV_INLINE Frame& GetFrame() noexcept { return frames[currentFrame]; }
		SV_INLINE VkCommandBuffer GetCMD(CommandList cmd) { return frames[currentFrame].commandBuffers[cmd]; }

		// Binding Members

		VkRenderPass						activeRenderPass[GraphicsLimit_CommandList];
		std::map<size_t, VulkanPipeline>	pipelines;
		std::mutex							pipelinesMutex;

	private:
		u64 IDCount = 0u;
		std::mutex IDMutex;
	public:
		inline u64 GetID() noexcept { std::lock_guard<std::mutex> lock(IDMutex); return IDCount++; }

	};

	// API FUNCTIONS

	Result graphics_vulkan_swapchain_create(VkSwapchainKHR oldSwapchain);
	Result graphics_vulkan_swapchain_destroy(bool resizing = true);

	void graphics_vulkan_state_update_graphics(CommandList cmd);
	VkDescriptorSet graphics_vulkan_state_update_graphics_descriptors(VulkanPipeline& pipeline, ShaderType shaderType, GraphicsState& state, bool samplers, bool images, bool uniforms, CommandList cmd);

	VkResult graphics_vulkan_singletimecmb_begin(VkCommandBuffer* pCmd);
	VkResult graphics_vulkan_singletimecmb_end(VkCommandBuffer cmd);

	void graphics_vulkan_buffer_copy(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);
	VkResult graphics_vulkan_imageview_create(VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags, u32 layerCount, VkImageView& view);

	VkSemaphore graphics_vulkan_semaphore_create();
	VkFence graphics_vulkan_fence_create(bool sign);

	void graphics_vulkan_acquire_image();
	void graphics_vulkan_submit_commandbuffers();
	void graphics_vulkan_present();

	Result graphics_vulkan_buffer_create(Buffer_vk& buffer, const GPUBufferDesc& desc);
	Result graphics_vulkan_image_create(Image_vk& image, const GPUImageDesc& desc);
	Result graphics_vulkan_sampler_create(Sampler_vk& sampler, const SamplerDesc& desc);
	Result graphics_vulkan_shader_create(Shader_vk& shader, const ShaderDesc& desc);
	Result graphics_vulkan_renderpass_create(RenderPass_vk& renderPass, const RenderPassDesc& desc);
	Result graphics_vulkan_inputlayoutstate_create(InputLayoutState_vk& inputLayoutState, const InputLayoutStateDesc& desc);
	Result graphics_vulkan_blendstate_create(BlendState_vk& blendState, const BlendStateDesc& desc);
	Result graphics_vulkan_depthstencilstate_create(DepthStencilState_vk& depthStencilState, const DepthStencilStateDesc& desc);
	Result graphics_vulkan_rasterizerstate_create(RasterizerState_vk& rasterizerState, const RasterizerStateDesc& desc);

	Result graphics_vulkan_buffer_destroy(Buffer_vk& buffer);
	Result graphics_vulkan_image_destroy(Image_vk& image);
	Result graphics_vulkan_sampler_destroy(Sampler_vk& sampler);
	Result graphics_vulkan_shader_destroy(Shader_vk& shader);
	Result graphics_vulkan_renderpass_destroy(RenderPass_vk& renderPass);

}

#endif