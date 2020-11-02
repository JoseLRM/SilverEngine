#pragma once

#include "core_editor.h"

namespace sv {

	class Panel {
	public:
		Panel();
		virtual ~Panel() = default;

		bool display();

		void show();
		void hide();
		void close();

		inline const vec2u& get_size() const noexcept { return m_Size; }
		inline bool isEnabled() const noexcept { return m_Enabled; }
		inline bool isFocused() const noexcept { return m_Focused; }
		inline bool isVisible() const noexcept { return m_Visible; }
		inline const char* getName() const noexcept { return m_Name.c_str(); }

		inline void setName(const char* name) { m_Name = name; }

	protected:
		virtual ImGuiWindowFlags getWindowFlags() { return ImGuiWindowFlags_None; }
		virtual void beginDisplay() {}
		virtual bool onDisplay() = 0;
		virtual void endDisplay() {}

	private:
		bool m_Enabled = true;
		bool m_Focused = false;
		bool m_Visible = false;
		vec2u m_Size;
		std::string m_Name;

	};

}