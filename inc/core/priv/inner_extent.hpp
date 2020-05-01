#pragma once

#include "./inner_vec.hpp"

namespace cw {
	namespace core {
		namespace priv {
			template<typename _type, ull_t _size>
			struct inner_extent_t : public inner_vec_t<_type, _size> {};
		}
	}
}
