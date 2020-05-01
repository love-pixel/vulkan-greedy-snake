#include "./../../../../inc/dev/window_group/priv/platform_support_win32.hpp"
#include "./window_group_win32.hpp"

namespace cw {
	namespace dev {
		namespace priv {
			HINSTANCE get_hinstance(window_group_t* group) {
				window_group_win32_t* native = dynamic_cast<window_group_win32_t*>(group);
				assert(native != nullptr);
				return native->get_hinstance();
			}
			HWND get_hwnd(window_group_t* group) {
				window_group_win32_t* native = dynamic_cast<window_group_win32_t*>(group);
				assert(native != nullptr);
				return native->get_hwnd();
			}
		}
	}
}