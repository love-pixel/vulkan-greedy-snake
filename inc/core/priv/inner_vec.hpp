#pragma once

#include "./../integer.hpp"
#include <type_traits>

namespace cw {
	namespace core {
		namespace priv {
			template<typename _type, ull_t _size>
			struct inner_vec_t { static_assert(std::is_reference_v<_type> == false); };
		}
	}
}