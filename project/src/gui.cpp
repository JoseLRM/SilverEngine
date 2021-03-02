#include "SilverEngine/core.h"

#include "SilverEngine/gui.h"
#include "SilverEngine/utils/allocators/InstanceAllocator.h"

#include "renderer/renderer_internal.h"

#define PARSE_GUI() sv::GUI_internal& gui = *reinterpret_cast<sv::GUI_internal*>(gui_)

namespace sv {

	/*
	  TODO LIST:
	  - GuiDraw structure that defines the rendering of a widget or some component (PD: Should not contain inherited alpha)
	  - Align gui widgets
	  - Popups
	  - Combobox
	*/

	struct GUI_internal {

		InstanceAllocator<GuiContainer, 10u> containers;
		InstanceAllocator<GuiButton, 10u>    buttons;
		InstanceAllocator<GuiSlider, 10u>    sliders;
		InstanceAllocator<GuiDrag, 10u>	     drags;
		InstanceAllocator<GuiTextField, 10u> textfields;
		InstanceAllocator<GuiCheckbox, 10u>  checkboxes;
	        InstanceAllocator<GuiCombobox, 10u>  comboboxes;
		InstanceAllocator<GuiLabel, 10u>     labels;

		InstanceAllocator<GuiWindow, 5u> windows;

		std::vector<GuiWidget*>	root;
		v2_f32                  resolution;
		GuiWidget* widget_focused = nullptr;
		u32                     focus_action = 0u;

		// used during update
		v2_f32 mouse_position;
		v2_f32 dragged_begin_pos;
		u32    text_position = 0u;

	};

	GUI* gui_create()
	{
		GUI_internal& gui = *new GUI_internal();

		return reinterpret_cast<GUI*>(&gui);
	}

	void gui_destroy(GUI* gui_)
	{
		PARSE_GUI();

		gui.containers.clear();
		gui.buttons.clear();
		gui.sliders.clear();
		gui.drags.clear();
		gui.textfields.clear();
		gui.checkboxes.clear();
		gui.comboboxes.clear();
		gui.labels.clear();

		gui.windows.clear();

		gui.root.clear();

		delete& gui;
	}

	static void gui_window_destroy_internal(GUI* gui_, GuiWindow* window)
	{
		PARSE_GUI();
		gui_widget_destroy(gui_, window->container);
	}

	void gui_clear(GUI* gui_)
	{
		PARSE_GUI();

		for (auto& pool : gui.windows) {
			for (GuiWindow& wnd : pool) {
				gui_window_destroy_internal(gui_, &wnd);
			}
		}
		gui.windows.clear();

		while (gui.root.size()) {

			GuiWidget* widget = gui.root.back();
			gui.root.pop_back();

			gui_widget_destroy(gui_, widget);
		}

		gui.widget_focused = nullptr;
	}

	// BOUNDS FUNCTIONS

	SV_INLINE static f32 compute_coord(const GUI_internal& gui, const GuiWidget::Coord& coord, f32 aspect_coord, f32 dimension, f32 resolution, bool vertical, f32 parent_coord, f32 parent_dimension)
	{
		f32 res = 0.5f;

		// Centred coord
		switch (coord.constraint)
		{
		case GuiConstraint_Relative:
			res = coord.value * parent_dimension;
			break;

		case GuiConstraint_Center:
			res = 0.5f * parent_dimension;
			break;

		case GuiConstraint_Pixel:
			res = coord.value / resolution;
			break;

		case GuiConstraint_Aspect:
			res = aspect_coord * coord.value * (vertical ? (gui.resolution.x / gui.resolution.y) : (gui.resolution.y / gui.resolution.x));
			break;

		}

		// Inverse coord
		if (coord.alignment >= 3u) {
			res = parent_dimension - res;
		}

		// Parent offset
		res += (parent_coord - parent_dimension * 0.5f);

		// Alignment
		switch (coord.alignment)
		{
		case GuiCoordAlignment_Left:
		case GuiCoordAlignment_InverseLeft:
			res = res + dimension * 0.5f;
			break;

		case GuiCoordAlignment_Right:
		case GuiCoordAlignment_InverseRight:
			res = res - dimension * 0.5f;
			break;
		}

		return res;
	}

	SV_INLINE static f32 compute_dimension(const GUI_internal& gui, const GuiWidget::Dimension& dimension, f32 inv_dimension, f32 resolution, bool vertical, f32 parent_dimension)
	{
		f32 res = 0.5f;

		// Centred coord
		switch (dimension.constraint)
		{
		case GuiConstraint_Relative:
			res = dimension.value * parent_dimension;
			break;

		case GuiConstraint_Center:
			res = 0.5f * parent_dimension;
			break;

		case GuiConstraint_Pixel:
			res = dimension.value / resolution;
			break;

		case GuiConstraint_Aspect:
			res = inv_dimension * (vertical ? (gui.resolution.x / gui.resolution.y) : (gui.resolution.y / gui.resolution.x));
			break;
		}

		return res;
	}

	SV_INLINE bool mouse_in_bounds(const GUI_internal& gui, const v4_f32& bounds)
	{
		return abs(bounds.x - gui.mouse_position.x) <= bounds.z * 0.5f && abs(bounds.y - gui.mouse_position.y) <= bounds.w * 0.5f;
	}

	SV_INLINE static f32 get_scroll_width(const GUI_internal& gui, f32 container_width)
	{
		return std::min(10.f / gui.resolution.x, container_width);
	}

	SV_INLINE static bool container_has_vertical_scroll(const GuiContainer& container)
	{
		return container.vertical_scroll && (container.up_extension != 0.f || container.down_extension != 0.f);
	}

	SV_INLINE static f32 get_button_scroll_height(const GuiContainer& container, f32 container_height)
	{
		return container_height * 0.1f;
	}

