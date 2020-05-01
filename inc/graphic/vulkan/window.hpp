#pragma once

#include "./device.hpp"
#include <functional>

namespace cw {
	namespace graphic {
		namespace vulkan {
			using render_func_t = std::function<void(vk::CommandBuffer, vk::Rect2D)>;
			struct window_ci_t {
				const device_t* device = nullptr;
				bool vsync = true;
				bool depth_stencil = true;
				decltype(auto) set_device(device_t const* device) { this->device = device; return *this; }
				decltype(auto) set_vsync(bool const& vsync) { this->vsync = vsync; return *this; }
				decltype(auto) set_depth_stencil(bool const& depth_stencil) { this->depth_stencil = depth_stencil; return *this; }
#ifdef VK_USE_PLATFORM_WIN32_KHR
				HINSTANCE hinstance;
				HWND hwnd;
				decltype(auto) set_hinstance(HINSTANCE const& hinstance) { this->hinstance = hinstance; return *this; }
				decltype(auto) set_hwnd(HWND const& hwnd) { this->hwnd = hwnd; return *this; }
#endif
			};
			class window_t {
			public:
				window_t(window_ci_t const& ci) : m_device(ci.device), m_vsync(ci.vsync){
#ifdef VK_USE_PLATFORM_WIN32_KHR
					m_surface = vk::Instance(*m_device).createWin32SurfaceKHR(
						vk::Win32SurfaceCreateInfoKHR()
						.setHinstance(ci.hinstance)
						.setHwnd(ci.hwnd)
					);
#endif
					m_support_present_modes = func::find_surface_present_modes(*m_device, m_surface).value();
					m_support_presents = func::get_surface_supports(*m_device, m_surface);

					const auto& familys = m_device->get_queue_familys();
					if (familys.graphic.has_value() && m_support_presents[familys.graphic.value().index]) m_present_queue = vk::Device(*m_device).getQueue(familys.graphic.value().index, 0);
					else if (familys.transfer.has_value() && m_support_presents[familys.transfer.value().index]) m_present_queue = vk::Device(*m_device).getQueue(familys.transfer.value().index, 0);
					else if (familys.compute.has_value() && m_support_presents[familys.compute.value().index]) m_present_queue = vk::Device(*m_device).getQueue(familys.compute.value().index, 0);
					else if (familys.sparse_binding.has_value() && m_support_presents[familys.sparse_binding.value().index]) m_present_queue = vk::Device(*m_device).getQueue(familys.sparse_binding.value().index, 0);
					else assert(0);
					assert(m_present_queue != VK_NULL_HANDLE);

					if (familys.graphic.has_value() && m_support_presents[familys.graphic.value().index]) m_submit_queue = vk::Device(*m_device).getQueue(familys.graphic.value().index, 0);
					else if (familys.transfer.has_value() && m_support_presents[familys.transfer.value().index]) m_submit_queue = vk::Device(*m_device).getQueue(familys.transfer.value().index, 0);
					else if (familys.compute.has_value() && m_support_presents[familys.compute.value().index]) m_submit_queue = vk::Device(*m_device).getQueue(familys.compute.value().index, 0);
					else if (familys.sparse_binding.has_value() && m_support_presents[familys.sparse_binding.value().index]) m_submit_queue = vk::Device(*m_device).getQueue(familys.sparse_binding.value().index, 0);
					else assert(0);
					assert(m_submit_queue != VK_NULL_HANDLE);


					m_color.surface_format = func::find_surface_formats(*m_device, m_surface).value()[0];
					if (ci.depth_stencil) m_depth_stencil = depth_stencil_t();
					if (m_depth_stencil.has_value()) m_depth_stencil.value().format = func::find_depth_stencil_formats(*m_device).value()[0];

					std::vector<vk::AttachmentDescription> attachments;
					// Color attachment
					attachments.push_back(
						vk::AttachmentDescription()
						.setFormat(m_color.surface_format.format)
						.setSamples(vk::SampleCountFlagBits::e1)
						.setLoadOp(vk::AttachmentLoadOp::eClear)
						.setStoreOp(vk::AttachmentStoreOp::eStore)
						.setInitialLayout(vk::ImageLayout::eUndefined)
						.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
					);
					// Depth attachment
					if (m_depth_stencil.has_value()) {
						attachments.push_back(
							vk::AttachmentDescription()
							.setFormat(m_depth_stencil.value().format)
							.setSamples(vk::SampleCountFlagBits::e1)
							.setLoadOp(vk::AttachmentLoadOp::eClear)
							.setStoreOp(vk::AttachmentStoreOp::eStore)
							.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
							.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
							.setInitialLayout(vk::ImageLayout::eUndefined)
							.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
						);
					}

					auto colorReference =
						vk::AttachmentReference()
						.setAttachment(0)
						.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
					std::optional<vk::AttachmentReference> depthReference;

					auto subpassDescription =
						vk::SubpassDescription()
						.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
						.setColorAttachmentCount(1)
						.setPColorAttachments(&colorReference)
						.setInputAttachmentCount(0)
						.setPInputAttachments(nullptr)
						.setPreserveAttachmentCount(0)
						.setPPreserveAttachments(nullptr)
						.setPResolveAttachments(nullptr);
					if (m_depth_stencil.has_value()) {
						depthReference =
							vk::AttachmentReference()
							.setAttachment(1)
							.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
						subpassDescription.setPDepthStencilAttachment(&depthReference.value());
					}
					// Subpass dependencies for layout transitions
					std::array<vk::SubpassDependency, 2> dependencies{
						vk::SubpassDependency()
						.setSrcSubpass(VK_SUBPASS_EXTERNAL)
						.setDstSubpass(0)
						.setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
						.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
						.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
						.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
						.setDependencyFlags(vk::DependencyFlagBits::eByRegion),
						vk::SubpassDependency()
						.setSrcSubpass(0)
						.setDstSubpass(VK_SUBPASS_EXTERNAL)
						.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
						.setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
						.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
						.setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
						.setDependencyFlags(vk::DependencyFlagBits::eByRegion)
					};

					m_render_pass = vk::Device(*m_device).createRenderPass(
						vk::RenderPassCreateInfo()
						.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
						.setPAttachments(attachments.data())
						.setSubpassCount(1)
						.setPSubpasses(&subpassDescription)
						.setDependencyCount(static_cast<uint32_t>(dependencies.size()))
						.setPDependencies(dependencies.data())
					);

					m_sync.present_available = vk::Device(*m_device).createSemaphore(vk::SemaphoreCreateInfo());
					m_sync.render_finish = vk::Device(*m_device).createSemaphore(vk::SemaphoreCreateInfo());
					m_submit_info = vk::SubmitInfo()
						.setWaitSemaphoreCount(1)
						.setPWaitSemaphores(&m_sync.present_available)
						.setSignalSemaphoreCount(1)
						.setPSignalSemaphores(&m_sync.render_finish)
						.setCommandBufferCount(1)
						.setPWaitDstStageMask(&m_wait_stage);

					m_command_pool = vk::Device(*m_device).createCommandPool(
						vk::CommandPoolCreateInfo()
						.setQueueFamilyIndex(m_device->get_queue_familys().graphic.value().index)
						.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
					);
					

					build(m_vsync);
				}
				~window_t() {
					clean();
					
					vk::Device(*m_device).destroyCommandPool(m_command_pool);
					vk::Device(*m_device).destroySemaphore(m_sync.render_finish);
					vk::Device(*m_device).destroySemaphore(m_sync.present_available);

					vk::Device(*m_device).destroyRenderPass(m_render_pass);
					vk::Instance(*m_device).destroySurfaceKHR(m_surface);
				}
				void rebuild(bool vsync = true) {
					clean();
					build(vsync);
				}
				void run(bool check_active = true) {
					if (check_active && !is_active()) return;
					auto result = acquire_next_image(m_sync.present_available);
					if (result.first) rebuild(m_vsync);
					m_sync.current_index = result.second;
					m_submit_info.setPCommandBuffers(&m_execute_cmds[m_sync.current_index]);
					m_submit_queue.submit({ m_submit_info }, nullptr);
					if(present(m_sync.current_index, m_sync.render_finish)) rebuild(m_vsync);
				}
				//decltype(auto) build_default_cmds() {
				//	m_default_cmds = vk::Device(*m_device).allocateCommandBuffers(
				//		vk::CommandBufferAllocateInfo()
				//		.setCommandPool(m_command_pool)
				//		.setLevel(vk::CommandBufferLevel::ePrimary)
				//		.setCommandBufferCount(m_color.images.size())
				//	);
				//	vk::ClearColorValue defaultClearColor = vk::ClearColorValue().setFloat32({ 0.2f, 0.3f, 0.3f, 1.0f });
				//	vk::ClearValue clearValues[2];
				//	clearValues[0].color = defaultClearColor;
				//	clearValues[1].depthStencil = { 1.0f, 0 };
				//	for (int i = 0; i < m_default_cmds.size(); ++i) {
				//		m_default_cmds[i].begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
				//		m_default_cmds[i].beginRenderPass(
				//			vk::RenderPassBeginInfo()
				//			.setRenderPass(m_render_pass)
				//			.setFramebuffer(m_framebuffers[i])
				//			.setClearValueCount(2)
				//			.setPClearValues(clearValues)
				//			.setRenderArea({ {0,0},m_last_extent })
				//			, vk::SubpassContents::eInline
				//		);
				//		m_default_cmds[i].endRenderPass();
				//		m_default_cmds[i].end();
				//	}
				//	return m_default_cmds;
				//}
				//void clean_default_cmds() {
				//	vk::Device(*m_device).waitIdle();
				//	vk::Device(*m_device).freeCommandBuffers(m_command_pool, m_default_cmds);
				//	m_default_cmds.clear();
				//}
				void caculate(std::vector<render_func_t> const& render_funcs, bool store = true) {
					m_render_funcs = render_funcs;
					vk::Rect2D area = { {0,0},m_last_extent };
					for (auto& iter : m_default_cmds) iter.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
					for (int i = 0; i < m_default_cmds.size(); ++i) {
						m_default_cmds[i].begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
						m_default_cmds[i].beginRenderPass(
							vk::RenderPassBeginInfo()
							.setRenderPass(m_render_pass)
							.setFramebuffer(m_framebuffers[i])
							.setClearValueCount(m_clear_values.size())
							.setPClearValues(m_clear_values.data())
							.setRenderArea(area)
							, vk::SubpassContents::eInline
						);
						for (const auto& func : m_render_funcs) func(m_default_cmds[i], area);
						//if (m_render_func != nullptr) m_render_func(m_default_cmds[i], area);
						m_default_cmds[i].endRenderPass();
						m_default_cmds[i].end();
					}
					if (!store) m_render_funcs.clear();
					//if (!store) m_render_func = nullptr;
					m_execute_cmds = m_default_cmds;
				}
				decltype(auto) extent() const { return func::get_surface_extent(*m_device, m_surface); }

