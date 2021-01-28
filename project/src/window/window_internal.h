#pragma once

#include "SilverEngine/window.h"
#include "graphics/graphics_internal.h"

namespace sv {

	Result window_initialize();
	Result window_close();

	struct Window_internal {

		WindowState			state = WindowState_Invalid;
		WindowFlags			flags = WindowFlag_None;
		std::wstring		title;
		v4_u32				bounds;
		WindowHandle		handle = nullptr;
		bool				close_request = false;
		bool				resized = false;
		bool				style_modified;
		bool				flags_modified;
		WindowState			new_state;
		WindowFlags			new_flags;
		SwapChain_internal* swap_chain = nullptr;
		std::vector<u8>		raw_mouse_buffer;
		v2_f32				mouse_dragging_acumulation;

	};

}