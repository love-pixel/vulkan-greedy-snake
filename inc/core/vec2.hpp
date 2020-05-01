#pragma once

#include "./priv/inner_vec.hpp"

namespace cw {
	namespace core {
		namespace priv {
			template<typename _type>
			struct inner_vec_t<_type, 2> {
				using type = _type;
				static_assert(std::is_reference_v<type> == false);

				constexpr inner_vec_t() noexcept :m_vec{} {}
				constexpr inner_vec_t(const type& v0, const type& v1) noexcept :m_vec{ v0,v1 } {}
				constexpr inner_vec_t(const type& other) noexcept : m_vec{ other, other } {}
				constexpr inner_vec_t(const inner_vec_t& other) noexcept : m_vec{ other.m_vec[0], other.m_vec[1] } {}

				constexpr inline ull_t size() const noexcept { return 2; }
				constexpr inline static ull_t size_s() noexcept { return 2; }
				constexpr inline const type* data() const noexcept { return m_vec; }
				constexpr inline type* data() noexcept { return m_vec; }
				constexpr inline decltype(auto) to_vec() const { return inner_vec_t{ *this }; }

				// operator : [] = - ==
				constexpr inline decltype(auto) operator[](ull_t index) const noexcept { return m_vec[index]; }
				constexpr inline decltype(auto) operator[](ull_t index) noexcept { return m_vec[index]; }
				constexpr inline decltype(auto) operator=(const inner_vec_t& other) noexcept { m_vec[0] = other.m_vec[0]; m_vec[1] = other.m_vec[1]; return *this; }
				constexpr inline decltype(auto) operator=(const type& other) noexcept { m_vec[0] = other; m_vec[1] = other; return *this; }
				constexpr inline decltype(auto) operator-() noexcept { return inner_vec_t{ -m_vec[0], -m_vec[1] }; }

				// operator : += -= *= /=
				constexpr inline decltype(auto) operator+=(const inner_vec_t& other) noexcept { m_vec[0] += other.m_vec[0]; m_vec[1] += other.m_vec[1]; return *this; }
				constexpr inline decltype(auto) operator-=(const inner_vec_t& other) noexcept { m_vec[0] -= other.m_vec[0]; m_vec[1] -= other.m_vec[1]; return *this; }
				constexpr inline decltype(auto) operator*=(const inner_vec_t& other) noexcept { m_vec[0] *= other.m_vec[0]; m_vec[1] *= other.m_vec[1]; return *this; }
				constexpr inline decltype(auto) operator/=(const inner_vec_t& other) noexcept { m_vec[0] /= other.m_vec[0]; m_vec[1] /= other.m_vec[1]; return *this; }
				
				constexpr inline decltype(auto) operator+=(const type& other) noexcept { m_vec[0] += other; m_vec[1] += other; return *this; }
				constexpr inline decltype(auto) operator-=(const type& other) noexcept { m_vec[0] -= other; m_vec[1] -= other; return *this; }
				constexpr inline decltype(auto) operator*=(const type& other) noexcept { m_vec[0] *= other; m_vec[1] *= other; return *this; }
				constexpr inline decltype(auto) operator/=(const type& other) noexcept { m_vec[0] /= other; m_vec[1] /= other; return *this; }

				// operator : + - * /
				constexpr inline decltype(auto) operator+(const inner_vec_t& other) const noexcept { return inner_vec_t{ m_vec[0] + other.m_vec[0], m_vec[1] + other.m_vec[1] }; }
				constexpr inline decltype(auto) operator-(const inner_vec_t& other) const noexcept { return inner_vec_t{ m_vec[0] - other.m_vec[0], m_vec[1] - other.m_vec[1] }; }
				constexpr inline decltype(auto) operator*(const inner_vec_t& other) const noexcept { return inner_vec_t{ m_vec[0] * other.m_vec[0], m_vec[1] * other.m_vec[1] }; }
				constexpr inline decltype(auto) operator/(const inner_vec_t& other) const noexcept { return inner_vec_t{ m_vec[0] / other.m_vec[0], m_vec[1] / other.m_vec[1] }; }

				constexpr inline decltype(auto) operator+(const type& other) const noexcept { return inner_vec_t{ m_vec[0] + other, m_vec[1] + other }; }
				constexpr inline decltype(auto) operator-(const type& other) const noexcept { return inner_vec_t{ m_vec[0] - other, m_vec[1] - other }; }
				constexpr inline decltype(auto) operator*(const type& other) const noexcept { return inner_vec_t{ m_vec[0] * other, m_vec[1] * other }; }
				constexpr inline decltype(auto) operator/(const type& other) const noexcept { return inner_vec_t{ m_vec[0] / other, m_vec[1] / other }; }

