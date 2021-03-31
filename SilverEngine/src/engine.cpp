#include "os.h"
#include "engine.h"
#include "dev.h"
#include "scene.h"
#include "asset_system.h"

namespace sv {

    GlobalEngineData engine;
    GlobalInputData input;

#if SV_DEV
    GlobalDevData dev;
#endif
    
    Result os_create_window();
    void os_recive_input();
    Result os_destroy_window();
    void initialize_console();
    void close_console();

    Result graphics_initialize();
    Result graphics_close();

    Result renderer_initialize();
    Result renderer_close();

    void graphics_begin();
    void graphics_end();
    void renderer_begin();
    void renderer_end();

    Result close_scene(Scene* scene);
    void close_assets();
    void update_assets();
    void update_console();
    Result initialize_scene(Scene** pscene, const char* name);
    void update_editor();
    Result initialize_editor();
    Result close_editor();
    void draw_editor(CommandList cmd);
    void draw_console(CommandList cmd);
    void update_scene(Scene* scene);
    void draw_scene(Scene* scene);

    void os_update_user_callbacks();

    /////////////////////////////////////////////////////////////////// PROCESS INPUT /////////////////////////////////////////////////////////////

    SV_AUX void process_input()
    {
	input.text.clear();
	input.text_commands.resize(1u);
	input.text_commands.back() = TextCommand_Null;

	// Reset unused input
	input.unused = true;

	// KEYS
	// reset pressed and released
	for (Key i = Key(0); i < Key_MaxEnum; ++(u32&)i) {

	    if (input.keys[i] == InputState_Pressed) {
		input.keys[i] = InputState_Hold;
	    }
	    else if (input.keys[i] == InputState_Released) input.keys[i] = InputState_None;
	}

	// MOUSE BUTTONS
	// reset pressed and released
	for (MouseButton i = MouseButton(0); i < MouseButton_MaxEnum; ++(u32&)i) {

	    if (input.mouse_buttons[i] == InputState_Pressed) {
		input.mouse_buttons[i] = InputState_Hold;
	    }
	    else if (input.mouse_buttons[i] == InputState_Released) input.mouse_buttons[i] = InputState_None;
	}

	input.mouse_last_pos = input.mouse_position;
	input.mouse_wheel = 0.f;

	os_recive_input();

	input.mouse_dragged = input.mouse_position - input.mouse_last_pos;
    }
    
    //////////////////////////////////////////////////////////////////// ASSET FUNCTIONS //////////////////////////////////////////////////////////

