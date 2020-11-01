#include "core.h"

#include <iostream>
#include <stdarg.h>

#include "platform/platform_impl.h"

#include "rendering/material_system/material_system_internal.h"
#include "rendering/debug_renderer/debug_renderer_internal.h"
#include "rendering/sprite_renderer/sprite_renderer_internal.h"
#include "platform/graphics/graphics_internal.h"
#include "platform/input/input_internal.h"
#include "platform/window/window_internal.h"
#include "simulation/task_system/task_system_internal.h"
#include "simulation/asset_system/asset_system_internal.h"
#include "simulation/animator/animator_internal.h"
#include "simulation/event_system/event_system_internal.h"
#include "simulation/sprite_animator/sprite_animator_internal.h"

namespace sv {

	///////////////////////////////////////////////// PLATFORM /////////////////////////////////////////////////

	int show_message(const wchar* title, const wchar* content, MessageStyleFlags style)
	{
#ifdef SV_PLATFORM_WIN
		UINT flags = 0u;

		if (style & MessageStyle_IconInfo) flags |= MB_ICONINFORMATION;
		else if (style & MessageStyle_IconWarning) flags |= MB_ICONWARNING;
		else if (style & MessageStyle_IconError) flags |= MB_ICONERROR;

		if (style & MessageStyle_Ok) flags |= MB_OK;
		else if (style & MessageStyle_OkCancel) flags |= MB_OKCANCEL;
		else if (style & MessageStyle_YesNo) flags |= MB_YESNO;
		else if (style & MessageStyle_YesNoCancel) flags |= MB_YESNOCANCEL;

		int res = MessageBox(0, content, title, flags);

		if (style & MessageStyle_Ok) {
			if (res == IDOK) return 0;
			else return -1;
		}
		else if (style & MessageStyle_OkCancel) {
			switch (res)
			{
			case IDOK:
				return 0;
			case IDCANCEL:
				return 1;
			default:
				return -1;
			}
		}
		else if (style & MessageStyle_YesNo) {
			switch (res)
			{
			case IDYES:
				return 0;
			case IDNO:
				return 1;
			default:
				return -1;
			}
		}
		else if (style & MessageStyle_YesNoCancel) {
			switch (res)
			{
			case IDYES:
				return 0;
			case IDNO:
				return 1;
			case IDCANCEL:
				return 2;
			default:
				return -1;
			}
		}

		return res;
#endif
	}

	///////////////////////////////////////////////// ASSERTION /////////////////////////////////////////////////

	void throw_assertion(const char* content, ui32 line, const char* file)
	{
#ifdef SV_ENABLE_LOGGING
		__internal__do_not_call_this_please_or_you_will_die__console_log(4u, "[ASSERTION] '%s', file: '%s', line: %u", content, file, line);
#endif
		std::wstringstream ss;
		ss << L"'";
		ss << parse_wstring(content);
		ss << L"'. File: '";
		ss << parse_wstring(file);
		ss << L"'. Line: " << line;
		ss << L". Do you want to continue?";

		if (sv::show_message(L"ASSERTION!!", ss.str().c_str(), sv::MessageStyle_IconError | sv::MessageStyle_YesNo) == 1)
		{
			exit(1u);
		}
	}

	///////////////////////////////////////////////// LOGGING /////////////////////////////////////////////////

#ifndef SV_ENABLE_LOGGING

	Result logging_initialize()
	{
#ifdef SV_PLATFORM_WIN
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
		return Result_Success;
	}

	Result logging_close()
	{
		return Result_Success;
	}


#else
	static std::mutex g_LogMutex;
	std::ofstream g_LogFile;

	std::string date_string(const Date& date)
	{
		std::stringstream stream;
		stream << '[';
		if (date.hour < 10u) stream << '0';
		stream << date.hour << ':';
		if (date.minute < 10u) stream << '0';
		stream << date.minute << ':';
		if (date.second < 10u) stream << '0';
		stream << date.second << ']';
		std::string res = stream.str();
		return res;
	}

	Result logging_initialize()
	{
#ifdef SV_PLATFORM_WIN
		ShowWindow(GetConsoleWindow(), SW_SHOWDEFAULT);
#endif

		std::string logFolder = "logs/";
#ifdef SV_RES_PATH
		logFolder = SV_RES_PATH + logFolder;
#endif
		Date date = timer_date();

		std::string day;
		if (date.day < 10) day = '0';
		day += std::to_string(date.day);

		std::string month;
		if (date.month < 10) month = '0';
		month += std::to_string(date.month);

		std::string year = std::to_string(date.year);

		std::string hour;
		if (date.hour < 10) hour = '0';
		hour += std::to_string(date.hour);

		std::string minute;
		if (date.minute < 10) minute = '0';
		minute += std::to_string(date.minute);

		std::string second;
		if (date.second < 10) second = '0';
		second += std::to_string(date.second);

		std::string logFile;
		logFile = '[' + day + '-' + month + '-' + year + "][" + hour + '-' + minute + '-' + second + "].log";

		std::string absPath = logFolder + logFile;

		g_LogFile.open(absPath.c_str());
		if (!g_LogFile.is_open()) {

			// Create logs folder
			if (std::filesystem::create_directories(logFolder)) return Result_UnknownError;

			g_LogFile.open(absPath.c_str());

			if (!g_LogFile.is_open())
				return Result_NotFound;
		}

		return Result_Success;
	}