				constexpr inline static friend decltype(auto) operator+(const type& other, const inner_vec_t& vecn) noexcept { return inner_vec_t{ other + vecn.m_vec[0],other + vecn.m_vec[1] }; }
				constexpr inline static friend decltype(auto) operator-(const type& other, const inner_vec_t& vecn) noexcept { return inner_vec_t{ other - vecn.m_vec[0],other - vecn.m_vec[1] }; }
				constexpr inline static friend decltype(auto) operator*(const type& other, const inner_vec_t& vecn) noexcept { return inner_vec_t{ other * vecn.m_vec[0],other * vecn.m_vec[1] }; }
				constexpr inline static friend decltype(auto) operator/(const type& other, const inner_vec_t& vecn) noexcept { return inner_vec_t{ other / vecn.m_vec[0],other / vecn.m_vec[1] }; }

				// operator : > >= < <= ==
				constexpr inline decltype(auto) operator> (const inner_vec_t& other) const noexcept { return (m_vec[0] >  other.m_vec[0]) && (m_vec[1] >  other.m_vec[1]); }
				constexpr inline decltype(auto) operator>=(const inner_vec_t& other) const noexcept { return (m_vec[0] >= other.m_vec[0]) && (m_vec[1] >= other.m_vec[1]); }
				constexpr inline decltype(auto) operator< (const inner_vec_t& other) const noexcept { return (m_vec[0] <  other.m_vec[0]) && (m_vec[1] <  other.m_vec[1]); }
				constexpr inline decltype(auto) operator<=(const inner_vec_t& other) const noexcept { return (m_vec[0] <= other.m_vec[0]) && (m_vec[1] <= other.m_vec[1]); }
				constexpr inline decltype(auto) operator==(const inner_vec_t& other) const noexcept { return (m_vec[0] == other.m_vec[0]) && (m_vec[1] == other.m_vec[1]); }
				constexpr inline decltype(auto) operator!=(const inner_vec_t& other) const noexcept { return (m_vec[0] != other.m_vec[0]) || (m_vec[1] != other.m_vec[1]); }
						  
				constexpr inline decltype(auto) operator> (const type& other) const noexcept { return (m_vec[0] >  other) && (m_vec[1] >  other); }
				constexpr inline decltype(auto) operator>=(const type& other) const noexcept { return (m_vec[0] >= other) && (m_vec[1] >= other); }
				constexpr inline decltype(auto) operator< (const type& other) const noexcept { return (m_vec[0] <  other) && (m_vec[1] <  other); }
				constexpr inline decltype(auto) operator<=(const type& other) const noexcept { return (m_vec[0] <= other) && (m_vec[1] <= other); }
				constexpr inline decltype(auto) operator==(const type& other) const noexcept { return (m_vec[0] == other) && (m_vec[1] == other); }
				constexpr inline decltype(auto) operator!=(const type& other) const noexcept { return (m_vec[0] != other) || (m_vec[1] != other); }

				constexpr inline static friend decltype(auto) operator> (const type& other,const inner_vec_t& vecn) noexcept { return (other >  vecn.m_vec[0]) && (other >  vecn.m_vec[1]); }
				constexpr inline static friend decltype(auto) operator>=(const type& other,const inner_vec_t& vecn) noexcept { return (other >= vecn.m_vec[0]) && (other >= vecn.m_vec[1]); }
				constexpr inline static friend decltype(auto) operator< (const type& other,const inner_vec_t& vecn) noexcept { return (other <  vecn.m_vec[0]) && (other <  vecn.m_vec[1]); }
				constexpr inline static friend decltype(auto) operator<=(const type& other,const inner_vec_t& vecn) noexcept { return (other <= vecn.m_vec[0]) && (other <= vecn.m_vec[1]); }
				constexpr inline static friend decltype(auto) operator==(const type& other,const inner_vec_t& vecn) noexcept { return (other == vecn.m_vec[0]) && (other == vecn.m_vec[1]); }
				constexpr inline static friend decltype(auto) operator!=(const type& other,const inner_vec_t& vecn) noexcept { return (other != vecn.m_vec[0]) || (other != vecn.m_vec[1]); }

			public:
				type m_vec[2];
			};
		}
		template<typename _type>
		using vec2_t = priv::inner_vec_t<_type, 2>;
	}
}
