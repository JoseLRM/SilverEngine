#include "core.h"

#include "window_internal.h"
#include "graphics/graphics_internal.h"
#include "platform_impl.h"

#include "engine.h"
#include "input/input_internal.h"

namespace sv {

	static WindowHandle g_WindowHandle;

	static uvec4 g_Bounds;
	static uvec4 g_BoundsNew;
	static WindowStyleFlags g_Style;
	static bool g_StyleModified = false;
	static WindowStyleFlags g_StyleNew = false;
	static bool g_Resized = false;

	static uvec4 g_LastFullscreen_Bounds;
	static bool g_LastFullscreen_Maximized;

	static std::wstring g_Title;
	static std::wstring g_IconFilePath;

	WindowStyleFlags window_filter_style(WindowStyleFlags flags)
	{
		if (flags & WindowStyle_Fullscreen) flags = WindowStyle_Fullscreen;
		else if (flags & WindowStyle_NoBorder && ~flags & WindowStyle_NoTitle) flags &= ~WindowStyle_NoBorder;
		return flags;
	}

	// WINDOWS

#ifdef SV_PLATFORM_WINDOWS

	static WindowProc g_UserProc = nullptr;

	DWORD window_parse_style(WindowStyleFlags flags)
	{
		DWORD style = WS_VISIBLE;

		if (flags & WindowStyle_Fullscreen) style |= WS_POPUP;
		else {
			if (~flags & WindowStyle_NoTitle) style |= WS_CAPTION | WS_OVERLAPPED;
			if (~flags & WindowStyle_NoBorder) style |= WS_BORDER | WS_SYSMENU;
			if (flags & WindowStyle_Maximizable) style |= WS_MINIMIZEBOX;
			if (flags & WindowStyle_Minimizable) style |= WS_MAXIMIZEBOX;
			if (flags & WindowStyle_Resizable) style |= WS_SIZEBOX;
		}

		return style;
	}

	void window_adjust(int x, int y, int& w, int& h, DWORD style)
	{
		RECT rect;
		rect.left = x;
		rect.right = x + w;
		rect.top = y;
		rect.bottom = y + h;

		AdjustWindowRect(&rect, style, false);
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;
	}

	ivec4 window_adjusted_bounds(DWORD style)
	{
		ivec4 bounds;
		if (g_Style & WindowStyle_Fullscreen) {
			uvec2 res = window_desktop_size();
			bounds = { 0, 0, int(res.x), int(res.y) };
		}
		else {
			bounds = ivec4(g_Bounds.x, g_Bounds.y, g_Bounds.z, g_Bounds.w);
			window_adjust(bounds.x, bounds.y, bounds.z, bounds.w, style);
		}
		return bounds;
	}

	LRESULT CALLBACK WindowProcFn(HWND hWnd, ui32 msg, WPARAM wParam, LPARAM lParam)
	{
		// Call User Functions
		if (g_UserProc) g_UserProc(hWnd, msg, wParam, lParam);

		switch (msg) {
		case WM_DESTROY:
		case WM_CLOSE:
		case WM_QUIT:
			engine_request_close();
			return 0;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			// input
			ui8 keyCode = (ui8)wParam;
			if (keyCode > 255) {
				log_warning("Unknown keycode: %u", keyCode);
			}
			else if (~lParam & (1 << 30)) input_key_pressed_add(keyCode);

			break;
		}
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			// input
			ui8 keyCode = (ui8)wParam;
			if (keyCode > 255) {
				log_warning("Unknown keycode: %u", keyCode);
				VK_SHIFT;
			}
			else input_key_released_add(keyCode);

			break;
		}
		case WM_LBUTTONDOWN:
			input_mouse_pressed_add(0);
			break;
		case WM_RBUTTONDOWN:
			input_mouse_pressed_add(1);
			break;
		case WM_MBUTTONDOWN:
			input_mouse_pressed_add(2);
			break;
		case WM_LBUTTONUP:
			input_mouse_released_add(0);
			break;
		case WM_RBUTTONUP:
			input_mouse_released_add(1);
			break;
		case WM_MBUTTONUP:
			input_mouse_released_add(2);
			break;
		case WM_MOUSEMOVE:
		{
			ui16 _x = LOWORD(lParam);
			ui16 _y = HIWORD(lParam);

			float w = g_Bounds.z;
			float h = g_Bounds.w;

			float x = (float(_x) / w) - 0.5f;
			float y = (1.f - (float(_y) / h)) - 0.5f;

			input_mouse_position_set(x, y);
			break;
		}