	static v4_f32 compute_widget_bounds(const GUI_internal& gui, GuiWidget& widget, v4_f32 parent_bounds)
	{
#ifndef SV_DIST
		if (widget.x.constraint == GuiConstraint_Aspect && widget.x.constraint == widget.y.constraint) {
			SV_LOG_WARNING("Gui coords has both aspect constraint, that's not possible...");
		}
		if (widget.w.constraint == GuiConstraint_Aspect && widget.w.constraint == widget.h.constraint) {
			SV_LOG_WARNING("Gui dimensions has both aspect constraint, that's not possible...");
		}
#endif

		// Scroll adjust
		if (widget.parent && container_has_vertical_scroll(*widget.parent)) {

			parent_bounds.y -= widget.parent->vertical_offset;

			f32 scroll_width = get_scroll_width(gui, parent_bounds.z);

			parent_bounds.x -= scroll_width * 0.5f;
			parent_bounds.z -= scroll_width;
		}

		// Compute local bounds
		f32 w;
		f32 h;
		f32 x;
		f32 y;

		if (widget.w.constraint == GuiConstraint_Aspect) {
			h = compute_dimension(gui, widget.h, 0.5f, gui.resolution.y, true, parent_bounds.w);
			w = compute_dimension(gui, widget.w, h, gui.resolution.x, false, parent_bounds.z);
		}
		else {
			w = compute_dimension(gui, widget.w, 0.5f, gui.resolution.x, false, parent_bounds.z);
			h = compute_dimension(gui, widget.h, w, gui.resolution.y, true, parent_bounds.w);
		}

		if (widget.x.constraint == GuiConstraint_Aspect) {
			y = compute_coord(gui, widget.y, 0.5f, h, gui.resolution.y, true, parent_bounds.y, parent_bounds.w);
			x = compute_coord(gui, widget.x, y, w, gui.resolution.x, false, parent_bounds.x, parent_bounds.z);
		}
		else {
			x = compute_coord(gui, widget.x, 0.5f, w, gui.resolution.x, false, parent_bounds.x, parent_bounds.z);
			y = compute_coord(gui, widget.y, x, h, gui.resolution.y, true, parent_bounds.y, parent_bounds.w);
		}

		return { x, y, w, h };
	}

	static v4_f32 compute_widget_bounds(const GUI_internal& gui, GuiWidget& widget)
	{
		v4_f32 parent_bounds = { 0.5f, 0.5f, 1.f, 1.f };

		if (widget.parent != nullptr) {
			parent_bounds = compute_widget_bounds(gui, *widget.parent);
		}

		return compute_widget_bounds(gui, widget, parent_bounds);
	}

	SV_INLINE static v4_f32 compute_window_decoration_bounds(const GUI_internal& gui, GuiWindow& window, const v4_f32& container_bounds)
	{
		f32 aspect = gui.resolution.x / gui.resolution.y;

		v4_f32 decoration_bounds;
		decoration_bounds.x = container_bounds.x;
		decoration_bounds.y = container_bounds.y + container_bounds.w * 0.5f + (window.outline_size + window.decoration_height * 0.5f) * aspect;
		decoration_bounds.z = container_bounds.z + window.outline_size * 2.f;
		decoration_bounds.w = window.decoration_height * aspect;
		return decoration_bounds;
	}

	SV_INLINE static v4_f32 compute_window_closebutton_bounds(const GUI_internal& gui, GuiWindow& window, const v4_f32& container_bounds)
	{
		f32 aspect = gui.resolution.x / gui.resolution.y;

		f32 button_size = window.decoration_height * 0.2f;

		v4_f32 closebutton_bounds;
		closebutton_bounds.x = container_bounds.x + container_bounds.z * 0.5f - button_size;
		closebutton_bounds.y = container_bounds.y + container_bounds.w * 0.5f + (window.outline_size + window.decoration_height * 0.5f) * aspect;
		closebutton_bounds.z = button_size;
		closebutton_bounds.w = button_size * aspect;
		return closebutton_bounds;
	}

	SV_INLINE static v4_f32 compute_window_bounds(const GUI_internal& gui, GuiWindow& window, const v4_f32& container_bounds)
	{
		f32 aspect = gui.resolution.x / gui.resolution.y;

		v4_f32 bounds;
		bounds.x = container_bounds.x;
		bounds.y = container_bounds.y;
		bounds.z = container_bounds.z + window.outline_size * 2.f;
		bounds.w = container_bounds.w + (window.outline_size * 2.f) * aspect;
		return bounds;
	}

	// UPDATE

	static void update_window(GUI_internal& gui, GuiWindow& window)
	{
		v4_f32 container_bounds = compute_widget_bounds(gui, *window.container);
		v4_f32 decoration_bounds = compute_window_decoration_bounds(gui, window, container_bounds);

		if (mouse_in_bounds(gui, decoration_bounds)) {

			InputState mouse_state = input.mouse_buttons[MouseButton_Left];

			if (mouse_state && window.close_button && mouse_in_bounds(gui, compute_window_closebutton_bounds(gui, window, container_bounds))) {

				if (mouse_state == InputState_Released) {
					window.close_request = true;
				}
			}
			else if (mouse_state == InputState_Pressed) {

				gui.widget_focused = window.container;
				gui.dragged_begin_pos = gui.mouse_position - v2_f32{ container_bounds.x, container_bounds.y };
				gui.focus_action = 0u;
			}
		}
		else {

			v4_f32 bounds = compute_window_bounds(gui, window, container_bounds);
			if (mouse_in_bounds(gui, bounds) && !mouse_in_bounds(gui, container_bounds)) {

				InputState mouse_state = input.mouse_buttons[MouseButton_Left];

				if (mouse_state == InputState_Pressed) {

					gui.widget_focused = window.container;

					bool vertical = abs(gui.mouse_position.x - container_bounds.x) < container_bounds.z * 0.5f;
					bool horizontal = abs(gui.mouse_position.y - container_bounds.y) < container_bounds.w * 0.5f;

					bool top = gui.mouse_position.y > container_bounds.y + container_bounds.w * 0.5f;
					bool right = gui.mouse_position.x > container_bounds.x + container_bounds.z * 0.5f;

					if (vertical) {
						if (top) {
							gui.dragged_begin_pos.y = container_bounds.y - container_bounds.w * 0.5f;
							gui.focus_action = 1u;
						}
						else {
							gui.dragged_begin_pos.y = container_bounds.y + container_bounds.w * 0.5f;
							gui.focus_action = 2u;
						}
					}
					else if (horizontal) {
						if (right) {
							gui.dragged_begin_pos.x = container_bounds.x - container_bounds.z * 0.5f;
							gui.focus_action = 3u;
						}
						else {
							gui.dragged_begin_pos.x = container_bounds.x + container_bounds.z * 0.5f;
							gui.focus_action = 4u;
						}
					}
					else {

						if (top) {
							gui.dragged_begin_pos.y = container_bounds.y - container_bounds.w * 0.5f;
						}
						else {
							gui.dragged_begin_pos.y = container_bounds.y + container_bounds.w * 0.5f;
						}

						if (right) {
							gui.dragged_begin_pos.x = container_bounds.x - container_bounds.z * 0.5f;
						}
						else {
							gui.dragged_begin_pos.x = container_bounds.x + container_bounds.z * 0.5f;
						}

						if (top && !right) {
							gui.focus_action = 5u;
						}
						else if (top && right) {
							gui.focus_action = 6u;
						}
						else if (!top && !right) {
							gui.focus_action = 7u;
						}
						else if (!top && right) {
							gui.focus_action = 8u;
						}
					}
				}
			}
		}
	}

