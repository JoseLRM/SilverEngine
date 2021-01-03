#pragma once

#include "simulation/sprite_animator.h"

namespace sv {

	/*
		Duration variable means the duration per sprite.
		Make sure that animationAsset has a valid reference in the AnimatedSprite class
	*/
	struct AnimatedSprite_internal {
		
		bool running = false;
		float time = 0.f; 
		u32 repeatCount = 0u;
		u32 repeatIndex = 0u;
		float duration = 0.3f;
		u32 index = 0u;
		SpriteAnimationAsset animation;

	};

	Result sprite_animation_load_file(const char* filePath, void* pObject);
	Result sprite_animation_destroy(void* pObject);
	Result sprite_animation_reload(const char* filePath, void* pObject);
	Result sprite_animator_serialize(ArchiveO& file, void* pObject);

	Result	sprite_animator_initialize();
	void	sprite_animator_update(float dt);
	Result	sprite_animator_close();

}