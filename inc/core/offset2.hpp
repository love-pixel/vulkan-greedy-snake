#pragma once

#include "./priv/inner_offset.hpp"
#include "./vec2.hpp"

namespace cw {
	namespace core {
		namespace priv {
			template<typename _type>
			struct inner_offset_t<_type, 2> : public inner_vec_t<_type, 2> {
				using type = _type;
				using vec_type = inner_vec_t<type, 2>;

				constexpr inner_offset_t() noexcept : vec_type{} {}
				constexpr inner_offset_t(const type& x, const type& y) noexcept : vec_type{ x, y } {}
				constexpr inner_offset_t(const type& other) noexcept : vec_type{ other, other } {}
				constexpr inner_offset_t(const inner_offset_t& other) noexcept : vec_type{ other.m_vec[0], other.m_vec[1] } {}

				constexpr inline decltype(auto) set_x(const type& x) noexcept { vec_type::m_vec[0] = x; return *this; }
				constexpr inline decltype(auto) set_y(const type& y) noexcept { vec_type::m_vec[1] = y; return *this; }

				constexpr inline decltype(auto) x() noexcept { return vec_type::m_vec[0]; }
				constexpr inline decltype(auto) y() noexcept { return vec_type::m_vec[1]; }
				constexpr inline decltype(auto) x() const noexcept { return vec_type::m_vec[0]; }
				constexpr inline decltype(auto) y() const noexcept { return vec_type::m_vec[1]; }

				//constexpr decltype(auto) to_rvec() { return rvec2_t<_type&>{vec_t<_type, 2, true>::m_proxy0, vec_t<_type, 2, true>::m_proxy1}; }
			};
		}
		template<typename _type>
		using offset2_t = priv::inner_offset_t<_type, 2>;
	}
}