    internal Result create_image_asset(void* asset)
    {
	GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);
	image = nullptr;
	return Result_Success;
    }

    internal Result load_image_asset(void* asset, const char* filepath)
    {
	GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);

	// Get file data
	void* data;
	u32 width;
	u32 height;
	svCheck(load_image(filepath, &data, &width, &height));

	// Create Image
	GPUImageDesc desc;

	desc.pData = data;
	desc.size = width * height * 4u;
	desc.format = Format_R8G8B8A8_UNORM;
	desc.layout = GPUImageLayout_ShaderResource;
	desc.type = GPUImageType_ShaderResource;
	desc.usage = ResourceUsage_Static;
	desc.CPUAccess = CPUAccess_None;
	desc.width = width;
	desc.height = height;

	Result res = graphics_image_create(&desc, &image);

	delete[] data;
	return res;
    }

    internal Result destroy_image_asset(void* asset)
    {
	GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);
	graphics_destroy(image);
	image = nullptr;
	return Result_Success;
    }

    internal Result reload_image_asset(void* asset, const char* filepath)
    {
	svCheck(destroy_image_asset(asset));
	return load_image_asset(asset, filepath);
    }

    internal Result create_mesh_asset(void* asset)
    {
	new(asset) Mesh();
	return Result_Success;
    }

    internal Result load_mesh_asset(void* asset, const char* filepath)
    {
	Mesh& mesh = *new(asset) Mesh();
	svCheck(load_mesh(filepath, mesh));
	svCheck(mesh_create_buffers(mesh));
	return Result_Success;
    }

    internal Result free_mesh_asset(void* asset)
    {
	Mesh& mesh = *reinterpret_cast<Mesh*>(asset);

	graphics_destroy(mesh.vbuffer);
	graphics_destroy(mesh.ibuffer);

	mesh.~Mesh();
	return Result_Success;
    }

    internal Result create_material_asset(void* asset)
    {
	new(asset) Material();
	return Result_Success;
    }

    internal Result load_material_asset(void* asset, const char* filepath)
    {
	Material& material = *new(asset) Material();
	svCheck(load_material(filepath, material));
	return Result_Success;
    }

    internal Result free_material_asset(void* asset)
    {
	Material& mat = *reinterpret_cast<Material*>(asset);
	mat.~Material();
	return Result_Success;
    }

    SV_AUX Result register_assets()
    {
	// Register assets
	
	AssetTypeDesc desc;
	const char* extensions[5u];
	desc.extensions = extensions;


	// Texture
	extensions[0] = "png";
	extensions[1] = "tga";
	extensions[2] = "jpg";
	extensions[3] = "jpeg";
	extensions[4] = "JPG";

	desc.name = "Texture";
	desc.asset_size = sizeof(GPUImage*);
	desc.extension_count = 5u;
	desc.create = create_image_asset;
	desc.load_file = load_image_asset;
	desc.free = destroy_image_asset;
	desc.reload_file = reload_image_asset;
	desc.unused_time = 3.f;

	svCheck(register_asset_type(&desc));

	// Mesh
	extensions[0] = "mesh";

	desc.name = "Mesh";
	desc.asset_size = sizeof(Mesh);
	desc.extension_count = 1u;
	desc.create = create_mesh_asset;
	desc.load_file = load_mesh_asset;
	desc.free = free_mesh_asset;
	desc.reload_file = nullptr;
	desc.unused_time = 5.f;

	svCheck(register_asset_type(&desc));

	// Material
	extensions[0] = "mat";

	desc.name = "Material";
	desc.asset_size = sizeof(Material);
	desc.extension_count = 1u;
	desc.create = create_material_asset;
	desc.load_file = load_material_asset;
	desc.free = free_material_asset;
	desc.reload_file = nullptr;
	desc.unused_time = 2.5f;

	svCheck(register_asset_type(&desc));

	return Result_Success;
    }
    
    void engine_main()
    {
	Result res;

	initialize_console();

	SV_LOG_INFO("Initializing %s", engine.name);
	
	res = os_create_window();
	if (result_okay(res)) {
	    SV_LOG_INFO("Window created");
	}
	else {
	    SV_LOG_ERROR("Can't create the window");
	    return;
	}

	// TODO initialize_audio());
	// TODO task_initialize();

	// Initialize Graphics API
	res = graphics_initialize();
	if (result_okay(res)) {
	    SV_LOG_INFO("Graphics API initialized");
	}
	else {
	    SV_LOG_ERROR("Can't initialize graphics API");
	    return;
	}

	// Initialize Renderer
	res = renderer_initialize();
	if (result_okay(res)) {
	    SV_LOG_INFO("Renderer initialized");
	}
	else {
	    SV_LOG_ERROR("Can't initialize the renderer");
	    return;
	}

	// Register components
	{
	    register_component<SpriteComponent>("Sprite");
	    register_component<CameraComponent>("Camera");
	    register_component<MeshComponent>("Mesh");
	    register_component<LightComponent>("Light");
	}

	res = register_assets();
	if (result_fail(res)) {
	    SV_LOG_ERROR("Can't register default assets");
	    return;
	}

#if SV_DEV
	initialize_editor();
#endif
	
	engine.running = true;

	static Time lastTime = 0.f;

	os_update_user_callbacks();

	// User init
	if (engine.user.initialize) {
	    res = engine.user.initialize();
	    if (result_fail(res)) {
		engine.running = false;
	    }
	}
	
	while (engine.running) {

	    // Calculate DeltaTime
	    Time actualTime = timer_now();
	    engine.deltatime = f32(actualTime - lastTime);
	    lastTime = actualTime;

	    if (engine.deltatime > 0.3f) engine.deltatime = 0.3f;

	    // Calculate FPS
	    {
		static f32 fpsTimeCount = 0.f;
		static u32 fpsCount = 0u;

		fpsTimeCount += engine.deltatime;
		++fpsCount;
		if (fpsTimeCount > 0.25f) {
		    fpsTimeCount -= 0.25f;
		    engine.FPS = fpsCount * 4u;
		    fpsCount = 0u;
		}
	    }
	    
	    process_input();

	    update_assets();
	    
	    graphics_begin();
	    renderer_begin();

#if SV_DEV
	    update_console();
#endif

	    // Scene management
	    {
		if (engine.next_scene_name.size()) {

		    // Close last scene
		    if (engine.scene) {
			close_scene(engine.scene);
			// TODO: handle error
		    }

		    initialize_scene(&engine.scene, engine.next_scene_name.c_str());
		    engine.next_scene_name.clear();
		    // TODO Handle error

		}
	    }

#if SV_DEV
	    update_editor();
#endif

	    // Update scene
	    if (engine.scene) update_scene(engine.scene);
    
	    if (engine.close_request) {
		break;
	    }            
	    
            // Draw scene
	    if (engine.scene) draw_scene(engine.scene);
	    
	    // Draw editor and the console and present to screen
	    CommandList cmd = graphics_commandlist_get();
	    
#if SV_DEV
	    draw_editor(cmd);
	    draw_console(cmd);
#endif

	    renderer_end();
	    graphics_end();
	}

	SV_LOG_INFO("Closing %s", engine.name);

	// User close
	if (engine.user.close) {
		res = engine.user.close();
		if (result_fail(res)) {
			SV_LOG_ERROR("User can't close successfully: %s", result_str(res));
		}
	}
	
	if (engine.scene) close_scene(engine.scene);
	//TODO svCheck(engine.callbacks.close());

	free_unused_assets();

#if SV_DEV
        close_editor();
#endif

	if (result_fail(renderer_close())) { SV_LOG_ERROR("Can't close render utils"); }
	if (result_fail(graphics_close())) { SV_LOG_ERROR("Can't close graphicsAPI"); }
	if (result_fail(os_destroy_window())) { SV_LOG_ERROR("Can't destroy the window properly"); }
	close_assets();
	// if (result_fail(task_close())) { SV_LOG_ERROR("Can't close the task system"); }

#if SV_DEV
	close_console();
#endif

    }

}