	Result logging_close()
	{
		g_LogFile.close();
		return Result_Success;
	}

	void __internal__do_not_call_this_please_or_you_will_die__console_clear()
	{
		std::lock_guard<std::mutex> lock(g_LogMutex);
		system("CLS");
	}

	void log(const char* title, const char* s1, va_list args, ui32 id)
	{
		char logBuffer[1001];
		size_t offset = 0u;

		// Date
		{
			std::string dateStr = date_string(timer_date());
			memcpy(logBuffer, dateStr.data(), dateStr.size());
			offset += dateStr.size();
		}

		// Title
		{
			size_t titleSize = strlen(title);
			if (titleSize != 0u) {
				logBuffer[offset++] = '[';
				logBuffer[offset + titleSize] = ']';
				memcpy(logBuffer + offset, title, titleSize);
				offset += titleSize + 1u;
			}
		}

		// Content
		vsnprintf(logBuffer + offset, 1000 - offset, s1, args);

		size_t size = strlen(logBuffer);
		logBuffer[size] = '\n';
		logBuffer[size + 1u] = '\0';

#ifdef SV_PLATFORM_WIN
		ui32 platformFlags = 0u;

		switch (id)
		{
		case 0u:
			platformFlags |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;

		case 1u:
			platformFlags |= FOREGROUND_GREEN;
			break;

		case 2u:
			platformFlags |= FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;

		case 3u:
			platformFlags |= FOREGROUND_RED;
			break;

		case 4u:
			platformFlags |= FOREGROUND_RED | FOREGROUND_BLUE;
			break;

		case 5u:
			platformFlags |= FOREGROUND_RED | FOREGROUND_GREEN;
			break;
		}
#endif

		std::lock_guard<std::mutex> lock(g_LogMutex);

#ifdef SV_PLATFORM_WIN
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), platformFlags);
#endif

		std::cout << logBuffer;
		g_LogFile << logBuffer;
	}

	void __internal__do_not_call_this_please_or_you_will_die__console_log(ui32 id, const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		log("", s, args, id);

		va_end(args);
	}

#endif
	
	///////////////////////////////////////////////// PROFILER /////////////////////////////////////////////////

#ifdef SV_ENABLE_PROFILER

	struct ChronoProfile {
		float beginTime;
		float endTime;
	};

	struct ScalarProfile {
		SV_PROFILER_SCALAR value;
	};

	static std::unordered_map<std::string, ChronoProfile>		g_ProfilerChrono;
	static std::shared_mutex									g_ProfilerChronoMutex;
	static std::unordered_map<const char*, ScalarProfile>		g_ProfilerScalar;
	static std::mutex											g_ProfilerScalarMutex;

	void __internal__do_not_call_this_please_or_you_will_die__profiler_chrono_begin(const char* name)
	{
		std::scoped_lock lock(g_ProfilerChronoMutex);
		g_ProfilerChrono[name].beginTime = timer_now();
	}

	void __internal__do_not_call_this_please_or_you_will_die__profiler_chrono_end(const char* name)
	{
		std::scoped_lock lock(g_ProfilerChronoMutex);
		g_ProfilerChrono[name].endTime = timer_now();
	}

	float __internal__do_not_call_this_please_or_you_will_die__profiler_chrono_get(const char* name)
	{
		std::shared_lock lock(g_ProfilerChronoMutex);
		auto& prof = g_ProfilerChrono[name];
		return prof.endTime - prof.beginTime;
	}

	void __internal__do_not_call_this_please_or_you_will_die__profiler_scalar_set(const char* name, SV_PROFILER_SCALAR value)
	{
		std::scoped_lock lock(g_ProfilerScalarMutex);
		g_ProfilerScalar[name].value = value;
	}

	void __internal__do_not_call_this_please_or_you_will_die__profiler_scalar_add(const char* name, SV_PROFILER_SCALAR value)
	{
		std::scoped_lock lock(g_ProfilerScalarMutex);
		g_ProfilerScalar[name].value += value;
	}

	void __internal__do_not_call_this_please_or_you_will_die__profiler_scalar_mul(const char* name, SV_PROFILER_SCALAR value)
	{
		std::scoped_lock lock(g_ProfilerScalarMutex);
		g_ProfilerScalar[name].value *= value;
	}
	
	SV_PROFILER_SCALAR __internal__do_not_call_this_please_or_you_will_die__profiler_scalar_get(const char* name)
	{
		std::scoped_lock lock(g_ProfilerScalarMutex);
		return g_ProfilerScalar[name].value;
	}

