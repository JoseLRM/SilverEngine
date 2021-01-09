#pragma once

#include "scene/scene.h"
#include "core/input.h"

#define GAME_TITLE L"Game Test"

#ifndef SV_DIST
#define FPS_MODE
#endif

using namespace sv;

struct GameState {

	virtual ~GameState() {}

	virtual Result	initialize() = 0;
	virtual void	update(f32) = 0;
	virtual void	fixedUpdate() = 0;
	virtual void	render() = 0;
	virtual Result	close() = 0;

	virtual CameraComponent& getCamera() = 0;

	SV_INLINE f32 camWidth() { return getCamera().projection.width; }
	SV_INLINE f32 camHeight() { return getCamera().projection.height; }
	SV_INLINE v2_f32 camSize() { CameraComponent& cam = getCamera(); return { cam.projection.width, cam.projection.height }; }

	SV_INLINE i32 inScreenX(f32 x, f32 r)
	{
		f32 w = camWidth() * 0.5f;
		if (x + r < -w) return -1;
		if (x - r > w) return 1;
		return 0;
	}

	SV_INLINE i32 inScreenY(f32 y, f32 r)
	{
		f32 h = camHeight() * 0.5f;
		if (y + r < -h) return -1;
		if (y - r > h) return 1;
		return 0;
	}

	SV_INLINE bool inScreen(const v2_f32& pos, f32 radius)
	{
		return inScreenX(pos.x, radius) == 0 && inScreenY(pos.y, radius) == 0;
	}

};

extern std::unique_ptr<GameState> g_State;

Result	game_initialize();
void	game_update(f32 dt);
void	game_render();
Result	game_close();