		// RAW MOUSE
		//case WM_INPUT:
		//{
		//	UINT size;
		//	if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == -1) break;
		//
		//	rawMouseBuffer.resize(size);
		//	if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawMouseBuffer.data(), &size, sizeof(RAWINPUTHEADER)) == -1) break;
		//
		//	RAWINPUT& rawInput = reinterpret_cast<RAWINPUT&>(*rawMouseBuffer.data());
		//	if (rawInput.header.dwType == RIM_TYPEMOUSE) {
		//		if (rawInput.data.mouse.lLastX != 0 || rawInput.data.mouse.lLastY != 0) {
		//			jshInput::MouseDragged(rawInput.data.mouse.lLastX, rawInput.data.mouse.lLastY);
		//		}
		//	}
		//}
		//break;
		case WM_SIZE:
		{
			g_Bounds.z = LOWORD(lParam);
			g_Bounds.w = HIWORD(lParam);
			g_BoundsNew = g_Bounds;

			//switch (wParam)
			//{
			//case SIZE_MINIMIZED:
			//	m_Minimized = true;
			//	break;
			//default:
			//	m_Minimized = false;
			//	break;
			//}

			g_Resized = true;

			break;
		}
		case WM_MOVE:
		{
			g_Bounds.x = LOWORD(lParam);
			g_Bounds.y = HIWORD(lParam);
			g_BoundsNew = g_Bounds;

			//m_Minimized = false;
			//jsh::WindowMovedEvent e(screenX, screenY);
			//jshEvent::Dispatch(e);
			break;
		}
		case WM_SETFOCUS:
		{
			//jsh::WindowGainFocusEvent e;
			//jshEvent::Dispatch(e);
			break;
		}
		case WM_KILLFOCUS:
		{
			//jsh::WindowLostFocusEvent e;
			//jshEvent::Dispatch(e);
			break;
		}

		}

		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
