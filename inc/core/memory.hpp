#pragma once

#include "./integer.hpp"
#include <optional>
#include <vector>
#include <cassert>
#include <cstring>

namespace cw {
	namespace core {
		class memory_view_t {
		public:
			memory_view_t(ull_t byte = 0, void* data = nullptr) : m_byte(byte), m_data(data) {}
			template<typename _type> memory_view_t(std::vector<_type>& vector) : memory_view_t(sizeof(_type)* vector.size(), static_cast<void*>(vector.data())) {}
			template<typename _type> memory_view_t(_type& other) : memory_view_t(sizeof(_type), static_cast<void*>(&other)) {}
			decltype(auto) set_byte(ull_t byte) { m_byte = byte; return *this; }
			decltype(auto) set_data(void* data) { m_data = data; return *this; }
			//decltype(auto) set_data(ull_t offset, ull_t byte, void* data) {}
			decltype(auto) is_byte(ull_t byte, bool is_index = true) const { return is_index ? byte < m_byte : byte <= m_byte; }
			decltype(auto) is_alignment(ull_t alignment) const { return (m_byte % alignment) == 0; }
			decltype(auto) is_index(ull_t alignment, ull_t index) const { return is_alignment(alignment) && is_byte(index * alignment); }
			decltype(auto) reset(ull_t byte = 0, void* data = nullptr) { m_byte = byte; m_data = data; return *this; }
			template<typename _type> decltype(auto) reset(std::vector<_type>& vector) { m_byte = sizeof(_type) * vector.size(); m_data = static_cast<void*>(vector.data()); return *this; }
			decltype(auto) byte() const { return m_byte; }
			decltype(auto) size(ull_t alignment) const { assert(is_alignment(alignment)); return m_byte / alignment; }
			decltype(auto) data() { return m_data; }
			decltype(auto) data() const { return (const void*)m_data; }
			decltype(auto) at(ull_t byte) { assert(is_byte(byte)); return static_cast<void*>(static_cast<byte_t*>(m_data) + byte); }
			decltype(auto) at(ull_t byte) const { assert(is_byte(byte)); return static_cast<const void*>(static_cast<byte_t*>(m_data) + byte); }
			decltype(auto) at(ull_t alignment, ull_t index) { assert(is_index(alignment, index)); return at(alignment * index); }
			decltype(auto) at(ull_t alignment, ull_t index) const { assert(is_index(alignment, index)); return at(alignment * index); }
			template<typename _type> decltype(auto) at(ull_t index = 0) { assert(is_index(sizeof(_type), index)); return static_cast<_type*>(at(sizeof(_type) * index)); }
			template<typename _type> decltype(auto) at(ull_t index = 0) const { assert(is_index(sizeof(_type), index)); return static_cast<const _type*>(at(sizeof(_type) * index)); }
			template<typename _type> decltype(auto) ref(ull_t index = 0) { return *at<_type>(index); }
			template<typename _type> decltype(auto) ref(ull_t index = 0) const { return *at<_type>(index); }
			decltype(auto) sub_view(ull_t offset, ull_t byte) { assert(offset + byte <= m_byte); return memory_view_t(byte, at(offset)); }
			decltype(auto) sub_view(ull_t offset, ull_t byte) const { assert(offset + byte <= m_byte); return (const memory_view_t(byte, const_cast<void*>(at(offset)))); }
			decltype(auto) copy_from(ull_t byte, void* data, ull_t offset = 0) { assert(is_byte(offset + byte, false)); memcpy(static_cast<void*>(static_cast<byte_t*>(m_data) + offset), data, byte); return *this; }
			decltype(auto) copy_from(memory_view_t const& view, ull_t offset = 0) { assert(is_byte(offset + view.m_byte, false)); memcpy(static_cast<void*>(static_cast<byte_t*>(m_data) + offset), view.m_data, view.m_byte); return *this; }
			//decltype(auto) at_byte(ull_t byte) { assert(is_byte(byte)); return static_cast<void*>(static_cast<byte_t*>(m_data) + byte); }
			//decltype(auto) at_byte(ull_t byte) const { assert(is_byte(byte)); return static_cast<const void*>(static_cast<byte_t*>(m_data) + byte); }
			//decltype(auto) sub_view(ull_t byte_offset, ull_t byte_size) { assert(byte_offset + byte_size <= m_byte); return memory_view_t(byte_size, at_byte(byte_offset)); }
			//decltype(auto) sub_view(ull_t byte_offset, ull_t byte_size) const { assert(byte_offset + byte_size <= m_byte); return const memory_view_t(byte_size, const_cast<void*>(at_byte(byte_offset))); }
			//decltype(auto) at(ull_t index) { assert(is_valid_index(index)); return at_byte(index * m_alignment); }
			//decltype(auto) at(ull_t index) const { assert(is_valid_index(index)); return at_byte(index * m_alignment); }
		private:
			ull_t m_byte;
			void* m_data;
		};
		class memory_t {
		public:
			memory_t(ull_t byte = 0, void* data = nullptr) : m_byte(byte), m_data(byte != 0 ? (data ? memcpy(new byte_t[byte], data, byte) : memset(new byte_t[byte], 0, byte)) : nullptr) {}
			memory_t(memory_view_t& view) : memory_t(view.byte(), view.data()) {}
			template<typename _type> memory_t(std::vector<_type>& vector) : memory_t(sizeof(_type)* vector.size(), static_cast<void*>(vector.data())) {}
			~memory_t() { clean(); }
			decltype(auto) reset(ull_t byte = 0, void* data = nullptr) { clean(); m_byte = byte; m_data = (byte != 0 ? (data ? memcpy(new byte_t[byte], data, byte) : memset(new byte_t[byte], 0, byte)) : nullptr); return *this; }
			decltype(auto) rebyte(ull_t byte, bool clean = true) {
				auto old_byte = m_byte;
				auto old_data = m_data;
				m_byte = byte;
				if (m_byte != 0) {
					m_data = memset(new byte_t[m_byte], 0, m_byte);
					if (!clean) memcpy(m_data, old_data, old_byte > m_byte ? m_byte : old_byte);
				}
				else m_data = nullptr;
				delete old_data;
				return *this;
			}
			decltype(auto) resize(ull_t alignment, ull_t size, bool clean = true) { return rebyte(alignment * size, clean); }
			template<typename _type> decltype(auto) resize(ull_t size, bool clean = true) { return rebyte(sizeof(_type) * size, clean); }
			decltype(auto) byte() const { return m_byte; }
			decltype(auto) data() { return m_data; }
			decltype(auto) data() const { return (const void*)m_data; }
			decltype(auto) view(ull_t offset = 0, std::optional<ull_t> byte = std::nullopt) { return memory_view_t(valid(offset, byte), static_cast<void*>(static_cast<byte_t*>(m_data) + offset)); }
			decltype(auto) view(ull_t offset = 0, std::optional<ull_t> byte = std::nullopt) const { return (const memory_view_t(valid(offset, byte), static_cast<void*>(static_cast<byte_t*>(m_data) + offset))); }
			//decltype(auto) view(ull_t offset = 0, std::optional<ull_t> byte = std::nullopt) {
			//	/*ull_t byte_size = byte.has_value() ? byte.value() : m_byte - offset;
			//	assert(offset + byte_size <= m_byte);*/
			//	return memory_view_t(valid(offset, byte), static_cast<void*>(static_cast<byte_t*>(m_data) + offset));
			//}
			//const auto view(ull_t offset = 0, std::optional<ull_t> byte = std::nullopt) const {
			//	/*ull_t byte_size = byte.has_value() ? byte.value() : m_byte - offset;
			//	assert(offset + byte_size <= m_byte);*/
			//	return memory_view_t(valid(offset, byte), static_cast<void*>(static_cast<byte_t*>(m_data) + offset));
			//}
			//decltype(auto) set_byte(ull_t byte) { m_byte = byte; return *this; }
			//decltype(auto) set_data(void* data) { m_data = data; return *this; }
		private:
			ull_t valid(ull_t offset = 0, std::optional<ull_t> byte = std::nullopt) const {
				ull_t byte_size = byte.has_value() ? byte.value() : m_byte - offset;
				assert(offset + byte_size <= m_byte);
				return byte_size;
			}
			void clean() { m_byte = 0; if (m_data) { delete m_data; m_data = nullptr; } }
		private:
			ull_t m_byte;
			void* m_data;
		};
		//class memory_t {
		//public:
		//	memory_t() : m_alignment(0), m_size(0), m_data(nullptr) {}
		//	memory_t(ull_t alignment, ull_t size, s32_t init_value = 0) : m_alignment(alignment), m_size(size), m_data(memset(new byte_t[m_alignment * m_size], init_value, m_alignment* m_size)) { }
		//	~memory_t() { if (m_data) delete m_data; }
		//	//decltype(auto) rebyte(ull_t byte, s32_t init_value = 0, bool clean = true) {
		//	//	auto old_byte = m_alignment * m_size;
		//	//
		//	//	return *this;
		//	//}
		//	decltype(auto) reset(ull_t alignment, ull_t size, s32_t init_value = 0) {
		//		m_alignment = alignment;
		//		m_size = size;
		//		auto new_byte = m_alignment * m_size;
		//		void* new_data = memset(new byte_t[new_byte], init_value, new_byte);
		//		//if (!clean) {
		//		//	if (new_byte > old_byte) memset(static_cast<void*>(static_cast<byte_t*>(new_data) + old_byte), 0, new_byte - old_byte);
		//		//	memcpy(new_data, m_data, old_byte);
		//		//}
		//		delete m_data;
		//		m_data = new_data;
		//		return *this;
		//	}
		//	decltype(auto) resize(ull_t size, s32_t init_value = 0, bool clean = true) {
		//		auto old_byte = m_alignment * m_size;
		//		m_size = size;
		//		auto new_byte = m_alignment * m_size;
		//		void* new_data = memset(new byte_t[new_byte], init_value, new_byte);
		//		//if (new_byte > old_byte) memset(static_cast<void*>(static_cast<byte_t*>(new_data) + old_byte), 0, new_byte - old_byte);
		//		if (!clean) memcpy(new_data, m_data, old_byte > new_byte ? new_byte : old_byte);
		//		delete m_data;
		//		m_data = new_data;
		//		return *this;
		//	}
		//	decltype(auto) realignment(ull_t alignment) {
		//
		//	}
		//	decltype(auto) set_data(ull_t byte_size, void const* data, ull_t byte_offset = 0) {
		//		assert(byte_size + byte_offset <= m_size * m_alignment);
		//		memcpy(static_cast<void*>(static_cast<byte_t*>(m_data) + byte_offset), data, byte_size);
		//		return *this;
		//	}
		//	decltype(auto) alignment() const { return m_alignment; }
		//	decltype(auto) byte() const { return m_alignment * m_size; }
		//	decltype(auto) size() const { return m_size; }
		//	decltype(auto) data() { return m_data; }
		//	decltype(auto) data() const { return (const void*)m_data; }
		//private:
		//	ull_t m_alignment;
		//	ull_t m_size;
		//	void* m_data;
		//};

	}
}