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
    static Date last_user_lib_write = {};

    void initialize_console();
    void close_console();
    void update_console();
    void update_editor();
    bool initialize_editor();
    bool close_editor();
    void draw_editor();
    void draw_console();
#endif
    
    bool os_startup();
    void os_recive_input();
    bool os_shutdown();
    
    bool graphics_initialize();
    bool graphics_close();

    bool renderer_initialize();
    bool renderer_close();

    void graphics_begin();
    void graphics_end();
    void renderer_begin();
    void renderer_end();

    bool close_scene(Scene* scene);
    void close_assets();
    void update_assets();
    
    bool initialize_scene(Scene** pscene, const char* name);
    void update_scene();
    void draw_scene();

    void os_update_user_callbacks(const char* dll);
    void os_free_user_callbacks();

    ////////////////////////////////////////////////////////////////// UPDATE DLL ////////////////////////////////////////////////////////////

    void set_gamecode_filepath(const char* filepath)
    {
	Archive file;
	file << filepath;
	if (!bin_write(hash_string("GAME DLL PATH"), file)) {

	    SV_LOG_ERROR("Can't save the gamecode filepath");
	}
    }

    SV_AUX void get_usercode_filepath(char* filepath)
    {
	Archive file;
	if (bin_read(hash_string("GAME DLL PATH"), file)) {

	    file >> filepath;
	}
	else sprintf(filepath, "%s", "Game.dll");
    }
    
    SV_AUX void recive_user_callbacks()
    {
	char filepath[FILEPATH_SIZE];
	get_usercode_filepath(filepath);
	
#if SV_DEV
	
	os_free_user_callbacks();
	
	if (!file_copy(filepath, "system/GameTemp.dll")) {
	    SV_LOG_ERROR("Can't create temporal game dll");
	    return;
	}
	
	os_update_user_callbacks("system/GameTemp.dll");

	Date date;
	if (file_date(filepath, nullptr, &date, nullptr)) {
	    last_user_lib_write = date;
	}
#else
	os_update_user_callbacks(filepath);
#endif
    }
    
#if SV_DEV
    
    SV_INTERNAL void update_user_callbacks()
    {
	static Time last_update = 0.0;
	Time now = timer_now();
	
	if (now - last_update > 1.0) {

	    char filepath[FILEPATH_SIZE];
	    get_usercode_filepath(filepath);
	    
	    // Check if the file is modified
	    Date date;
	    if (file_date(filepath, nullptr, &date, nullptr)) {

		if (date <= last_user_lib_write)
		    return;
	    }
	    else return;
	}
	else return;

	last_update = now;

	recive_user_callbacks();
    }