	static void update_widget(GUI_internal& gui, GuiWidget& widget, const v4_f32& parent_bounds)
	{
		if (!widget.enabled) return;

		switch (widget.type)
		{
		case GuiWidgetType_Container:
		{
			GuiContainer& container = *reinterpret_cast<GuiContainer*>(&widget);
			v4_f32 bounds = compute_widget_bounds(gui, container, parent_bounds);

			for (GuiWidget* son : container.sons) {
				update_widget(gui, *son, bounds);
			}

			// Scroll stuff
			if (container.vertical_scroll) {

				// Compute extensions

				if (container.sons.empty()) {

					container.up_extension = 0.f;
					container.down_extension = 0.f;
				}
				else {

					f32 min = f32_max;
					f32 max = f32_min;

					// There is no need to compute the bounds with the scroll
					v4_f32 temp_bounds = bounds;
					temp_bounds.y += container.vertical_offset;

					for (GuiWidget* son : container.sons) {

						v4_f32 son_bounds = compute_widget_bounds(gui, *son, temp_bounds);

						min = std::min(min, son_bounds.y - son_bounds.w * 0.5f);
						max = std::max(max, son_bounds.y + son_bounds.w * 0.5f);
					}

					f32 min_pos = bounds.y - bounds.w * 0.5f;
					f32 max_pos = bounds.y + bounds.w * 0.5f;

					container.up_extension = std::max(max - max_pos, 0.f);
					container.down_extension = std::max(min_pos - min, 0.f);
				}

				if (container_has_vertical_scroll(container)) {

					f32 wheel;

					if (mouse_in_bounds(gui, bounds) && gui.widget_focused == nullptr) {

						wheel = input.mouse_wheel;

						// Dragging scroll
						if (input.mouse_buttons[MouseButton_Left] && gui.mouse_position.x > (bounds.x + bounds.z * 0.5f - get_scroll_width(gui, bounds.z))) {

							gui.widget_focused = &container;
							gui.focus_action = 10u;
						}
					}
					else wheel = 0.f;

					f32 min_scroll = -container.down_extension;
					f32 max_scroll = container.up_extension;

					container.vertical_offset = std::min(std::max(wheel * 0.05f + container.vertical_offset, min_scroll), max_scroll);
				}
			}
		}
		break;

		case GuiWidgetType_Button:
		{
			GuiButton& button = *reinterpret_cast<GuiButton*>(&widget);
			v4_f32 bounds = compute_widget_bounds(gui, button, parent_bounds);

			// Mouse in button
			if (mouse_in_bounds(gui, bounds)) {

				switch (button.hover_state)
				{
				case HoverState_None:
				case HoverState_Leave:
					button.hover_state = HoverState_Enter;
					break;

				case HoverState_Enter:
					button.hover_state = HoverState_Hover;
					break;

				}

				if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed)
					gui.widget_focused = &button;
			}
			else {

				if (button.hover_state == HoverState_Enter || button.hover_state == HoverState_Hover) {

					button.hover_state = HoverState_Leave;
				}
				else if (button.hover_state == HoverState_Leave) {

					button.hover_state = HoverState_None;
				}
			}
		}
		break;

		case GuiWidgetType_Drag:
		case GuiWidgetType_Slider:
		case GuiWidgetType_TextField:
		case GuiWidgetType_Checkbox:
		{
			v4_f32 bounds = compute_widget_bounds(gui, widget, parent_bounds);

			// Mouse widget
			if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed && mouse_in_bounds(gui, bounds)) {

				gui.text_position = 0u;
				gui.widget_focused = &widget;
			}
		}
		break;