				operator vk::RenderPass() const { return m_render_pass; }
			private:
				void build(bool vsync = true) {
					m_vsync = vsync;
					m_last_extent = func::get_surface_extent(*m_device, m_surface);
					vk::SwapchainKHR old_swapchain = m_swapchain;
					m_swapchain = vk::Device(*m_device).createSwapchainKHR(
						vk::SwapchainCreateInfoKHR()
						.setSurface(m_surface)
						.setMinImageCount(func::find_surface_image_count(*m_device, m_surface))
						.setImageFormat(m_color.surface_format.format)
						.setImageColorSpace(m_color.surface_format.colorSpace)
						.setImageExtent(m_last_extent)
						.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | func::find_surface_image_usage(*m_device, m_surface))
						.setPreTransform(func::find_surface_transforms(*m_device, m_surface).value()[0])
						.setImageArrayLayers(1)
						.setImageSharingMode(vk::SharingMode::eExclusive)
						.setQueueFamilyIndexCount(0)
						.setPQueueFamilyIndices(nullptr)
						.setPresentMode(m_vsync ? m_support_present_modes.front() : m_support_present_modes.back())
						.setClipped(VK_TRUE)
						.setCompositeAlpha(func::find_surface_composite_alphas(*m_device, m_surface).value()[0])
						.setOldSwapchain(old_swapchain)
					);
					assert(m_color.images.empty());
					m_color.images = vk::Device(*m_device).getSwapchainImagesKHR(m_swapchain);
					for (const auto& iter : m_color.images) {
						m_color.views.push_back(
							vk::Device(*m_device).createImageView(
								vk::ImageViewCreateInfo()
								.setFormat(m_color.surface_format.format)
								.setImage(iter)
								.setComponents(
									vk::ComponentMapping()
									.setR(vk::ComponentSwizzle::eR)
									.setG(vk::ComponentSwizzle::eG)
									.setB(vk::ComponentSwizzle::eB)
									.setA(vk::ComponentSwizzle::eA))
								.setSubresourceRange(
									vk::ImageSubresourceRange()
									.setAspectMask(vk::ImageAspectFlagBits::eColor)
									.setBaseMipLevel(0)
									.setLevelCount(1)
									.setBaseArrayLayer(0)
									.setLayerCount(1))
								.setViewType(vk::ImageViewType::e2D))
						);
					}
					if (m_depth_stencil.has_value()) {
						m_depth_stencil.value().image = vk::Device(*m_device).createImage(
							vk::ImageCreateInfo()
							.setImageType(vk::ImageType::e2D)
							.setFormat(m_depth_stencil.value().format)
							.setExtent({ m_last_extent.width, m_last_extent.height, 1 })
							.setMipLevels(1)
							.setArrayLayers(1)
							.setSamples(vk::SampleCountFlagBits::e1)
							.setTiling(vk::ImageTiling::eOptimal)
							.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc)
						);
						auto memReqs = vk::Device(*m_device).getImageMemoryRequirements(m_depth_stencil.value().image);
						m_depth_stencil.value().memory = vk::Device(*m_device).allocateMemory(
							vk::MemoryAllocateInfo()
							.setAllocationSize(memReqs.size)
							.setMemoryTypeIndex(func::find_memory_index(*m_device, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal).value())
						);
						vk::Device(*m_device).bindImageMemory(m_depth_stencil.value().image, m_depth_stencil.value().memory, 0);
						auto imageViewCI =
							vk::ImageViewCreateInfo()
							.setViewType(vk::ImageViewType::e2D)
							.setImage(m_depth_stencil.value().image)
							.setFormat(m_depth_stencil.value().format)
							.setSubresourceRange(
								vk::ImageSubresourceRange()
								.setBaseMipLevel(0)
								.setLevelCount(1)
								.setBaseArrayLayer(0)
								.setLayerCount(1)
								.setAspectMask(vk::ImageAspectFlagBits::eDepth)
							);
						// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
						if (m_depth_stencil.value().format >= vk::Format::eD16UnormS8Uint) {
							imageViewCI.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
						}
						m_depth_stencil.value().view = vk::Device(*m_device).createImageView(imageViewCI);
					}

