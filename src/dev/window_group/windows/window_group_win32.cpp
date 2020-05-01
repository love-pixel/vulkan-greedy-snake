#include "./window_group_win32.hpp"

#include <iostream>

namespace cw {
	namespace dev {
		namespace priv {
			decltype(auto) to_key(UINT wparam) {
				switch (wparam) {
				case 'W': return key_e::e_w;
				case 'A': return key_e::e_a;
				case 'S': return key_e::e_s;
				case 'D': return key_e::e_d;
				case VK_LEFT: return key_e::e_left;
				case VK_RIGHT: return key_e::e_right;
				case VK_UP: return key_e::e_up;
				case VK_DOWN: return key_e::e_down;
				case VK_SPACE: return key_e::e_space;
				case 'R': return key_e::e_r;
				case 'F': return key_e::e_f;
				}
				return key_e::e_null;
			}

			window_group_win32_t::window_group_win32_t(const window_group_ci_t& create_info) {
				global_build();
				m_hwnd = create_window(create_info, this);
			}
			window_group_win32_t::~window_group_win32_t() {
				destroy_window(m_hwnd);
				global_clean();
			}
			std::queue<event_t>& window_group_win32_t::update() {
				while (!window_group_t::m_event_queue.empty()) { window_group_t::m_event_queue.pop(); }
				MSG message;
				while (PeekMessageW(&message, m_hwnd, 0, 0, PM_REMOVE)) {
					TranslateMessage(&message);
					DispatchMessageW(&message);
				}
				return window_group_t::m_event_queue;
			}
			//bool window_group_win32_t::is_draw_able() {
			//	//auto is_iconic = IsIconic(m_hwnd);
			//	RECT rect;
			//	GetClientRect(m_hwnd, &rect);
			//	//extent_t extent = { (core::u32_t)rect.right - rect.left, (core::u32_t)rect.bottom - rect.top };
			//	if (m_is_active == true && (rect.right - rect.left) != 0 && (rect.bottom - rect.top) != 0 && IsIconic(m_hwnd) == false) return true;
			//	return false;
			//}
			void window_group_win32_t::kill_active_priv() {
				PostQuitMessage(0);
			}
			const HINSTANCE window_group_win32_t::get_hinstance() const { return m_global_instance; }
			const HWND window_group_win32_t::get_hwnd() const { return m_hwnd; }

