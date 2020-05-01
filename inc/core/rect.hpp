#pragma once

//#include "./offset2.hpp"
//#include "./extent2.hpp"

namespace cw {
	namespace core {
		template<typename _offset_type, typename _extent_type>
		struct rect_t {
			using offset_type = _offset_type;
			using extent_type = _extent_type;

			constexpr rect_t() : m_offset{}, m_extent{} {}
			constexpr rect_t(const offset_type& offset, const extent_type& extent) : m_offset{ offset }, m_extent{ extent } {}

			constexpr inline decltype(auto) set_offset(const offset_type& offset) noexcept { m_offset = offset; return *this; }
			constexpr inline decltype(auto) set_extent(const extent_type& extent) noexcept { m_extent = extent; return *this; }

		public:
			offset_type m_offset;
			extent_type m_extent;
		};

		/*template<typename _offset_type, typename _extent_type>
		struct rect_t {
			using offset_type = priv::inner_offset_t<_offset_type, 2>;
			using extent_type = priv::inner_extent_t<_extent_type, 2>;

			constexpr rect_t() : m_offset{}, m_extent{} {}
			constexpr rect_t(const offset_type& offset, const extent_type& extent) : m_offset{ offset }, m_extent{ extent } {}

			constexpr inline decltype(auto) set_offset(const offset_type& offset) noexcept { m_offset = offset; return *this; }
			constexpr inline decltype(auto) set_extent(const extent_type& extent) noexcept { m_extent = extent; return *this; }

		public:
			offset_type m_offset;
			extent_type m_extent;
		};*/

	}
}