					std::vector<vk::ImageView> attachments;
					if (m_depth_stencil.has_value()) { attachments.resize(2); attachments[1] = m_depth_stencil.value().view; }
					for (const auto& iter : m_color.views) {
						attachments[0] = iter;
						m_framebuffers.push_back(
							vk::Device(*m_device).createFramebuffer(
								vk::FramebufferCreateInfo()
								.setRenderPass(m_render_pass)
								.setAttachmentCount(attachments.size())
								.setPAttachments(attachments.data())
								.setWidth(m_last_extent.width)
								.setHeight(m_last_extent.height)
								.setLayers(1))
						);
					}
					
					//m_execute_cmds = build_default_cmds();
					m_default_cmds = vk::Device(*m_device).allocateCommandBuffers(
						vk::CommandBufferAllocateInfo()
						.setCommandPool(m_command_pool)
						.setLevel(vk::CommandBufferLevel::ePrimary)
						.setCommandBufferCount(m_color.images.size())
					);
					caculate(m_render_funcs, true);
				}
				void clean() {
					//clean_default_cmds();
					vk::Device(*m_device).waitIdle();
					vk::Device(*m_device).freeCommandBuffers(m_command_pool, m_default_cmds);
					m_default_cmds.clear();
					
					m_execute_cmds.clear();

					for (const auto& iter : m_framebuffers) vk::Device(*m_device).destroyFramebuffer(iter);
					m_framebuffers.clear();
					if (m_depth_stencil.has_value()) {
						vk::Device(*m_device).destroyImageView(m_depth_stencil.value().view);
						vk::Device(*m_device).freeMemory(m_depth_stencil.value().memory);
						vk::Device(*m_device).destroyImage(m_depth_stencil.value().image);
					}
					for (auto& iter : m_color.views) vk::Device(*m_device).destroyImageView(iter);
					m_color.images.clear();
					m_color.views.clear();
					vk::Device(*m_device).destroySwapchainKHR(m_swapchain);
					m_swapchain = nullptr;
				}

