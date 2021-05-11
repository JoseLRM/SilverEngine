#pragma once

#include "platform/graphics.h"
#include "platform/os.h"
#include "core/font.h"

namespace sv {

    constexpr u32 GUI_WINDOW_NAME_SIZE = 30u;

    bool _gui_initialize();
    bool _gui_close();

    bool _gui_begin();
    void _gui_end();

    void _gui_draw(CommandList cmd);

    enum GuiPopupTrigger : u32 {
	GuiPopupTrigger_Root,
	GuiPopupTrigger_LastWidget
    };

    SV_API void gui_push_id(u64 id);
    SV_API void gui_push_id(const char* id);
    SV_API void gui_pop_id(u32 count = 1u);
	
    SV_API bool gui_begin_window(const char* title);
    SV_API void gui_end_window();
    SV_API bool gui_show_window(const char* title);
    SV_API bool gui_hide_window(const char* title);

    SV_API bool gui_begin_popup(const char* title, GuiPopupTrigger trigger);
    SV_API void gui_end_popup();

    SV_API bool gui_button(const char* text, u64 id);
    SV_API bool gui_checkbox(const char* text, bool& value, u64 id);
    SV_API bool gui_checkbox(const char* text, u64 id);
    SV_API bool gui_drag_f32(f32& value, f32 adv, f32 min, f32 max, u64 id);
    SV_API bool gui_drag_v2_f32(v2_f32& value, f32 adv, f32 min, f32 max, u64 id);
    SV_API void gui_text(const char* text, u64 id);
    SV_API bool gui_collapse(const char* text, u64 id);
    SV_API void gui_image(GPUImage* image, GPUImageLayout layout, u64 id);
    
}
