#pragma once

#include <Windows.h>

namespace cw {
	namespace dev {
		class window_group_t;
		namespace priv {
			HINSTANCE get_hinstance(window_group_t* group);
			HWND get_hwnd(window_group_t* group);
		}
	}
}