#endif

	Result window_initialize(const InitializationWindowDesc& desc)
	{
#ifdef SV_PLATFORM_WINDOWS

		// Register class
		WNDCLASSW wndClass;
		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hbrBackground = 0;
		wndClass.hCursor = LoadCursorW(0, IDC_ARROW);
		wndClass.hIcon = 0;
		wndClass.hInstance = 0;
		wndClass.lpfnWndProc = WindowProcFn;
		wndClass.lpszClassName = L"SilverWindow";
		wndClass.lpszMenuName = 0;
		wndClass.style = 0;

		if (!RegisterClassW(&wndClass)) {
			sv::log_error("Can't Register Window Class");
			return Result_PlatformError;
		}

		// Set style
		WindowStyleFlags styleFlags = window_filter_style(desc.style);
		DWORD style = window_parse_style(styleFlags);
		g_Style = desc.style;

		if (desc.style & WindowStyle_Fullscreen) {
			uvec2 res = window_desktop_size();
			g_Bounds = { 0u, 0u, res.x, res.y };
		}
		else {
			g_Bounds = desc.bounds;
		}

		g_BoundsNew = g_Bounds;

		int w = g_Bounds.z, h = g_Bounds.w;
		window_adjust(g_Bounds.x, g_Bounds.y, w, h, style);

		// Title
		if (desc.title != nullptr) g_Title = desc.title;
		else g_Title = L"SilverEngine";

		g_WindowHandle = CreateWindowExW(0u, L"SilverWindow", g_Title.c_str(), style, g_Bounds.x, g_Bounds.y, w, h, 0, 0, 0, 0);
#endif

		if (g_WindowHandle == 0) {
			log_error("Error creating Window class");
			return Result_PlatformError;
		}

		// Set Icon
		if (desc.iconFilePath != nullptr) {
			svCheck(window_icon_set(desc.iconFilePath));
		}

		return Result_Success;
	}

	Result window_close()
	{
		return CloseWindow(HWND(g_WindowHandle)) ? Result_Success : Result_PlatformError;
	}

	void window_update()
	{
		g_Resized = false;

		if (g_Bounds.x != g_BoundsNew.x || g_Bounds.y != g_BoundsNew.y || g_Bounds.z != g_BoundsNew.z || g_Bounds.w != g_BoundsNew.w) {
			g_Bounds = g_BoundsNew;
#ifdef SV_PLATFORM_WINDOWS
			ivec4 bounds = window_adjusted_bounds(GetWindowLongPtr((HWND)g_WindowHandle, GWL_STYLE));
			SetWindowPos((HWND)g_WindowHandle, 0u, bounds.x, bounds.y, bounds.z, bounds.w, 0);
#endif
		}

		if (g_StyleModified) {
			graphics_gpu_wait();

			bool maximize = false;

			if (g_Style & WindowStyle_Fullscreen && ~g_StyleNew & WindowStyle_Fullscreen) {
				maximize = g_LastFullscreen_Maximized;
				if (!maximize) {
					g_Bounds = g_LastFullscreen_Bounds;
					g_BoundsNew = g_Bounds;
				}
			}
			else if (~g_Style & WindowStyle_Fullscreen && g_StyleNew & WindowStyle_Fullscreen) {
				g_LastFullscreen_Bounds = g_Bounds;
				g_LastFullscreen_Maximized = GetWindowLong((HWND)g_WindowHandle, GWL_STYLE) & WS_MAXIMIZE;
			}

			g_Style = g_StyleNew;

#ifdef SV_PLATFORM_WINDOWS
			// Set Windows style
			DWORD style = window_parse_style(g_Style);
			if (maximize) {
				style |= WS_MAXIMIZE;
			}
			SetWindowLongPtrW((HWND)sv::window_handle_get(), GWL_STYLE, (LONG_PTR)style);

			ivec4 bounds = window_adjusted_bounds(style);
			SetWindowPos((HWND)g_WindowHandle, 0u, bounds.x, bounds.y, bounds.z, bounds.w, 0);
#endif

			g_StyleModified = false;
		}

		MSG msg;

		while (PeekMessageW(&msg, 0, 0u, 0u, PM_REMOVE) > 0) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		
		if (g_Resized) {
			graphics_swapchain_resize();
		}
	}

	// HANDLE

	WindowHandle window_handle_get() noexcept { return g_WindowHandle; }

	// BOUNDS

	uvec4 window_bounds_get()
	{
		return g_Bounds;
	}

	uvec2 window_position_get()
	{
		return { g_Bounds.x, g_Bounds.y };
	}

	uvec2 window_size_get()
	{
		return { g_Bounds.z, g_Bounds.w };
	}

	float window_aspect_get()
	{
		return float(g_Bounds.z) / float(g_Bounds.w);
	}

	void window_bounds_set(const uvec4& bounds)
	{
		g_BoundsNew = bounds;
	}

	void window_position_set(const uvec2& position)
	{
		g_BoundsNew.x = position.x;
		g_BoundsNew.y = position.y;
	}

	void window_size_set(const uvec2& size)
	{
		g_BoundsNew.z = size.x;
		g_BoundsNew.w = size.y;
	}

	void window_aspect_set(float aspect)
	{
		log_error("TODO->window_aspect_set");
	}

	// ICON

	Result window_icon_set(const wchar* filePath)
	{
#ifdef SV_SRC_PATH
		std::wstring filePathStr = SV_SRC_PATH_W;
		filePathStr += filePath;
		filePath = filePathStr.c_str();
#endif

#ifdef SV_PLATFORM_WINDOWS
		
		auto icon = LoadImageW(0, filePath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED | LR_LOADTRANSPARENT);
		if (icon == NULL) return Result_PlatformError;

		HWND hwnd = (HWND)window_handle_get();

		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);

#endif

		return Result_Success;
	}

	const wchar* window_icon_get_filepath()
	{
		return g_IconFilePath.c_str();
	}

	// TITLE

	Result window_title_set(const wchar* title)
	{
		if (!SetWindowText((HWND)g_WindowHandle, title)) return Result_PlatformError;
		return Result_Success;
	}

	const wchar* window_title_get()
	{
		return g_Title.c_str();
	}

	// STYLE

	WindowStyleFlags window_style_get()
	{
		return g_Style;
	}

	void window_style_set(WindowStyleFlags style)
	{
		style = window_filter_style(style);
		if (style != g_Style) {
			g_StyleModified = true;
			g_StyleNew = style;
		}
	}

	// UTILS

	uvec2 window_desktop_size()
	{
#ifdef SV_PLATFORM_WINDOWS
		HWND desktop = GetDesktopWindow();
		RECT rect;
		GetWindowRect(desktop, &rect);
		return { ui32(rect.right), ui32(rect.bottom) };
#endif
	}

#ifdef SV_PLATFORM_WINDOWS

	void window_userproc_set(WindowProc userProc)
	{
		g_UserProc = userProc;
	}
	
#endif

}