			void window_group_win32_t::global_build() {
				if (m_global_counter == 0) {
					m_global_instance = GetModuleHandleW(nullptr);
					global_register_wndclass();

				}
				++m_global_counter;
			}
			void window_group_win32_t::global_clean() {
				--m_global_counter;
				if (m_global_counter == 0) {

					global_unregister_wndclass();
					m_global_instance = nullptr;
				}
			}
			void window_group_win32_t::global_register_wndclass() {
				WNDCLASSW window_class;
				window_class.style = 0;
				window_class.lpfnWndProc = wndproc;
				window_class.cbClsExtra = 0;
				window_class.cbWndExtra = 0;
				window_class.hInstance = m_global_instance;
				window_class.hIcon = NULL;
				window_class.hCursor = LoadCursor(NULL, IDC_ARROW); // 鼠标进入到窗口后的图标
				window_class.hbrBackground = 0;
				window_class.lpszMenuName = NULL;
				window_class.lpszClassName = m_global_wndclass_name.c_str();
				auto success = RegisterClassW(&window_class);
				assert(success);
			}
			void window_group_win32_t::global_unregister_wndclass() {
				UnregisterClassW(m_global_wndclass_name.c_str(), m_global_instance);
			}
			HWND window_group_win32_t::create_window(const window_group_ci_t& create_info, void* extptr) {
				DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
				DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;

				RECT wr = { 0, 0, create_info.m_rect.m_extent.width(), create_info.m_rect.m_extent.height() };
				AdjustWindowRectEx(&wr, style, FALSE, ex_style);
				auto width = wr.right - wr.left;
				auto height = wr.bottom - wr.top;

				auto hwnd = CreateWindowExW(
					0,
					m_global_wndclass_name.c_str(), create_info.m_title.c_str(),		// class name/app name
					style,							    // window style
					//CW_USEDEFAULT, CW_USEDEFAULT,	    // x/y coords
					create_info.m_rect.m_offset.x(), create_info.m_rect.m_offset.y(),                 // x/y coords
					width, height,					// width/height
					NULL, NULL,						// handle to parent/menu
					m_global_instance, extptr);			// hInstance/extra parameters
				assert(hwnd);
				//SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
				ShowWindow(hwnd, SW_SHOW);
				SetForegroundWindow(hwnd);
				SetFocus(hwnd);
				return hwnd;
			}
			void window_group_win32_t::destroy_window(HWND& hwnd) {
				assert(hwnd);
				auto result = DestroyWindow(hwnd);
				/*if (!result) {
					auto rt = GetLastError();
					assert(result);
				}
				assert(result);*/
				hwnd = nullptr;
			}
			LRESULT CALLBACK window_group_win32_t::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
				window_group_win32_t* group = nullptr;
				if (msg == WM_CREATE) {
					LONG_PTR create_param = (LONG_PTR)reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams;
					group = reinterpret_cast<window_group_win32_t*>(create_param);
					group->m_is_active = true;
					SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create_param);
					return DefWindowProcW(hwnd, msg, wparam, lparam);
				}
				group = hwnd ? reinterpret_cast<window_group_win32_t*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)) : nullptr;
				if (group) {
					switch (msg) {
					case WM_CLOSE: {
						//PostQuitMessage(0);
						group->m_event_queue.push(event_t{ event_e::e_close });
						group->m_is_active = false;
						SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)nullptr);
						return 0;
					}break;
					case WM_ENTERSIZEMOVE: {
						group->m_data.enter_size_move = true;

						RECT rect{};
						GetWindowRect(hwnd, &rect);
						group->m_data.last_offset = { (core::s32_t)rect.left, (core::s32_t)rect.top };
						GetClientRect(hwnd, &rect);
						group->m_data.last_extent = { (core::u32_t)rect.right - rect.left, (core::u32_t)rect.bottom - rect.top };
					}break;
					case WM_EXITSIZEMOVE: {
						group->m_data.enter_size_move = false;

						bool resize = false, move = false, merge = false;
						event_t event;
						RECT rect{};
						GetWindowRect(hwnd, &rect);
						offset_t offset = { (core::s32_t)rect.left, (core::s32_t)rect.top };
						if (group->m_data.last_offset != offset) {
							resize = true;
							event.etype = event_e::e_move;
							event.detail = offset;
							group->m_data.last_offset = offset;
							if(!merge) group->m_event_queue.push(event);
						}
						GetClientRect(hwnd, &rect);
						extent_t extent = { (core::u32_t)rect.right - rect.left, (core::u32_t)rect.bottom - rect.top };
						if (group->m_data.last_extent != extent) {
							move = true;
							event.etype = event_e::e_resize;
							event.detail = extent;
							group->m_data.last_extent = extent;
							if (!merge) group->m_event_queue.push(event);
						}
						if (resize && move && merge) {
							event.etype = event_e::e_rect;
							event.detail = rect_t{ offset , extent };
							group->m_event_queue.push(event);
						}
						
					}break;
					case WM_SIZE: {
						switch (wparam) {
						case SIZE_MINIMIZED: {
							group->m_data.min = true;
							group->m_data.max = false;
							//std::cout << LOWORD(lparam) << " " << HIWORD(lparam) << std::endl;
							group->m_event_queue.push(event_t{ event_e::e_resize, extent_t{(core::u32_t)LOWORD(lparam),(core::u32_t)HIWORD(lparam)} });
						}break;
						case SIZE_MAXIMIZED: {
							group->m_data.min = false;
							group->m_data.max = true;
							//std::cout << LOWORD(lparam) << " " << HIWORD(lparam) << std::endl;
							group->m_event_queue.push(event_t{ event_e::e_resize, extent_t{(core::u32_t)LOWORD(lparam),(core::u32_t)HIWORD(lparam)} });
						}break;
						case SIZE_RESTORED: {
							if (group->m_data.min || group->m_data.max /*|| group->m_data.min == group->m_data.max*/) {
								group->m_event_queue.push(event_t{ event_e::e_resize, extent_t{(core::u32_t)LOWORD(lparam),(core::u32_t)HIWORD(lparam)} });
								group->m_data.min = false;
								group->m_data.max = false;
							}
						}break;
						}

						//if (wparam == SIZE_RESTORED) std::cout << "SIZE_RESTORED" << std::endl;
						//if (wparam == SIZE_MINIMIZED) std::cout << "SIZE_MINIMIZED" << std::endl;
						//if (wparam == SIZE_MAXIMIZED) std::cout << "SIZE_MAXIMIZED" << std::endl;
						if (wparam == SIZE_MAXSHOW) std::cout << "SIZE_MAXSHOW" << std::endl;
						if (wparam == SIZE_MAXHIDE) std::cout << "SIZE_MAXHIDE" << std::endl;
						//{
						//	if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
						//	{
						//		destWidth = LOWORD(lParam);
						//		destHeight = HIWORD(lParam);
						//		windowResize();
						//	}
						//}
					}break;
					case WM_KEYDOWN:
					case WM_SYSKEYDOWN: {
						group->m_event_queue.push(event_t{ event_e::e_keydown, to_key(wparam) });
					}break;
					case WM_KEYUP:
					case WM_SYSKEYUP: {
						group->m_event_queue.push(event_t{ event_e::e_keyup, to_key(wparam) });
					}break;
					}
				}
				return DefWindowProcW(hwnd, msg, wparam, lparam);
			}

			core::ull_t window_group_win32_t::m_global_counter = 0;
			HINSTANCE window_group_win32_t::m_global_instance = nullptr;
			std::wstring window_group_win32_t::m_global_wndclass_name = L"cw-dev-wndclass";

		}
		std::unique_ptr<window_group_t> build_window_group(const window_group_ci_t& create_info) { return std::make_unique<priv::window_group_win32_t>(create_info); }
	}
}