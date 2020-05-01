#pragma once

#include "./../../core/integer.hpp"
#include "./../../core/offset2.hpp"
#include "./../../core/extent2.hpp"
#include "./../../core/rect.hpp"

#include <string>
#include <memory>
#include <variant>
#include <queue>

namespace cw {
	namespace dev {
		using namespace std::string_literals;

		using offset_t = core::offset2_t<core::s32_t>;
		using extent_t = core::extent2_t<core::u32_t>;
		using rect_t = core::rect_t<offset_t, extent_t>;

		struct window_group_ci_t {
			window_group_ci_t() noexcept : m_rect{ offset_t{200, 100}, extent_t{800, 600 } }, m_title{ L"ef-dev-window_group"s }{}
			window_group_ci_t(const rect_t& rect, const std::wstring& title) noexcept : m_rect{ rect }, m_title{ title }{}

			inline decltype(auto) set_rect(const rect_t& rect) noexcept { m_rect = rect; return *this; }
			inline decltype(auto) set_title(const std::wstring& title) noexcept { m_title = title; return *this; }

		public:
			rect_t m_rect;
			std::wstring m_title;
		};

		enum class key_e {
			e_w, e_a, e_s, e_d, e_left, e_right, e_up, e_down, e_space, e_r, e_f, e_null
		};

		inline decltype(auto) to_string(key_e key) {
			switch (key) {
			case key_e::e_w: return "W"s;
			case key_e::e_a: return "A"s;
			case key_e::e_s: return "S"s;
			case key_e::e_d: return "D"s;
			case key_e::e_left: return "LEFT"s;
			case key_e::e_right: return "RIGHT"s;
			case key_e::e_up: return "UP"s;
			case key_e::e_down: return "DOWN"s;
			case key_e::e_space: return "SPACE"s;
			case key_e::e_r: return "R"s;
			case key_e::e_f: return "F"s;
			}
			return "NULL"s;
		}

		enum class event_e {
			e_close, e_move, e_resize, e_rect, 
			e_keydown, e_keyup, 
			e_null
		};

		using event_detail_t = std::variant<std::monostate, offset_t, extent_t, rect_t, key_e>;

		struct event_t {
			//event_t() :etype{ event_e::e_null }, detail{ std::monostate{} }{}
			event_t(event_e etype = event_e::e_null,const event_detail_t& detail = std::monostate{}) :etype{ etype }, detail{ detail }{}
			inline decltype(auto) set_type(event_e etype) { this->etype = etype; return *this; }
			inline decltype(auto) set_detail(const event_detail_t& detail) { this->detail = detail; return *this; }
			event_e etype;
			event_detail_t detail;
		};

		class window_group_t {
		public:
			virtual ~window_group_t() {}
			virtual std::queue<event_t>& update() = 0;
			bool is_active() { if (m_is_active) return true; kill_active_priv(); return false; }
			//virtual bool is_draw_able() = 0;
			void kill_active() { m_is_active = false; }
		protected:
			window_group_t() :m_is_active{ false } {}
			virtual void kill_active_priv() = 0;
			std::queue<event_t> m_event_queue;
			bool m_is_active;
		};

		std::unique_ptr<window_group_t> build_window_group(const window_group_ci_t& create_info);
	}
}