#endif

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

    SV_INTERNAL bool create_image_asset(void* asset)
    {
	GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);
	image = nullptr;
	return true;
    }

    SV_INTERNAL bool load_image_asset(void* asset, const char* filepath)
    {
	GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);

	// Get file data
	void* data;
	u32 width;
	u32 height;
	if (!load_image(filepath, &data, &width, &height)) return false;

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

	bool res = graphics_image_create(&desc, &image);

	delete[] data;
	return res;
    }

    SV_INTERNAL bool destroy_image_asset(void* asset)
    {
	GPUImage*& image = *reinterpret_cast<GPUImage**>(asset);
	graphics_destroy(image);
	image = nullptr;
	return true;
    }

    SV_INTERNAL bool reload_image_asset(void* asset, const char* filepath)
    {
	SV_CHECK(destroy_image_asset(asset));
	return load_image_asset(asset, filepath);
    }

    SV_INTERNAL bool create_mesh_asset(void* asset)
    {
	new(asset) Mesh();
	return true;
    }

    SV_INTERNAL bool load_mesh_asset(void* asset, const char* filepath)
    {
	Mesh& mesh = *new(asset) Mesh();
	SV_CHECK(load_mesh(filepath, mesh));
	SV_CHECK(mesh_create_buffers(mesh));
	return true;
    }

    SV_INTERNAL bool free_mesh_asset(void* asset)
    {
	Mesh& mesh = *reinterpret_cast<Mesh*>(asset);

	graphics_destroy(mesh.vbuffer);
	graphics_destroy(mesh.ibuffer);

	mesh.~Mesh();
	return true;
    }

    SV_INTERNAL bool create_material_asset(void* asset)
    {
	new(asset) Material();
	return true;
    }

    SV_INTERNAL bool load_material_asset(void* asset, const char* filepath)
    {
	Material& material = *new(asset) Material();
	SV_CHECK(load_material(filepath, material));
	return true;
    }

    SV_INTERNAL bool free_material_asset(void* asset)
    {
	Material& mat = *reinterpret_cast<Material*>(asset);
	mat.~Material();
	return true;
    }

    SV_AUX bool register_assets()
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

	SV_CHECK(register_asset_type(&desc));

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

	SV_CHECK(register_asset_type(&desc));

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

	SV_CHECK(register_asset_type(&desc));

	return true;
    }
    
    void engine_main()
    {
#if SV_DEV
	initialize_console();
#endif

	SV_LOG_INFO("Initializing %s", engine.name);
	
	if (os_startup()) {
	    SV_LOG_INFO("OS layer initialized");
	}
	else {
	    SV_LOG_ERROR("Can't initialize OS layer");
	    return;
	}

	// TODO initialize_audio());
	// TODO task_initialize();

	// Initialize Graphics API
	if (graphics_initialize()) {
	    SV_LOG_INFO("Graphics API initialized");
	}
	else {
	    SV_LOG_ERROR("Can't initialize graphics API");
	    return;
	}

	// Initialize Renderer
	if (renderer_initialize()) {
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

	    register_component<BodyComponent>("Body");
	}

	if (!register_assets()) {
	    SV_LOG_ERROR("Can't register default assets");
	    return;
	}

#if SV_DEV
	initialize_editor();
#endif
	
	engine.running = true;

	static Time lastTime = 0.f;

	recive_user_callbacks();

	// User init
	if (engine.user.initialize) {
	    if (!engine.user.initialize(true)) {
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

#if SV_DEV
	    update_user_callbacks();
#endif
	    
	    process_input();

	    if (engine.close_request) {
		break;
	    }

	    update_assets();
	    
	    graphics_begin();
	    renderer_begin();

	    // Scene management
	    {
		if (engine.next_scene_name[0] != '\0') {

		    // Close last scene
		    if (engine.scene) {
			close_scene(engine.scene);
			// TODO: handle error
		    }

		    initialize_scene(&engine.scene, engine.next_scene_name);
		    engine.next_scene_name[0] = '\0';
		    // TODO Handle error

		}
	    }
	    
#if SV_DEV
	    update_console();
	    update_editor();
	    
	    update_scene();
#else
	    update_scene();
#endif
    
	    
            // Draw scene
	    draw_scene();
	    
	    // Draw editor and the console	    
#if SV_DEV
	    draw_editor();
	    draw_console();
#endif

	    renderer_end();
	    graphics_end();
	}

	SV_LOG_INFO("Closing %s", engine.name);

	// User close
	if (engine.user.close) {
		if (!engine.user.close()) {
			SV_LOG_ERROR("User can't close successfully");
		}
	}
	
	if (engine.scene) close_scene(engine.scene);
	//TODO svCheck(engine.callbacks.close());

	free_unused_assets();

#if SV_DEV
        close_editor();
#endif

	if (!renderer_close()) { SV_LOG_ERROR("Can't close render utils"); }
	if (!graphics_close()) { SV_LOG_ERROR("Can't close graphicsAPI"); }
	if (!os_shutdown()) { SV_LOG_ERROR("Can't shutdown OS layer properly"); }
	close_assets();
	// if (result_fail(task_close())) { SV_LOG_ERROR("Can't close the task system"); }

#if SV_DEV
	close_console();
#endif

    }

}