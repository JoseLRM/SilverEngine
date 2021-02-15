#include "SilverEngine.h"
#include "SilverEngine/utils/allocators/FrameList.h"

using namespace sv;

struct TexturedMesh {
	Mesh mesh;
	u32 mat_index;
};

struct MeshComponent : public Component<MeshComponent> {
    Mesh     mesh;
    Material material;
};

GPUImage* offscreen = nullptr;
GPUImage* zbuffer = nullptr;
v3_f32				camera_position;
v2_f32				camera_rotation;
CameraProjection	camera;
CameraBuffer		camera_buffer;

ECS* ecs;
GUI* gui;

void load_model(ECS* ecs, const char* filepath, f32 scale = f32_max)
{
	ModelInfo info;
	if (result_fail(model_load(filepath, info))) {
		SV_LOG_ERROR("Can't load the model");
	}
	else {

		for (u32 i = 0u; i < info.meshes.size(); ++i) {
		    
			MeshInfo& m = info.meshes[i];

			Entity e = ecs_entity_create(ecs);

			MeshComponent& comp = *ecs_component_add<MeshComponent>(ecs, e);
			
			comp.mesh.positions = std::move(m.positions);
			comp.mesh.normals = std::move(m.normals);
			comp.mesh.texcoords = std::move(m.texcoords);
			comp.mesh.indices = std::move(m.indices);

			if (scale != f32_max)
				mesh_set_scale(comp.mesh, scale, true);
			mesh_create_buffers(comp.mesh);

			MaterialInfo& mat = info.materials[m.material_index];

			comp.material.diffuse_color = mat.diffuse_color;
			comp.material.specular_color = mat.specular_color;
			comp.material.shininess = mat.shininess;
			comp.material.diffuse_map = mat.diffuse_map;
		}
	}
}

Result init()
{
	svCheck(offscreen_create(1920u, 1080u, &offscreen));
	svCheck(zbuffer_create(1920u, 1080u, &zbuffer));

	ecs_component_register<MeshComponent>("Mesh");
	
	ecs_create(&ecs);
	
	svCheck(camerabuffer_create(&camera_buffer));

	gui = gui_create();

	//load_model(ecs, "assets/dragon.obj");
	load_model(ecs, "assets/gobber/GoblinX.obj");

	return Result_Success;
}

void update()
{
	key_shortcuts();

	gui_update(gui, window_width_get(engine.window), window_height_get(engine.window));

	projection_adjust(camera, f32(window_width_get(engine.window)) / f32(window_height_get(engine.window)));

	camera_controller3D(camera_position, camera_rotation, camera);
}

void render()
{
	CommandList cmd = graphics_commandlist_begin();

	render_context[cmd].offscreen = offscreen;
	render_context[cmd].zbuffer = zbuffer;
	render_context[cmd].camera_buffer = &camera_buffer;

	graphics_image_clear(offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color4f::Black(), 1.f, 0u, cmd);
	graphics_image_clear(zbuffer, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, Color4f::White(), 1.f, 0u, cmd);

	graphics_viewport_set(offscreen, 0u, cmd);
	graphics_scissor_set(offscreen, 0u, cmd);

	// Process lighting

	LightInstance lights[] = { LightInstance(Color3f::White(), v3_f32{ 0.f, 2.f, 0.f }, 3.f, 1.f, 0.5f) };

	// Mesh rendering

	static FrameList<MeshInstance> meshes;

	// Draw scene

	meshes.reset();

	camera.projection_type = ProjectionType_Perspective;
	camera.width = 0.5f;
	camera.height = 0.5f;
	camera.near = 0.3f;
	camera.far = 10000.f;
	projection_adjust(camera, window_aspect_get(engine.window));
	projection_update_matrix(camera);

	camera_buffer.rotation = v4_f32(XMQuaternionRotationRollPitchYawFromVector(camera_rotation.getDX()));
	camera_buffer.view_matrix = math_matrix_view(camera_position, camera_buffer.rotation);
	camera_buffer.projection_matrix = camera.projection_matrix;
	camera_buffer.position = camera_position;
	camerabuffer_update(&camera_buffer, cmd);

	EntityView<MeshComponent> entities(ecs);

	for (MeshComponent& mesh : entities) {
	    		meshes.emplace_back(XMMatrixScaling(1.01f, 1.01f,1.01f) * XMMatrixRotationY(timer_now() * 0.5f), &mesh.mesh, &mesh.material);
	}

	draw_meshes(meshes.data(), u32(meshes.size()), lights, 1u, cmd);

	gui_render(gui, cmd);
	
	graphics_present(engine.window, offscreen, GPUImageLayout_RenderTarget, cmd);
}

Result close()
{
	svCheck(graphics_destroy(offscreen));
	svCheck(graphics_destroy(zbuffer));

	camerabuffer_destroy(&camera_buffer);
	
	gui_destroy(gui);

	return Result_Success;
}

int main()
{
	InitializationDesc desc;
	desc.minThreadsCount = 2u;
	desc.callbacks.initialize = init;
	desc.callbacks.update = update;
	desc.callbacks.render = render;
	desc.callbacks.close = close;
	desc.windowDesc.bounds = { 0u, 0u, 1080u, 720u };
	desc.windowDesc.flags = WindowFlag_Default;
	desc.windowDesc.iconFilePath = nullptr;
	desc.windowDesc.state = WindowState_Windowed;
	//desc.windowDesc.state = WindowState_Fullscreen;
	desc.windowDesc.title = L"Test";

	if (result_fail(engine_initialize(&desc))) {
		printf("Can't init");
	}
	else {

		while (result_okay(engine_loop()));

		engine_close();
	}

#ifdef SV_ENABLE_LOGGING
	system("pause");
#endif

}