		case GuiWidgetType_Combobox:
		{
		    	v4_f32 bounds = compute_widget_bounds(gui, widget, parent_bounds);
			GuiCombobox& cbox = *reinterpret_cast<GuiCombobox*>(&widget);

			InputState input_state = input.mouse_buttons[MouseButton_Left];
			
			if (cbox.open) {

			    // Close combobox
			    if (input_state == InputState_Pressed) cbox.open = false;

			    
			}
			else {
			    
			    // Mouse widget
			    if (input_state == InputState_Released && mouse_in_bounds(gui, bounds)) {
				cbox.open = true;
			    }
			}
		}
		break;

		}
	}

	void gui_update(GUI* gui_, f32 width, f32 height)
	{
		PARSE_GUI();

		gui.resolution = { f32(width), f32(height) };

		// Init update
		gui.mouse_position = input.mouse_position + 0.5f;

		if (!input.unused) return;
		
		// Update windows and widgets
		for (auto& pool : gui.windows) {
			for (GuiWindow& window : pool) {
				update_window(gui, window);
			}
		}
		for (GuiWidget* root : gui.root) {
			update_widget(gui, *root, { 0.5f, 0.5f, 1.f, 1.f });
		}

		// Update focused widget
		if (gui.widget_focused != nullptr) {

			input.unused = false;

			switch (gui.widget_focused->type)
			{

			case GuiWidgetType_Container:
			{
				GuiContainer& container = *reinterpret_cast<GuiContainer*>(gui.widget_focused);

				InputState mouse_state = input.mouse_buttons[MouseButton_Left];

				if (mouse_state == InputState_Released || mouse_state == InputState_None) {
					gui.widget_focused = nullptr;
				}
				else if (gui.focus_action < 9u) {

					v2_f32 move = gui.mouse_position - gui.dragged_begin_pos;

					container.x.constraint = GuiConstraint_Relative;
					container.x.alignment = GuiCoordAlignment_Center;
					container.y.constraint = GuiConstraint_Relative;
					container.y.alignment = GuiCoordAlignment_Center;

					switch (gui.focus_action)
					{
					case 0:
						container.x.value = move.x;
						container.y.value = move.y;
						break;

						// RESIZE SIDES
					case 1: // top
					{
						move.y = std::max(0.0f, move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y + move.y * 0.5f;
					}
					break;

					case 2: // bottom
					{
						move.y = std::max(0.0f, -move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y - move.y * 0.5f;
					}
					break;

					case 3: // right
					{
						move.x = std::max(0.05f, move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x + move.x * 0.5f;
					}
					break;

					case 4: // left
					{
						move.x = std::max(0.05f, -move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x - move.x * 0.5f;
					}
					break;

					// RESIZE CORNERS
					case 5: // top - left
					{
						move.y = std::max(0.0f, move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y + move.y * 0.5f;
						move.x = std::max(0.05f, -move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x - move.x * 0.5f;
					}
					break;

					case 6: // top - right
					{
						move.y = std::max(0.0f, move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y + move.y * 0.5f;
						move.x = std::max(0.05f, move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x + move.x * 0.5f;
					}
					break;

					case 7: // bottom - left
					{
						move.y = std::max(0.0f, -move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y - move.y * 0.5f;
						move.x = std::max(0.05f, -move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x - move.x * 0.5f;
					}
					break;

					case 8: // bottom - right
					{
						move.y = std::max(0.0f, -move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y - move.y * 0.5f;
						move.x = std::max(0.05f, move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x + move.x * 0.5f;
					}
					break;

					}
				}
				else {

					v4_f32 bounds = compute_widget_bounds(gui, container);

					f32 button_height = get_button_scroll_height(container, bounds.w);
					f32 button_space = bounds.w - button_height;

					f32 prop = (gui.mouse_position.y - (bounds.y - bounds.w * 0.5f + button_height * 0.5f)) / button_space;
					prop = std::max(std::min(prop, 1.f), 0.f);

					container.vertical_offset = prop * (container.up_extension + container.down_extension) - container.down_extension;
				}
			}
			break;

			case GuiWidgetType_Button:
			case GuiWidgetType_Checkbox:
			{
				GuiButton& button = *reinterpret_cast<GuiButton*>(gui.widget_focused);
				v4_f32 bounds = compute_widget_bounds(gui, *gui.widget_focused);
				InputState state = input.mouse_buttons[MouseButton_Left];

				if (!mouse_in_bounds(gui, bounds) || state == InputState_None) {

					gui.widget_focused = nullptr;
				}

				else if (state == InputState_Released && gui.widget_focused->type == GuiWidgetType_Checkbox) {

					GuiCheckbox& cbox = *reinterpret_cast<GuiCheckbox*>(gui.widget_focused);
					cbox.active = !cbox.active;
				}
			}
			break;

			case GuiWidgetType_Slider:
			{
				GuiSlider& slider = *reinterpret_cast<GuiSlider*>(gui.widget_focused);

				InputState mouse_state = input.mouse_buttons[MouseButton_Left];

				if (mouse_state == InputState_Released || mouse_state == InputState_None) {
					gui.widget_focused = nullptr;
				}
				else {

					v4_f32 bounds = compute_widget_bounds(gui, slider);
					f32 min_pos = bounds.x - bounds.z * 0.5f;

					f32 normalized_value = (gui.mouse_position.x - min_pos) / bounds.z;

					slider.value = slider.min + normalized_value * (slider.max - slider.min);
					slider.value = std::max(slider.min, std::min(slider.value, slider.max));
				}
			}
			break;

			case GuiWidgetType_Drag:
			{
				GuiDrag& drag = *reinterpret_cast<GuiDrag*>(gui.widget_focused);

				InputState mouse_state = input.mouse_buttons[MouseButton_Left];

				if (mouse_state == InputState_Released || mouse_state == InputState_None) {
					gui.widget_focused = nullptr;
				}
				else {

					f32 adv = input.mouse_dragged.x * gui.resolution.x * drag.advance;
					drag.value = drag.value + adv;
				}
			}
			break;

			case GuiWidgetType_TextField:
			{
				GuiTextField& field = *reinterpret_cast<GuiTextField*>(gui.widget_focused);

				// Assert that the position is lower than the text size
				gui.text_position = std::min(gui.text_position, u32(field.text.size()));

				// Move text position
				if (input.keys[Key_Left] == InputState_Pressed) {
					gui.text_position = (gui.text_position == 0u) ? 0u : (gui.text_position - 1u);
				}
				if (input.keys[Key_Right] == InputState_Pressed) {
					gui.text_position = std::min(gui.text_position + 1u, u32(field.text.size()));
				}

				// Text commands
				for (TextCommand command : input.text_commands) {

					switch (command)
					{
					case TextCommand_DeleteLeft:
						if (gui.text_position != 0u) {
							--gui.text_position;
							field.text.erase(field.text.begin() + gui.text_position);
						}
						break;
					case TextCommand_DeleteRight:
						if (gui.text_position < field.text.size()) {
							field.text.erase(field.text.begin() + gui.text_position);
						}
						break;
					case TextCommand_Enter:
					case TextCommand_Escape:
						gui.widget_focused = nullptr;
						break;
					}
				}

				// Append text
				if (input.text.size()) {

					field.text.insert(field.text.begin() + gui.text_position, input.text.begin(), input.text.end());
					gui.text_position += u32(input.text.size());
				}
			}
			break;

			}
		}
	}

	static constexpr u32 MAX_FLOAT_CHARS = 20u;

	static void float_to_string(f32 n, char* str, u32* size, const u32 buffer_size)
	{
		bool negative = n < 0.f;

		constexpr f32 DECIMAL_MULT = 10000000.f;

		n = abs(n);
		u32 n0 = u32(n);
		u32 n1 = u32((n - i32(n)) * DECIMAL_MULT + DECIMAL_MULT);

		// Add integer values

		char* it = str;
		char* end = str + buffer_size;

		if (negative) {
			*it = '-';
			++it;
		}

		char* valid_ptr = it;

		if (n0 == 0u) {

			*it = '0';
			++it;
		}
		else {

			while (it != end) {

				if (n0 == 0u) {
					break;
				}

				u32 number = n0 % 10u;
				n0 /= 10u;

				char c = number + '0';
				*it = c;
				++it;

				if (n == 0u) {
					if (it == end) break;

					*it = '.';
					++it;
				}
			}
		}

		// Swap values
		char* it0 = it - 1U;

		while (valid_ptr < it0) {

			char aux = *it0;
			*it0 = *valid_ptr;
			*valid_ptr = aux;
			++valid_ptr;
			--it0;
		}

		// Add decimal values

		if (n1 != u32(DECIMAL_MULT) && it != end) {

			*it = '.';
			++it;

			valid_ptr = it;

			while (it != end) {

				if (n1 == 1u) {
					break;
				}

				u32 number = n1 % 10u;
				n1 /= 10u;

				char c = number + '0';
				*it = c;
				++it;
			}
		}

		// Swap values
		it0 = it - 1U;

		while (valid_ptr < it0) {

			char aux = *it0;
			*it0 = *valid_ptr;
			*valid_ptr = aux;
			++valid_ptr;
			--it0;
		}

		// Remove unnecesary 0
		--it;
		while (*it == '0' && it > (str + 1u)) --it;
		++it;

		*size = it - str;
	}

	static void draw_widget(GUI_internal& gui, GPUImage* offscreen, GuiWidget& widget, const v4_f32& parent_bounds, CommandList cmd)
	{
		// TODO: Should use the alpha and inherited_alpha value
		if (!widget.enabled) return;

		v4_f32 bounds = compute_widget_bounds(gui, widget, parent_bounds);

		v2_f32 pos = v2_f32{ bounds.x, bounds.y } *2.f - 1.f;
		v2_f32 size = v2_f32{ bounds.z, bounds.w } *2.f;

		switch (widget.type)
		{

		case GuiWidgetType_Container:
		{
			GuiContainer& container = *reinterpret_cast<GuiContainer*>(&widget);

			begin_debug_batch(cmd);

			draw_debug_quad(pos.getVec3(), size, widget.color, cmd);

			if (container_has_vertical_scroll(container)) {

				f32 scroll_width = get_scroll_width(gui, size.x) * 2.f;

				v2_f32 scroll_pos = pos;
				scroll_pos.x = (scroll_pos.x + size.x * 0.5f) - scroll_width * 0.5f;

				draw_debug_quad(scroll_pos.getVec3(), { scroll_width, size.y }, Color::Red(), cmd);

				f32 min_scroll = -container.down_extension;
				f32 max_scroll = container.up_extension;

				f32 prop = (container.vertical_offset + container.down_extension) / (container.up_extension + container.down_extension);

				f32 button_height = get_button_scroll_height(container, size.y);
				f32 button_space = size.y - button_height;

				f32 button_y_pos = pos.y - size.y * 0.5f + button_height * 0.5f + prop * button_space;

				draw_debug_quad({ scroll_pos.x, button_y_pos, 0.f }, { scroll_width, button_height }, Color::Blue(), cmd);
			}

			end_debug_batch(offscreen, XMMatrixIdentity(), cmd);

			for (GuiWidget* son : container.sons) {
				draw_widget(gui, offscreen, *son, bounds, cmd);
			}
		}
		break;

		case GuiWidgetType_Slider:
		{
			begin_debug_batch(cmd);

			GuiSlider& slider = *reinterpret_cast<GuiSlider*>(&widget);

			f32 normalized_value = (slider.value - slider.min) / (slider.max - slider.min);
			f32 subpos = pos.x - size.x * 0.5f + normalized_value * size.x;

			draw_debug_quad(pos.getVec3(), size, widget.color, cmd);
			draw_debug_quad({ subpos, pos.y, 0.f }, { size.x * 0.05f, size.y * 1.1f }, slider.button_color, cmd);

			end_debug_batch(offscreen, XMMatrixIdentity(), cmd);
		}
		break;

		case GuiWidgetType_Button:
		{
			GuiButton& button = *reinterpret_cast<GuiButton*>(&widget);

			begin_debug_batch(cmd);

			Color color;

			if (button.hover_state) color = button.hover_color;
			else color = button.color;

			draw_debug_quad(pos.getVec3(), size, color, cmd);
			end_debug_batch(offscreen, XMMatrixIdentity(), cmd);

			if (button.text.size()) {

				// TODO: Should use information about the largest character in the font
				f32 text_y = pos.y + size.y * 0.6f;
				draw_text(offscreen, button.text.c_str(), button.text.size(), pos.x - size.x * 0.5f, text_y, size.x, 1u, size.y, gui.resolution.x / gui.resolution.y, TextAlignment_Center, nullptr, cmd);
			}
		}
		break;

		case GuiWidgetType_Drag:
		{
			GuiDrag& drag = *reinterpret_cast<GuiDrag*>(&widget);

			begin_debug_batch(cmd);

			draw_debug_quad(pos.getVec3(), size, drag.color, cmd);

			end_debug_batch(offscreen, XMMatrixIdentity(), cmd);

			char strbuff[MAX_FLOAT_CHARS + 1u];
			u32 strsize;
			float_to_string(drag.value, strbuff, &strsize, MAX_FLOAT_CHARS);

			// TODO: Should use information about the largest character in the font
			f32 text_y = pos.y + size.y * 0.6f;
			draw_text(offscreen, strbuff, strsize, pos.x - size.x * 0.5f, text_y, size.x, 1u, size.y, gui.resolution.x / gui.resolution.y, TextAlignment_Center, nullptr, cmd);
		}
		break;

		case GuiWidgetType_TextField:
		{
			GuiTextField& field = *reinterpret_cast<GuiTextField*>(&widget);

			begin_debug_batch(cmd);
			draw_debug_quad(pos.getVec3(), size, widget.color, cmd);
			end_debug_batch(offscreen, XMMatrixIdentity(), cmd);

			if (field.text.size()) {

				// TODO: Should use information about the largest character in the font
				f32 text_y = pos.y + size.y * 0.6f;
				f32 text_x = pos.x - size.x * 0.5f;
				f32 aspect = gui.resolution.x / gui.resolution.y;

				draw_text(offscreen, field.text.c_str(), field.text.size(), text_x, text_y, size.x, 1u, size.y, aspect, TextAlignment_Left, nullptr, cmd);

				if (&widget == gui.widget_focused && sin(f32(timer_now()) * 5.f) > 0.f) {

					f32 line_x = text_x + compute_text_width(field.text.c_str(), gui.text_position, size.y, aspect, nullptr);

					begin_debug_batch(cmd);
					draw_debug_line({ line_x, text_y, 0.f }, { line_x, text_y - size.y, 0.f }, Color::White(), cmd);
					end_debug_batch(offscreen, XMMatrixIdentity(), cmd);
				}
			}
		}
		break;

		case GuiWidgetType_Checkbox:
		{
			GuiCheckbox& cbox = *reinterpret_cast<GuiCheckbox*>(&widget);

			begin_debug_batch(cmd);
			draw_debug_quad(pos.getVec3(), size, widget.color, cmd);

			if (cbox.active) {
				draw_debug_quad(pos.getVec3(), size * 0.7f, cbox.color_check, cmd);
			}

			end_debug_batch(offscreen, XMMatrixIdentity(), cmd);
		}
		break;

		case GuiWidgetType_Label:
		{
			GuiLabel& label = *reinterpret_cast<GuiLabel*>(&widget);

			begin_debug_batch(cmd);
			draw_debug_quad(pos.getVec3(), size, widget.color, cmd);
			end_debug_batch(offscreen, XMMatrixIdentity(), cmd);

			if (label.text.size()) {

				// TODO: Should use information about the largest character in the font
				f32 text_y = pos.y + size.y * 0.6f;
				f32 text_x = pos.x - size.x * 0.5f;
				f32 aspect = gui.resolution.x / gui.resolution.y;

				draw_text(offscreen, label.text.c_str(), label.text.size(), text_x, text_y, size.x, 1u, size.y, aspect, TextAlignment_Center, nullptr, cmd);
			}
		}
		break;

		default:
			begin_debug_batch(cmd);
			draw_debug_quad(pos.getVec3(), size, widget.color, cmd);
			end_debug_batch(offscreen, XMMatrixIdentity(), cmd);
			break;
		}
	}

	void gui_render(GUI* gui_, GPUImage* offscreen, CommandList cmd)
	{
		PARSE_GUI();

		// prepare
		graphics_viewport_set(offscreen, 0u, cmd);
		graphics_scissor_set(offscreen, 0u, cmd);
		graphics_depthstencilstate_unbind(cmd);
		graphics_blendstate_unbind(cmd);
		graphics_rasterizerstate_unbind(cmd);

		

		// Draw windows
		for (auto& pool : gui.windows) {
			for (GuiWindow& window : pool) {

				begin_debug_batch(cmd);

				v4_f32 container_bounds = compute_widget_bounds(gui, *window.container);
				v4_f32 bounds = compute_window_bounds(gui, window, container_bounds);
				v4_f32 decoration_bounds = compute_window_decoration_bounds(gui, window, container_bounds);

				v2_f32 dec_pos = v2_f32{ decoration_bounds.x, decoration_bounds.y };
				dec_pos = dec_pos * 2.f - 1.f;

				v2_f32 dec_size = v2_f32{ decoration_bounds.z, decoration_bounds.w };
				dec_size = dec_size * 2.f;

				draw_debug_quad(dec_pos.getVec3(), dec_size, window.decoration_color, cmd);

				v2_f32 pos = v2_f32{ bounds.x, bounds.y };
				pos = pos * 2.f - 1.f;

				v2_f32 size = v2_f32{ bounds.z, bounds.w };
				size = size * 2.f;

				draw_debug_quad(pos.getVec3(), size, window.color, cmd);
				
				if (window.close_button) {

					v4_f32 closebutton_bounds = compute_window_closebutton_bounds(gui, window, container_bounds);
					
					v2_f32 cb_pos = v2_f32{ closebutton_bounds.x, closebutton_bounds.y };
					cb_pos = cb_pos * 2.f - 1.f;

					v2_f32 cb_size = v2_f32{ closebutton_bounds.z, closebutton_bounds.w };
					cb_size = cb_size * 2.f;

					draw_debug_quad(cb_pos.getVec3(), cb_size, Color::Red(), cmd);
				}

				end_debug_batch(offscreen, XMMatrixIdentity(), cmd);

				if (window.title.size())
					draw_text(offscreen, window.title.c_str(), window.title.size(), dec_pos.x - dec_size.x * 0.5f + 0.03f, dec_pos.y + dec_size.y * 0.5f, dec_size.x, 1u, dec_size.y * 0.5f, gui.resolution.x / gui.resolution.y, TextAlignment_Left, 0, cmd);
			}
		}

		// Draw widgets

		for (GuiWidget* root : gui.root) {
			draw_widget(gui, offscreen, *root, { 0.5f, 0.5f, 1.f, 1.f }, cmd);
		}
	}

	GuiWidget* gui_widget_create(GUI* gui_, GuiWidgetType widget_type, GuiContainer* container)
	{
		PARSE_GUI();

		GuiWidget* widget;

		// Allocate widget
		switch (widget_type)
		{
		case GuiWidgetType_Container:
			widget = &gui.containers.create();
			break;

		case GuiWidgetType_Button:
			widget = &gui.buttons.create();
			break;

		case GuiWidgetType_Slider:
			widget = &gui.sliders.create();
			break;

		case GuiWidgetType_Drag:
			widget = &gui.drags.create();
			break;

		case GuiWidgetType_TextField:
			widget = &gui.textfields.create();
			break;

		case GuiWidgetType_Checkbox:
			widget = &gui.checkboxes.create();
			break;

		case GuiWidgetType_Combobox:
			widget = &gui.comboboxes.create();
			break;

		case GuiWidgetType_Label:
			widget = &gui.labels.create();
			break;

		default:
			SV_LOG_ERROR("Unknown widget type: %u", widget_type);
			return nullptr;
		}

		// Set parent
		if (container)
			container->sons.push_back(widget);
		else
			gui.root.push_back(widget);
		widget->parent = container;


		return widget;
	}

	void gui_widget_destroy(GUI* gui_, GuiWidget* widget)
	{
		if (widget == nullptr) return;
		PARSE_GUI();

		// Remove son reference from the parent
		if (widget->parent) {

			GuiContainer& parent = *widget->parent;

			for (auto it = parent.sons.begin(); it != parent.sons.end(); ++it) {
				if (*it == widget) {
					parent.sons.erase(it);
					break;
				}
			}
		}
		// Remove from root
		else {
			for (auto it = gui.root.begin(); it != gui.root.end(); ++it) {
				if (*it == widget) {
					gui.root.erase(it);
					break;
				}
			}
		}

		// Deallocate widget
		switch (widget->type)
		{
		case GuiWidgetType_Container:
		{
			GuiContainer& container = *reinterpret_cast<GuiContainer*>(widget);

			for (GuiWidget* son : container.sons) {

				son->parent = nullptr;
				gui_widget_destroy(gui_, son);
			}

			gui.containers.destroy(container);
		}
		break;

		case GuiWidgetType_Button:
			gui.buttons.destroy(*reinterpret_cast<GuiButton*>(widget));
			break;

		case GuiWidgetType_Slider:
			gui.sliders.destroy(*reinterpret_cast<GuiSlider*>(widget));
			break;

		case GuiWidgetType_Drag:
			gui.drags.destroy(*reinterpret_cast<GuiDrag*>(widget));
			break;

		case GuiWidgetType_TextField:
			gui.textfields.destroy(*reinterpret_cast<GuiTextField*>(widget));
			break;

		case GuiWidgetType_Checkbox:
			gui.checkboxes.destroy(*reinterpret_cast<GuiCheckbox*>(widget));
			break;

		case GuiWidgetType_Combobox:
			gui.comboboxes.destroy(*reinterpret_cast<GuiCombobox*>(widget));
			break;

		case GuiWidgetType_Label:
			gui.labels.destroy(*reinterpret_cast<GuiLabel*>(widget));
			break;
		}
	}

	GuiWidget* gui_widget_focused(GUI* gui_)
	{
		PARSE_GUI();
		return gui.widget_focused;
	}

	GuiWindow* gui_window_create(GUI* gui_)
	{
		PARSE_GUI();
		GuiWindow& window = gui.windows.create();

		window.container = gui_container_create(gui_);

		window.container->x.value = 0.15f;
		window.container->x.constraint = GuiConstraint_Relative;
		window.container->x.alignment = GuiCoordAlignment_Center;
		window.container->y.value = 0.7f;
		window.container->y.constraint = GuiConstraint_Relative;
		window.container->y.alignment = GuiCoordAlignment_Center;
		window.container->w.value = 0.2f;
		window.container->w.constraint = GuiConstraint_Relative;
		window.container->h.value = 0.4f;
		window.container->h.constraint = GuiConstraint_Relative;

		return &window;
	}

	void gui_window_destroy(GUI* gui_, GuiWindow* window)
	{
		PARSE_GUI();

		gui_widget_destroy(gui_, window->container);
		gui.windows.destroy(*window);
	}

	const std::vector<GuiWidget*>& gui_root_get(GUI* gui_)
	{
		PARSE_GUI();
		return gui.root;
	}

	////////////////////////////////////////////////////////////////////////// IMMEDIATE GUI //////////////////////////////////////////////////////////////////////////

	struct IGUI_WindowInfo {

		bool closed = false;
		v4_f32 bounds;
		f32 vertical_offset;
		GuiWindow* window;
		f32 yoff;

	};

	struct IGUI_WidgetInfo {

		GuiWidget* widget;

		union {

			struct {
				bool clicked;
			} button;

			struct {
				bool dragging;
				f32* value;
			} drag;

		};

	};

	struct IGUI_Style {

		Color color_window_decoration = Color::Black();
		Color color_window_background = Color::White(90u);
		Color color_window_border = Color::Black(150u);
		Color color_button = Color::Orange(100u);
		Color color_button_hover = Color::Orange(255u);

	};

	struct IGUI_internal {

		GUI* gui;

		IGUI_Style style;

		IGUI_WindowInfo* current_window = nullptr;

		IGUI_WindowInfo* window_focused = nullptr;
		IGUI_WidgetInfo* widget_focused = nullptr;

		std::unordered_map<const char*, IGUI_WindowInfo> window_info;
		std::unordered_map<u64, IGUI_WidgetInfo> widget_info;

	};
#define PARSE_IGUI() sv::IGUI_internal& igui = *reinterpret_cast<sv::IGUI_internal*>(igui_);

	IGUI* igui_create()
	{
		IGUI_internal& igui = *new IGUI_internal();

		igui.gui = gui_create();

		return reinterpret_cast<IGUI*>(&igui);
	}

	void igui_destroy(IGUI* igui_)
	{
		PARSE_IGUI();

		gui_destroy(igui.gui);

		delete& igui;
	}

	void igui_begin(IGUI* igui_)
	{
		PARSE_IGUI();

		gui_clear(igui.gui);
	}

	void igui_end(IGUI* igui_, f32 width, f32 height)
	{
		PARSE_IGUI();

		GUI_internal* gui = reinterpret_cast<GUI_internal*>(igui.gui);

		// Set widget focused
		if (igui.widget_focused) {

			gui->widget_focused = igui.widget_focused->widget;
		}
		else if (igui.window_focused) {

			gui->widget_focused = igui.window_focused->window->container;
		}

		gui_update(igui.gui, width, height);

		// Update windows
		for (auto& it : igui.window_info) {

			IGUI_WindowInfo& info = it.second;
			if (info.window) {

				info.bounds.x = info.window->container->x.value;
				info.bounds.y = info.window->container->y.value;
				info.bounds.z = info.window->container->w.value;
				info.bounds.w = info.window->container->h.value;
				info.vertical_offset = info.window->container->vertical_offset;
				info.closed = info.window->close_request;

				info.window = nullptr;
			}
		}

		// Update focused widget
		GuiWidget* focus = gui_widget_focused(igui.gui);
		igui.widget_focused = nullptr;
		igui.window_focused = nullptr;

		if (focus) {

			u64 id = focus->user_id;

			auto it = igui.widget_info.find(id);
			if (it != igui.widget_info.end()) {

				IGUI_WidgetInfo& info = it->second;
				igui.widget_focused = &info;

				switch (info.widget->type)
				{
				case GuiWidgetType_Button:
				{
					if (input.mouse_buttons[MouseButton_Left] == InputState_Released) {
						info.button.clicked = true;
					}
				}
				break;

				case GuiWidgetType_Drag:
				{
					info.drag.dragging = true;
					*info.drag.value = reinterpret_cast<GuiDrag*>(info.widget)->value;
				}
				break;
				}
			}
			else {

				auto it = igui.window_info.find((const char*)id);
				if (it != igui.window_info.end()) {

					IGUI_WindowInfo& info = it->second;
					igui.window_focused = &info;
				}
			}
		}
	}

	void igui_render(IGUI* igui_, GPUImage* offscreen, CommandList cmd)
	{
		PARSE_IGUI();

		gui_render(igui.gui, offscreen, cmd);
	}

	SV_INLINE IGUI_WindowInfo* create_window_info(IGUI_internal& igui, const char* name)
	{
		IGUI_WindowInfo* info;

		auto it = igui.window_info.find(name);
		if (it == igui.window_info.end()) {

			IGUI_WindowInfo i;
			i.bounds = { 0.5f, 0.5f, 0.1f, 0.2f };
			i.vertical_offset = 0.f;

			igui.window_info[name] = i;
			info = &igui.window_info[name];
		}
		else info = &it->second;

		return info;
	}

	SV_INLINE IGUI_WindowInfo* get_window_info(IGUI_internal& igui, const char* name)
	{
		auto it = igui.window_info.find(name);
		if (it != igui.window_info.end()) {

			return &it->second;
		}

		return nullptr;
	}

	bool igui_begin_window(IGUI* igui_, const char* name)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window == nullptr);
		
		IGUI_WindowInfo* info = create_window_info(igui, name);
		if (info->closed) return false;

		GuiWindow* window = gui_window_create(igui.gui);

		info->window = window;
		info->yoff = 5.f;

		window->container->x = { info->bounds.x, GuiConstraint_Relative, GuiCoordAlignment_Center };
		window->container->y = { info->bounds.y, GuiConstraint_Relative, GuiCoordAlignment_Center };
		window->container->w = { info->bounds.z, GuiConstraint_Relative };
		window->container->h = { info->bounds.w, GuiConstraint_Relative };
		window->container->vertical_offset = info->vertical_offset;
		window->container->vertical_scroll = true;
		window->container->user_id = (u64)name;
		window->container->color = igui.style.color_window_background;
		window->color = igui.style.color_window_border;
		window->decoration_color = igui.style.color_window_decoration;
		window->title = name;
		window->close_button = true;

		igui.current_window = info;

		return true;
	}

	void igui_end_window(IGUI* igui_)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window);

		igui.current_window = nullptr;
	}

	void igui_open_window(IGUI* igui_, const char* name)
	{
		PARSE_IGUI();
		IGUI_WindowInfo* info = get_window_info(igui, name);
		if (info) info->closed = false;
	}

	void igui_close_window(IGUI* igui_, const char* name)
	{
		PARSE_IGUI();
		IGUI_WindowInfo* info = get_window_info(igui, name);
		if (info) info->closed = true;
	}

	bool igui_is_open_window(IGUI* igui_, const char* name)
	{
		PARSE_IGUI();
		IGUI_WindowInfo* info = get_window_info(igui, name);
		return info ? !info->closed : false;
	}

	SV_INLINE static bool get_widget_info(IGUI_internal& igui, u64 id, IGUI_WidgetInfo** pinfo)
	{
		auto it = igui.widget_info.find(id);
		if (it == igui.widget_info.end()) {

			igui.widget_info[id] = {};
			*pinfo = &igui.widget_info[id];

			return true;
		}
		else *pinfo = &it->second;

		return false;
	}

	void igui_same_line(IGUI* igui_)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window);
		SV_LOG("TODO-> same line");
	}
	
	bool igui_button(IGUI* igui_, u64 id, const char* text)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window);

		GuiButton* button = gui_button_create(igui.gui, igui.current_window->window->container);

		IGUI_WidgetInfo* info;
		if (get_widget_info(igui, id, &info)) {

			info->button.clicked = false;
		}

		info->widget = button;

		constexpr f32 BUTTON_HEIGHT = 20.f;

		button->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		button->y = { igui.current_window->yoff, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
		button->w = { 0.6f, GuiConstraint_Relative };
		button->h = { BUTTON_HEIGHT, GuiConstraint_Pixel };
		button->color = igui.style.color_button;
		button->hover_color = igui.style.color_button_hover;
		button->user_id = id;
		button->text = text;
		button->inherited_alpha = 1.f;

		igui.current_window->yoff += BUTTON_HEIGHT + 3.f;

		bool result = info->button.clicked;
		info->button.clicked = false;
		return result;
	}

	bool igui_drag(IGUI* igui_, u64 id, const char* text, f32* n, f32 adv)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window);

		GuiDrag* drag = gui_drag_create(igui.gui, igui.current_window->window->container);

		IGUI_WidgetInfo* info;
		if (get_widget_info(igui, id, &info)) {

			info->drag.dragging = false;
		}

		info->widget = drag;
		info->drag.value = n;

		constexpr f32 DRAG_HEIGHT = 20.f;

		drag->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		drag->y = { igui.current_window->yoff, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
		drag->w = { 0.6f, GuiConstraint_Relative };
		drag->h = { DRAG_HEIGHT, GuiConstraint_Pixel };
		drag->color = Color::Red();
		drag->user_id = id;
		drag->value = *n;
		drag->advance = adv;

		igui.current_window->yoff += DRAG_HEIGHT + 3.f;

		bool result = info->drag.dragging;
		info->drag.dragging = false;
		return result;
	}

	void igui_text(IGUI* igui_, u64 id, const char* text)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window);

		GuiLabel* label = gui_label_create(igui.gui, igui.current_window->window->container);

		IGUI_WidgetInfo* info;
		if (get_widget_info(igui, id, &info)) {

			info->drag.dragging = false;
		}

		info->widget = label;

		constexpr f32 DRAG_HEIGHT = 20.f;

		label->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		label->y = { igui.current_window->yoff, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
		label->w = { 0.6f, GuiConstraint_Relative };
		label->h = { DRAG_HEIGHT, GuiConstraint_Pixel };
		label->color = Color::Red();
		label->user_id = id;
		label->text = text;

		igui.current_window->yoff += DRAG_HEIGHT + 3.f;
	}


}