#endif

	///////////////////////////////////////////////// ENGINE CALLBACKS /////////////////////////////////////////////////

#define SV_CATCH catch (std::exception e) {\
					SV_LOG_ERROR("STD Exception: %s", e.what()); \
					sv::show_message(L"STD Exception", sv::parse_wstring(e.what()).c_str(), sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}\
				catch (int i) {\
					SV_LOG_ERROR("Unknown Exception %i", i); \
					sv::show_message(L"Unknown Exception", std::to_wstring(i).c_str(), sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}\
				catch (...) {\
					SV_LOG_ERROR("Unknown Exception"); \
					sv::show_message(L"Unknown Exception", L"", sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}

	static ApplicationCallbacks		g_App;
	static constexpr Version		g_Version = { 0u, 0u, 0u };
	static std::string				g_Name;

	static float		g_DeltaTime = 0.f;
	static ui64			g_FrameCount = 0u;

	static bool g_EnableAnimations = true;

	// Initialization

	Result engine_initialize(const InitializationDesc* d)
	{
		if (d == nullptr) return Result_InvalidUsage;

		// Initialization Parameters
		const InitializationDesc& desc = *d;
		g_App = desc.callbacks;

		g_Name = "SilverEngine ";
		g_Name += g_Version.toString();

		// SYSTEMS
		try {

			svCheck(logging_initialize());

			SV_LOG_CLEAR();
			SV_LOG_INFO("Initializing %s", g_Name.c_str());

			svCheck(task_initialize(desc.minThreadsCount));
			svCheck(event_initialize());
			svCheck(asset_initialize(desc.assetsFolderPath));
			svCheck(window_initialize(desc.windowStyle, desc.windowBounds, desc.windowTitle, desc.iconFilePath));
			svCheck(graphics_initialize());
			svCheck(animator_initialize());
			svCheck(sprite_animator_initialize());
			svCheck(matsys_initialize());
			svCheck(debug_renderer_initialize());
			svCheck(sprite_renderer_initialize());

			// APPLICATION
			svCheck(g_App.initialize());
		}
		SV_CATCH;

		SV_LOG_INFO("Initialized successfuly");
		SV_LOG_SEPARATOR();

		return Result_Success;
	}

	Result engine_loop()
	{
		static Time		lastTime = 0.f;

		g_FrameCount++;

		try {

			// Calculate DeltaTime
			Time actualTime = timer_now();
			g_DeltaTime = actualTime - lastTime;
			lastTime = actualTime;

			if (g_DeltaTime > 0.3f) g_DeltaTime = 0.3f;

			// Update Input
			if (input_update()) return Result_CloseRequest;
			window_update();

			// Update animations
			if (g_EnableAnimations) {
				animator_update(g_DeltaTime);
				sprite_animator_update(g_DeltaTime);
			}

			// Update assets
			asset_update(g_DeltaTime);

			// Update User
			g_App.update(g_DeltaTime);

			// Begin Rendering
			graphics_begin();

			// Before rendering, update materials
			matsys_update();

			// User Rendering
			g_App.render();

			// End frame
			graphics_end();
		}
		SV_CATCH;

		return Result_Success;
	}

	Result engine_close()
	{
		SV_LOG_SEPARATOR();
		SV_LOG_INFO("Closing %s", g_Name.c_str());

		// APPLICATION
		try {
			svCheck(g_App.close());

			asset_free_unused();
			svCheck(sprite_renderer_close());
			svCheck(debug_renderer_close());
			svCheck(matsys_close());
			svCheck(sprite_animator_close());
			svCheck(animator_close());
			svCheck(graphics_close());
			svCheck(window_close());
			svCheck(asset_close());
			svCheck(event_close());
			svCheck(task_close());
			svCheck(logging_close());
		}
		SV_CATCH;

		return Result_Success;
	}

	Version engine_version_get() noexcept { return g_Version; }
	float engine_deltatime_get() noexcept { return g_DeltaTime; }

	ui64 engine_frame_count() noexcept { return g_FrameCount; }

	void engine_animations_enable()
	{
		g_EnableAnimations = true;
	}

	void engine_animations_disable()
	{
		g_EnableAnimations = false;
	}

	bool engine_animations_is_enabled()
	{
		return g_EnableAnimations;
	}

}