				/*decltype(auto)*/ bool is_active() {
					auto new_extent = vk::PhysicalDevice(*m_device).getSurfaceCapabilitiesKHR(m_surface).currentExtent;
					if (new_extent.width == 0 || new_extent.height == 0) { m_last_extent = new_extent; return false; }
					if (new_extent == m_last_extent) return true;
					m_last_extent = new_extent;
					rebuild(m_vsync);
					return true;
				}
				/*decltype(auto)*/ std::pair<bool, std::uint32_t> acquire_next_image(vk::Semaphore signal) const {
					std::pair<bool, std::uint32_t> result{ false, std::numeric_limits<std::uint32_t>::max() };
					auto temp = vk::Device(*m_device).acquireNextImageKHR(m_swapchain, UINT64_MAX, signal, nullptr);
					if ((temp.result == vk::Result::eErrorOutOfDateKHR) || (temp.result == vk::Result::eSuboptimalKHR)) result.first = true;
					else assert(temp.result == vk::Result::eSuccess);
					result.second = temp.value;
					return result;
				}
				/*decltype(auto)*/ bool present(uint32_t imageIndex, vk::Semaphore wait_semaphore = nullptr) const {
					auto present_info =
						vk::PresentInfoKHR()
						.setSwapchainCount(1)
						.setPSwapchains(&m_swapchain)
						.setPImageIndices(&imageIndex);
					if (wait_semaphore != VK_NULL_HANDLE) {
						present_info.waitSemaphoreCount = 1;
						present_info.pWaitSemaphores = &wait_semaphore;
					}
					auto result = m_present_queue.presentKHR(&present_info);
					if (!((result == vk::Result::eSuccess) || (result == vk::Result::eSuboptimalKHR))) {
						if (result == vk::Result::eErrorOutOfDateKHR) return true;
						else assert(result == vk::Result::eSuccess);
					}
					m_present_queue.waitIdle();
					return false;
				}
			private:
				const device_t* m_device = nullptr;
				vk::SurfaceKHR m_surface;
				bool m_vsync = true;
				std::vector<vk::PresentModeKHR> m_support_present_modes;
				std::vector<vk::Bool32> m_support_presents;
				struct color_t {
					vk::SurfaceFormatKHR surface_format;
					std::vector<vk::Image> images;
					std::vector<vk::ImageView> views;
				}m_color;
				struct depth_stencil_t {
					vk::Format format;
					vk::Image image;
					vk::DeviceMemory memory;
					vk::ImageView view;
				};
				std::optional<depth_stencil_t> m_depth_stencil;
				vk::RenderPass m_render_pass;
				vk::SwapchainKHR m_swapchain;
				std::vector<vk::Framebuffer> m_framebuffers;
				vk::Extent2D m_last_extent;

				struct {
					vk::Semaphore present_available;
					vk::Semaphore render_finish;
					std::uint32_t current_index;
				}m_sync;
				vk::Queue m_present_queue;
				vk::Queue m_submit_queue;

				vk::SubmitInfo m_submit_info;
				vk::PipelineStageFlags m_wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				vk::CommandPool m_command_pool;
				std::vector<vk::CommandBuffer> m_default_cmds;
				std::vector<vk::CommandBuffer> m_execute_cmds;
				std::vector<render_func_t> m_render_funcs;
				std::vector<vk::ClearValue> m_clear_values{
					vk::ClearValue().setColor(vk::ClearColorValue().setFloat32({ 0.2f, 0.3f, 0.3f, 1.0f })),
					vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue().setDepth(1.0f).setStencil(0))
				};
			};
		}
	}
}