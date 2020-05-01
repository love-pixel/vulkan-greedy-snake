#pragma once

#include "./device.hpp"
#include "./../../core/memory.hpp"

namespace cw {
	namespace graphic {
		namespace vulkan {
			struct buffer_ci_t {
				const device_t* device = nullptr;
				vk::BufferUsageFlags usage_flags;
				vk::MemoryPropertyFlags memory_flags;
				vk::DeviceSize byte = 0;
				std::optional<core::memory_view_t> memory_view;
				decltype(auto) set_device(device_t const* device) { this->device = device; return *this; }
				decltype(auto) set_usage_flags(vk::BufferUsageFlags const& usage_flags) { this->usage_flags = usage_flags; return *this; }
				decltype(auto) set_memory_flags(vk::MemoryPropertyFlags const& memory_flags) { this->memory_flags = memory_flags; return *this; }
				decltype(auto) set_byte(vk::DeviceSize const& byte) { this->byte = byte; return *this; }
				decltype(auto) set_memory_view(core::memory_view_t const& memory_view, bool set_byte = true) { this->memory_view = memory_view; if(set_byte) this->byte = memory_view.byte(); return *this; }
			};
			class buffer_t {
			private:
				const device_t* m_device = nullptr;
				vk::BufferUsageFlags m_usage_flags;
				vk::MemoryPropertyFlags m_memory_flags;
				vk::DeviceSize m_byte = 0;

				vk::Buffer m_buffer;
				vk::DeviceMemory m_memory;
			public:
				decltype(auto) byte() const { return m_byte; }
				
				decltype(auto) map(std::optional<vk::DeviceSize> const& byte = std::nullopt, vk::DeviceSize const& offset = 0) const {
					auto mapped_size = byte.has_value() ? byte.value() : m_byte;
					assert((offset + mapped_size) <= m_byte);
					return (const void*)vk::Device(*m_device).mapMemory(m_memory, offset, mapped_size, vk::MemoryMapFlags());
				}
				decltype(auto) map(std::optional<vk::DeviceSize> const& byte = std::nullopt, vk::DeviceSize const& offset = 0) {
					auto mapped_size = byte.has_value() ? byte.value() : m_byte;
					assert((offset + mapped_size) <= m_byte);
					return (void*)vk::Device(*m_device).mapMemory(m_memory, offset, mapped_size, vk::MemoryMapFlags());
				}
				decltype(auto) unmap() const { vk::Device(*m_device).unmapMemory(m_memory); return *this; }
				decltype(auto) unmap() { vk::Device(*m_device).unmapMemory(m_memory); return *this; }
				decltype(auto) copy_from(core::memory_view_t const& memory_view, std::optional<vk::DeviceSize> const& byte = std::nullopt, vk::DeviceSize const& memory_offset = 0, vk::DeviceSize const& offset = 0) {
					auto copy_byte = byte.has_value() ? byte.value() : memory_view.byte();
					assert((offset + copy_byte) <= m_byte && (memory_offset + copy_byte) <= memory_view.byte());
					auto mapped = map(copy_byte, offset);
					assert(mapped);
					memcpy(mapped, memory_view.at(memory_offset), copy_byte);
					if ((m_memory_flags & vk::MemoryPropertyFlagBits::eHostCoherent) == vk::MemoryPropertyFlags()) {
						vk::Device(*m_device).flushMappedMemoryRanges({
							vk::MappedMemoryRange()
							.setMemory(m_memory)
							.setOffset(offset)
							.setSize(copy_byte) }
						);
					}
					return unmap();
				}
				decltype(auto) copy_from(buffer_t const& other, std::optional<vk::DeviceSize> const& byte = std::nullopt, vk::DeviceSize const& other_offset = 0, vk::DeviceSize const& offset = 0) {
					auto copy_byte = byte.has_value() ? byte.value() : other.m_byte;
					assert((offset + copy_byte) <= m_byte && (other_offset + copy_byte) <= other.m_byte);
					auto copy_cmd = m_device->begin_single_command(/*vk::QueueFlagBits::eTransfer*/);
					copy_cmd.first.copyBuffer(other.m_buffer, m_buffer, {
						vk::BufferCopy()
						.setSrcOffset(other_offset)
						.setDstOffset(offset)
						.setSize(copy_byte) }
					);
					m_device->end_single_command(copy_cmd);
					return *this;
				}
				decltype(auto) copy(std::optional<vk::BufferUsageFlags> const& usage_flags, std::optional<vk::MemoryPropertyFlags> const& memory_flags, std::optional<vk::DeviceSize> const& byte = std::nullopt, vk::DeviceSize const& offset = 0) const {
					auto copy_byte = byte.has_value() ? byte.value() : m_byte;
					auto result = buffer_t(
						buffer_ci_t()
						.set_device(m_device)
						.set_usage_flags(usage_flags.has_value() ? usage_flags.value() : m_usage_flags)
						.set_memory_flags(memory_flags.has_value() ? memory_flags.value() : m_memory_flags)
						.set_byte(copy_byte)
					);
					result.copy_from(*this, copy_byte, offset);
					return result;
				}

				decltype(auto) operator=(buffer_t&& other) noexcept {
					std::swap(m_device, other.m_device);
					std::swap(m_usage_flags, other.m_usage_flags);
					std::swap(m_memory_flags, other.m_memory_flags);
					std::swap(m_byte, other.m_byte);
					std::swap(m_buffer, other.m_buffer);
					std::swap(m_memory, other.m_memory);
					return *this;
				}

				decltype(auto) operator=(std::nullptr_t) noexcept {
					this->~buffer_t();
					return *this;
				}

				operator const device_t* () const { return m_device; }
				operator vk::Buffer() const { return m_buffer; }
				operator vk::DeviceMemory() const { return m_memory; }
			public:
				buffer_t() = default;
				buffer_t(buffer_t const&) = delete;
				buffer_t(buffer_t&& other) noexcept {
					std::swap(m_device, other.m_device);
					std::swap(m_usage_flags, other.m_usage_flags);
					std::swap(m_memory_flags, other.m_memory_flags);
					std::swap(m_byte, other.m_byte);
					std::swap(m_buffer, other.m_buffer);
					std::swap(m_memory, other.m_memory);
				}
				buffer_t(buffer_ci_t const& ci) : m_device(ci.device), m_usage_flags(ci.usage_flags), m_memory_flags(ci.memory_flags), m_byte(ci.byte) {
					assert(m_device);
					m_buffer = vk::Device(*m_device).createBuffer(
						vk::BufferCreateInfo()
						.setUsage(m_usage_flags)
						.setSize(m_byte)
						.setSharingMode(vk::SharingMode::eExclusive)
					);

					auto memory_requirement = vk::Device(*m_device).getBufferMemoryRequirements(m_buffer);
					m_memory = vk::Device(*m_device).allocateMemory(
						vk::MemoryAllocateInfo()
						.setAllocationSize(memory_requirement.size)
						.setMemoryTypeIndex(m_device->find_memory_index(memory_requirement.memoryTypeBits, m_memory_flags))
					);
					vk::Device(*m_device).bindBufferMemory(m_buffer, m_memory, 0);

					if (ci.memory_view.has_value()) copy_from(ci.memory_view.value());
				}
				~buffer_t() {
					if (m_device) {
						vk::Device(*m_device).freeMemory(m_memory);
						vk::Device(*m_device).destroyBuffer(m_buffer);
						m_memory = nullptr;
						m_buffer = nullptr;
						m_device = nullptr;
					}
				}
			};
		}
	}
}