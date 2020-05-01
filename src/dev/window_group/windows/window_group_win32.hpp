#pragma once

#include "./../../../../inc/dev/window_group/window_group.hpp"

#include <Windows.h>
#include <cassert>

namespace cw {
	namespace dev {
		namespace priv {
			class window_group_win32_t : public window_group_t {
			public:
				window_group_win32_t(const window_group_ci_t& create_info);
				virtual ~window_group_win32_t();
				virtual std::queue<event_t>& update();
				//virtual bool is_draw_able();
				virtual void kill_active_priv();
				const HINSTANCE get_hinstance() const;
				const HWND get_hwnd() const;
			private:
				static void global_build();
				static void global_clean();
				static void global_register_wndclass();
				static void global_unregister_wndclass();
				static HWND create_window(const window_group_ci_t& create_info, void* extptr = nullptr);
				static void destroy_window(HWND& hwnd);
				static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
			private:
				static core::ull_t m_global_counter;
				static HINSTANCE m_global_instance;
				static std::wstring m_global_wndclass_name;

				HWND m_hwnd;

				struct {
					bool enter_size_move = false;
					bool min = false;
					bool max = false;
					offset_t last_offset;
					extent_t last_extent;
				}m_data;
			};
		}
	}
}

//#pragma once
//
//#include "./../../../../inc/dev/window_group/window_group.hpp"
//
//#include <Windows.h>
//#include <cassert>
//
//namespace cw {
//	namespace dev {
//		namespace priv {
//			class window_group_win32_t : public window_group_t {
//			public:
//				window_group_win32_t(const window_group_ci_t& create_info) {
//					global_build();
//					m_hwnd = create_window(create_info, this);
//					window_group_t::m_is_active = true;
//				}
//				virtual ~window_group_win32_t() {
//					destroy_window(m_hwnd);
//					global_clean();
//				}
//				virtual std::queue<event_t>& update() {
//					while (!window_group_t::m_event_queue.empty()) { window_group_t::m_event_queue.pop(); }
//					MSG message;
//					while (PeekMessageW(&message, m_hwnd, 0, 0, PM_REMOVE)) {
//						TranslateMessage(&message);
//						DispatchMessageW(&message);
//					}
//					return window_group_t::m_event_queue;
//				}
//				const HINSTANCE get_hinstance() const { return m_global_instance; }
//				const HWND get_hwnd() const { return m_hwnd; }
//			private:
//				static void global_build() {
//					if (m_global_counter == 0) {
//						m_global_instance = GetModuleHandleW(nullptr);
//						global_register_wndclass();
//
//					}
//					++m_global_counter;
//				}
//				static void global_clean() {
//					--m_global_counter;
//					if (m_global_counter == 0) {
//
//						global_unregister_wndclass();
//						m_global_instance = nullptr;
//					}
//				}
//				static void global_register_wndclass() {
//					WNDCLASSW window_class;
//					window_class.style = 0;
//					window_class.lpfnWndProc = wndproc;
//					window_class.cbClsExtra = 0;
//					window_class.cbWndExtra = 0;
//					window_class.hInstance = m_global_instance;
//					window_class.hIcon = NULL;
//					window_class.hCursor = 0;
//					window_class.hbrBackground = 0;
//					window_class.lpszMenuName = NULL;
//					window_class.lpszClassName = m_global_wndclass_name.c_str();
//					auto success = RegisterClassW(&window_class);
//					assert(success);
//				}
//				static void global_unregister_wndclass() {
//					UnregisterClassW(m_global_wndclass_name.c_str(), m_global_instance);
//				}
//				static HWND create_window(const window_group_ci_t& create_info, void* extptr = nullptr) {
//					DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
//					DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
//
//					RECT wr = { 0, 0, create_info.m_rect.m_extent.width(), create_info.m_rect.m_extent.height() };
//					AdjustWindowRectEx(&wr, style, FALSE, ex_style);
//					auto width = wr.right - wr.left;
//					auto height = wr.bottom - wr.top;
//
//					auto hwnd = CreateWindowExW(
//						0,
//						m_global_wndclass_name.c_str(), create_info.m_title.c_str(),		// class name/app name
//						style,							    // window style
//						//CW_USEDEFAULT, CW_USEDEFAULT,	    // x/y coords
//						create_info.m_rect.m_offset.x(), create_info.m_rect.m_offset.y(),                 // x/y coords
//						width, height,					// width/height
//						NULL, NULL,						// handle to parent/menu
//						m_global_instance, extptr);			// hInstance/extra parameters
//					assert(hwnd);
//					//SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
//					ShowWindow(hwnd, SW_SHOW);
//					SetForegroundWindow(hwnd);
//					SetFocus(hwnd);
//					return hwnd;
//				}
//				static void destroy_window(HWND& hwnd) {
//					assert(hwnd);
//					auto result = DestroyWindow(hwnd);
//					/*if (!result) {
//						auto rt = GetLastError();
//						assert(result);
//					}
//					assert(result);*/
//					hwnd = nullptr;
//				}
//				/*static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
//					switch (msg) {
//					case WM_CREATE: {
//						LONG_PTR create_param = (LONG_PTR)reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams;
//						window_group_win32_t* group = reinterpret_cast<window_group_win32_t*>(create_param);
//						group->m_is_active = true;
//						SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create_param);
//					}break;
//					default: {
//						window_group_win32_t* group = hwnd ? reinterpret_cast<window_group_win32_t*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)) : nullptr;
//						switch (msg) {
//						case WM_CLOSE: {
//							PostQuitMessage(0);
//							group->m_event_queue.push(event_t{ event_e::e_close });
//							group->m_is_active = false;
//							SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)nullptr);
//						}break;
//						}
//					}break;
//					}
//					return DefWindowProcW(hwnd, msg, wparam, lparam);
//				}*/
//				static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
//					window_group_win32_t* group = nullptr;
//					if (msg == WM_CREATE) {
//						LONG_PTR create_param = (LONG_PTR)reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams;
//						group = reinterpret_cast<window_group_win32_t*>(create_param);
//						group->m_is_active = true;
//						SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create_param);
//						return DefWindowProcW(hwnd, msg, wparam, lparam);
//					}
//					group = hwnd ? reinterpret_cast<window_group_win32_t*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)) : nullptr;
//					switch (msg) {
//					case WM_CLOSE: {
//						PostQuitMessage(0);
//						group->m_event_queue.push(event_t{ event_e::e_close });
//						group->m_is_active = false;
//						SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)nullptr);
//					}break;
//					}
//					return DefWindowProcW(hwnd, msg, wparam, lparam);
//				}
//			private:
//				static core::ull_t m_global_counter;
//				static HINSTANCE m_global_instance;
//				static std::wstring m_global_wndclass_name;
//
//				HWND m_hwnd;
//			};
//
//			core::ull_t window_group_win32_t::m_global_counter = 0;
//			HINSTANCE window_group_win32_t::m_global_instance = nullptr;
//			std::wstring window_group_win32_t::m_global_wndclass_name = L"cw-dev-wndclass";
//
//		}
//		std::unique_ptr<window_group_t> build_window_group(const window_group_ci_t& create_info) { return std::make_unique<priv::window_group_win32_t>(create_info); }
//	}
//}