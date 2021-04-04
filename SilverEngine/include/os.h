#pragma once

#include "core.h"

namespace sv {

    SV_API void print(const char* str);
    SV_API void printf(const char* str, ...);
    SV_API void show_message(const char* title, const char* content, bool error);

    /* TODO
       std::string file_dialog_open(u32 filterCount, const char** filters, const char* startPath);
       std::string file_dialog_save(u32 filterCount, const char** filters, const char* startPath);

       void set_cursor_position(Window* window, f32 x, f32 y);

       void system_pause();
    */

    // Memory

    SV_API void* allocate_memory(size_t size);
    SV_API void free_memory(void* ptr);

    // Window

    enum WindowState : u32 {
	WindowState_Windowed,
	WindowState_Maximized,
	WindowState_Minimized,
	WindowState_Fullscreen,
    };
    
    SV_API u64 os_window_handle();
    SV_API v2_u32 os_window_size();
    SV_API f32 os_window_aspect();
    SV_API void os_window_set_fullscreen(bool fullscreen);
    SV_API WindowState os_window_state();
    SV_API v2_u32 os_desktop_size();
    
    // File Management

    SV_API bool path_is_absolute(const char* path);
    SV_API void path_clear(char* path);

    SV_API Result file_read_binary(const char* filepath, u8** pData, size_t* pSize);
    SV_API Result file_read_binary(const char* filepath, List<u8>& data);
    SV_API Result file_read_text(const char* filepath, std::string& str);
    SV_API Result file_write_binary(const char* filepath, const u8* data, size_t size, bool append = false);
    SV_API Result file_write_text(const char* filepath, const char* str, size_t size, bool append = false);

    SV_API Result file_remove(const char* filepath);
    SV_API Result file_copy(const char* srcpath, const char* dstpath);
    SV_API bool   file_exists(const char* filepath);

    SV_API Result file_date(const char* filepath, Date* create, Date* last_write, Date* last_access);

    struct FolderIterator {
	u64 _handle;
    };

    struct FolderElement {
	bool is_file;
	Date create_date;
	Date last_write_date;
	Date last_access_date;
	char name[50];
	const char* extension;
    };

    SV_API Result folder_iterator_begin(const char* folderpath, FolderIterator* iterator, FolderElement* element);
    SV_API bool folder_iterator_next(FolderIterator* iterator, FolderElement* element);
    SV_API void folder_iterator_close(FolderIterator* iterator);
    
    SV_API Result load_image(const char* filePath, void** pdata, u32* width, u32* height);

    SV_API Result bin_read(size_t hash, List<u8>& data);
    SV_API Result bin_read(size_t hash, Archive& archive);

    SV_API Result bin_write(size_t hash, const void* data, size_t size);
    SV_API Result bin_write(size_t hash, Archive& archive);
    
    enum MouseButton : u32 {
	MouseButton_Left,
	MouseButton_Right,
	MouseButton_Center,

	MouseButton_MaxEnum,
	MouseButton_None,
    };

    enum Key : u32 {
	Key_Tab,
	Key_Shift,
	Key_Control,
	Key_Capital,
	Key_Escape,
	Key_Alt,
	Key_Space,
	Key_Left,
	Key_Up,
	Key_Right,
	Key_Down,
	Key_Enter,
	Key_Insert,
	Key_Delete,
	Key_Supr,
	Key_A,
	Key_B,
	Key_C,
	Key_D,
	Key_E,
	Key_F,
	Key_G,
	Key_H,
	Key_I,
	Key_J,
	Key_K,
	Key_L,
	Key_M,
	Key_N,
	Key_O,
	Key_P,
	Key_Q,
	Key_R,
	Key_S,
	Key_T,
	Key_U,
	Key_V,
	Key_W,
	Key_Z,
	Key_Num0,
	Key_Num1,
	Key_Num2,
	Key_Num3,
	Key_Num4,
	Key_Num5,
	Key_Num6,
	Key_Num7,
	Key_Num8,
	Key_Num9,
	Key_F1,
	Key_F2,
	Key_F3,
	Key_F4,
	Key_F5,
	Key_F6,
	Key_F7,
	Key_F8,
	Key_F9,
	Key_F10,
	Key_F11,
	Key_F12,
	Key_F13,
	Key_F14,
	Key_F15,
	Key_F16,
	Key_F17,
	Key_F18,
	Key_F19,
	Key_F20,
	Key_F21,
	Key_F22,
	Key_F23,
	Key_F24,

	Key_MaxEnum,
	Key_None,
    };

    enum InputState : u8 {
	InputState_None,
	InputState_Pressed,
	InputState_Hold,
	InputState_Released,
    };

    enum TextCommand : u32 {
	TextCommand_Null,
	TextCommand_DeleteLeft,
	TextCommand_DeleteRight,
	TextCommand_Enter,
	TextCommand_Escape,
    };

    struct GlobalInputData {

	InputState keys[Key_MaxEnum];
	InputState mouse_buttons[MouseButton_MaxEnum];

	std::string text;
	std::vector<TextCommand> text_commands;

	v2_f32	mouse_position;
	v2_f32	mouse_last_pos;
	v2_f32	mouse_dragged;
	f32	mouse_wheel;

	bool unused;

    };

    extern GlobalInputData SV_API_VAR input;
    
}
