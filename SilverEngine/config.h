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
#define SV_GFX_VK_MAX_DESCRIPTOR_SETS 100u
#define SV_GFX_VK_MAX_DESCRIPTOR_TYPES 32u
#define SV_GFX_VK_DESCRIPTOR_ALLOC_COUNT 10u
#define SV_GFX_VK_UNUSED_OBJECTS_TIMECHECK 30.f
#define SV_GFX_VK_UNUSED_OBJECTS_LIFETIME 10.f

// Entity Component System

#define SV_ECS_COMPONENT_POOL_SIZE 200u
#define SV_ECS_ENTITY_ALLOC_SIZE 1000u
#define SV_ECS_REQUEST_COMPONENTS_COUNT 16

// Renderer

#define SV_REND_BATCH_COUNT 10000u
#define SV_REND_MESH_INSTANCE_COUNT 100u

#define SV_REND_FORWARD_LIGHT_COUNT 10u

#define SV_REND_OFFSCREEN_FORMAT Format_R8G8B8A8_SRGB

// Scene

#define SV_SCENE_ASSET_LIFETIME 10.f