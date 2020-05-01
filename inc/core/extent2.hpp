#pragma once

#include "./priv/inner_extent.hpp"
#include "./vec2.hpp"

namespace cw {
	namespace core {
		namespace priv {
			template<typename _type>
			struct inner_extent_t<_type, 2> : public inner_vec_t<_type, 2> {
				using type = _type;
				using vec_type = inner_vec_t<type, 2>;

				constexpr inner_extent_t() noexcept : vec_type{} {}
				constexpr inner_extent_t(const type& width, const type& height) noexcept : vec_type{ width, height } {}
				constexpr inner_extent_t(const type& other) noexcept : vec_type{ other, other } {}
				constexpr inner_extent_t(const inner_extent_t& other) noexcept : vec_type{ other.m_vec[0], other.m_vec[1] } {}

				constexpr inline decltype(auto) set_width(const type& width) noexcept { vec_type::m_vec[0] = width; return *this; }
				constexpr inline decltype(auto) set_height(const type& height) noexcept { vec_type::m_vec[1] = height; return *this; }

				constexpr inline decltype(auto) width() noexcept { return vec_type::m_vec[0]; }
				constexpr inline decltype(auto) height() noexcept { return vec_type::m_vec[1]; }
				constexpr inline decltype(auto) width() const noexcept { return vec_type::m_vec[0]; }
				constexpr inline decltype(auto) height() const noexcept { return vec_type::m_vec[1]; }

				//constexpr decltype(auto) to_rvec() { return rvec2_t<_type&>{vec_t<_type, 2, true>::m_proxy0, vec_t<_type, 2, true>::m_proxy1}; }
			};
		}
		template<typename _type>
		using extent2_t = priv::inner_extent_t<_type, 2>;
	}
}