#pragma once

#define SV_SRC_PATH "../SilverEngine/"

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
#define SV_GFX_BARRIER_COUNT 8

#define SV_GFX_RENDER_TARGET_ATT_COUNT 8
#define SV_GFX_ATTACHMENTS_COUNT (SV_GFX_RENDER_TARGET_ATT_COUNT + 1)

// Graphics Vulkan

#define SV_GFX_VK_VALIDATION_LAYERS 1

// Entity Component System

#define SV_ECS_MAX_COMPONENTS 128
#define SV_ECS_REQUEST_COMPONENTS_COUNT 16
#define SV_ECS_NAME_COMPONENT 1

// Renderer

#define SV_REND_LAYER_BATCH_COUNT 10000

#define SV_REND_OFFSCREEN_FORMAT Format_R8G8B8A8_SRGB