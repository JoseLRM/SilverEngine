#pragma once

//constexpr const char* SV_SRC_PATH = "";
constexpr const char* SV_SRC_PATH = "../SilverEngine/";

#define SV_PLATFORM_WINDOWS
#define SV_DEBUG

// Graphics

#define SV_GFX_COMMAND_LIST_COUNT 32

#define SV_GFX_VERTEX_BUFFER_COUNT 32
#define SV_GFX_CONSTANT_BUFFER_COUNT 32
#define SV_GFX_IMAGE_COUNT 32
#define SV_GFX_SAMPLER_COUNT 16
#define SV_GFX_VIEWPORT_COUNT 16
#define SV_GFX_SCISSOR_COUNT 16
#define SV_GFX_INPUT_SLOT_COUNT 16
#define SV_GFX_INPUT_ELEMENT_COUNT 16

#define SV_GFX_RENDER_TARGET_ATT_COUNT 8
#define SV_GFX_ATTACHMENTS_COUNT (SV_GFX_RENDER_TARGET_ATT_COUNT + 1)

// Graphics Vulkan

#define SV_GFX_VK_VALIDATION_LAYERS 0

// Entity Component System

#define SV_ECS_REQUEST_COMPONENTS_COUNT 16
