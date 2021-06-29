#if SV_EDITOR

#include "core/renderer.h"
#include "debug/console.h"
#include "core/event_system.h"
#include "core/physics3D.h"

namespace sv {

	enum GizmosTransformMode : u32 {
		GizmosTransformMode_None,
		GizmosTransformMode_Position
    };
	
    constexpr f32 GIZMOS_SIZE = 0.1f;

    enum GizmosObject : u32 {
		GizmosObject_None,
		GizmosObject_AxisX,
		GizmosObject_AxisY,
		GizmosObject_AxisZ,
    };

    struct GizmosData {

		GizmosTransformMode mode = GizmosTransformMode_Position;

		GizmosObject object = GizmosObject_None;
		bool focus = false;
	
		v3_f32 start_offset;
		f32 axis_size;
		f32 selection_size;
    };

	struct TerrainBrushData {
		f32 strength = 10.f;
		f32 range = 10.f;
		f32 min_height = 0.f;
		f32 max_height = 1000.f;
	};

	enum EditorToolType : u32 {
		EditorToolType_None,
		EditorToolType_Gizmos,
		EditorToolType_TerrainBrush,
	};
	
	struct EditorToolData {
		EditorToolType tool_type = EditorToolType_None;
		GizmosData gizmos_data;
		TerrainBrushData terrain_brush_data;
	};

    enum AssetElementType : u32 {
		AssetElementType_Unknown,
		AssetElementType_Texture,
		AssetElementType_Mesh,
		AssetElementType_Material,
		AssetElementType_SpriteSheet,
		AssetElementType_Prefab,
		AssetElementType_Directory,
    };

    struct AssetElement {
		char name[FILENAME_SIZE + 1];
		AssetElementType type;
    };

    struct AssetBrowserInfo {
		char filepath[FILEPATH_SIZE + 1] = {};
		List<AssetElement> elements;
		f64 last_update = 0.0;

		struct {

			char assetname[FILENAME_SIZE + 1u] = "";
			u32 state = 0u;
			
		} create_asset_data;
    };

	enum SpriteSheetEditorState : u32 {
		SpriteSheetEditorState_Main,
		SpriteSheetEditorState_SpriteList,
		SpriteSheetEditorState_ModifySprite,
		SpriteSheetEditorState_NewSprite,
		SpriteSheetEditorState_AnimationList,
		SpriteSheetEditorState_NewAnimation,
		SpriteSheetEditorState_ModifyAnimation,
		SpriteSheetEditorState_AddSprite,
	};

    struct SpriteSheetEditorData {
		SpriteSheetAsset current_sprite_sheet;
		SpriteSheetEditorState state = SpriteSheetEditorState_Main;
		SpriteSheetEditorState next_state = SpriteSheetEditorState_Main;
		Sprite temp_sprite;
		SpriteAnimation temp_anim;
		u32 modifying_id = 0u;
		f32 simulation_time = 0.f;
    };

	struct MaterialEditorData {
		MaterialAsset material;
	};

	struct ImportModelData {
		ModelInfo model_info;
	};

	struct CreatePrefabData {
		char name[FILENAME_SIZE + 1u] = "";
	};

    struct GlobalEditorData {

		List<Entity> selected_entities;
		Prefab selected_prefab = 0;
		bool camera_focus = false;

		v2_f32 absolute_mouse_position;
		v2_f32 absolute_mouse_last_position;
		v2_f32 absolute_mouse_dragged;
		
		v2_f32 editor_view_size;
		v2_f32 editor_view_position;
		bool in_editor_view;

		AssetBrowserInfo asset_browser;

		EditorToolData tool_data;

		GPUImage* offscreen = NULL;
	
		TextureAsset image;
		static constexpr v4_f32 TEXCOORD_FOLDER = { 0.f, 0.f, 0.05989583333333f, 0.05989583333333f };
		static constexpr v4_f32 TEXCOORD_PLAY = { 141.f / 1920.f, 6.f / 1920.f, 264.f / 1920.f, 129.f / 1920.f };
		static constexpr v4_f32 TEXCOORD_PAUSE = { 275.f / 1920.f, 5.f / 1920.f, 386.f / 1920.f, 116.f / 1920.f };
		static constexpr v4_f32 TEXCOORD_STOP = { 275.f / 1920.f, 5.f / 1920.f, 386.f / 1920.f, 116.f / 1920.f };
		static constexpr v4_f32 TEXCOORD_LIGHT_PROBE = { 0.7f, 0.7f, 1.f, 1.f };

		SpriteSheetEditorData sprite_sheet_editor_data;
		MaterialEditorData material_editor_data;
		ImportModelData import_model_data;
		CreatePrefabData create_prefab_data;

		char next_scene_name[SCENENAME_SIZE + 1u] = "";
    };

    GlobalEditorData editor;

	SV_AUX bool is_entity_selected(Entity entity)
	{
		for (Entity e : editor.selected_entities)
			if (e == entity) return true;
		return false;
	}

	SV_AUX void select_entity(Entity entity, bool unselect_all = false)
	{
		if (unselect_all || !input.keys[Key_Shift]) editor.selected_entities.reset();
		else if (is_entity_selected(entity)) return;

		if (entity_exists(entity))
			editor.selected_entities.push_back(entity);

		editor.selected_prefab = 0;
	}

	SV_AUX void unselect_entity(Entity entity)
	{
		foreach (i, editor.selected_entities.size()) {
			if (editor.selected_entities[i] == entity) {
				editor.selected_entities.erase(i);
				break;
			}
		}
	}

	SV_AUX void select_prefab(Prefab prefab)
	{
		editor.selected_entities.reset();
		editor.selected_prefab = prefab;
	}

	SV_AUX void edit_sprite_sheet(const char* filepath)
	{
		gui_show_window("SpriteSheet Editor");
		SpriteSheetAsset& asset = editor.sprite_sheet_editor_data.current_sprite_sheet;
		if (!load_asset_from_file(asset, filepath)) {

			SV_LOG_ERROR("Unknown error loading '%s'", filepath);
			gui_hide_window("SpriteSheet Editor");
		}
	}

    SV_INTERNAL void show_reset_popup()
    {
		if (dev.engine_state == EngineState_ProjectManagement || dev.engine_state == EngineState_None) {
			return;
		}
	
		if (dev.engine_state != EngineState_ProjectManagement && show_dialog_yesno("Code reloaded!!", "Do you want to reset the game?")) {

			_engine_reset_game();
			dev.next_engine_state = EngineState_Edit;
		}
    }

    /////////////////////////////////////////////// KEY SHORTCUTS //////////////////////////////////////

    SV_AUX void update_key_shortcuts()
    {
		// Fullscreen
		if (input.keys[Key_F10] == InputState_Pressed) {
			
			os_window_set_fullscreen(os_window_state() != WindowState_Fullscreen);
		}

		// Debug rendering
		if (input.keys[Key_F1] == InputState_Pressed) {
			dev.debug_draw = !dev.debug_draw;
		}

		// Console show - hide
		if (input.keys[Key_F3] == InputState_Pressed) {

			if (console_is_open())
				console_close();
			else console_open();
		}

		// Change debug camera projection
		if (input.keys[Key_F4] == InputState_Pressed) {

			dev.camera.projection_type = (dev.camera.projection_type == ProjectionType_Orthographic) ? ProjectionType_Perspective : ProjectionType_Orthographic;

			if (dev.camera.projection_type == ProjectionType_Orthographic) {

				dev.camera.width = 30.f;
				dev.camera.height = 30.f;
				dev.camera.near = -1000.f;
				dev.camera.far = 1000.f;
				dev.camera.rotation = { 0.f, 0.f, 0.f, 1.f };
			}
			else {

				dev.camera.width = 0.1f;
				dev.camera.height = 0.1f;
				dev.camera.near = 0.03f;
				dev.camera.far = 100000.f;
			}
		}
	
		// Compile game code
		if (input.keys[Key_F5] == InputState_Pressed)
			_os_compile_gamecode();
	
		if (dev.engine_state != EngineState_Play) {

			// Close engine or play game
			if (input.keys[Key_F11] == InputState_Pressed) {

				if (input.keys[Key_Control] && input.keys[Key_Alt])
					engine.close_request = true;
				else
					dev.next_engine_state = EngineState_Play;
			}
	    
			if (input.unused && input.keys[Key_Control]) {

				// Undo
				if (input.keys[Key_Z] == InputState_Pressed) {

					DoUndoStack& stack = dev.do_undo_stack;
					stack.lock();
					stack.undo_action(nullptr);
					stack.unlock();
				}

				// Do
				if (input.keys[Key_Y] == InputState_Pressed) {

					DoUndoStack& stack = dev.do_undo_stack;
					stack.lock();
					stack.do_action(nullptr);
					stack.unlock();
				}

				// Duplicate
				if (input.keys[Key_D] == InputState_Pressed) {

					for (Entity e : editor.selected_entities) {
						duplicate_entity(e);
					}
				}
			}
		}
		else {

			// Editor mode
			if (input.keys[Key_F11] == InputState_Pressed) {
				dev.next_engine_state = EngineState_Edit;
			}
		}
    }

    /////////////////////////////////////////////// DO UNDO ACTIONS ////////////////////////////////////
    
    typedef void(*ConstructEntityActionFn)(Entity entity);
    
    struct EntityCreate_Action {
		Entity entity;
		Entity parent;
		ConstructEntityActionFn construct_entity;
    };
    
    SV_INTERNAL void do_entity_create(void* pdata, void* preturn) {

		EntityCreate_Action& data = *(EntityCreate_Action*)pdata;

		Entity parent = data.parent;
		if (!entity_exists(parent))
			parent = 0;

		const char* name = (const char*)(pdata) + sizeof(data);
		if (*name == '\0')
			name = nullptr;
	
		data.entity = create_entity(parent, name);

		if (preturn) {
			memcpy(preturn, &data.entity, sizeof(Entity));
		}

		if (data.construct_entity) {
			data.construct_entity(data.entity);
		}
    }
    SV_INTERNAL void undo_entity_create(void* pdata, void* preturn) {

		EntityCreate_Action& data = *(EntityCreate_Action*)pdata;
		if (entity_exists(data.entity))
			destroy_entity(data.entity);
    }
    
    SV_INTERNAL void construct_entity_sprite(Entity entity) {
		add_entity_component(entity, get_component_id("Sprite"));
    }
    SV_INTERNAL void construct_entity_2D_camera(Entity entity) {
		add_entity_component(entity, get_component_id("Camera"));
    }
    
    SV_INTERNAL Entity editor_create_entity(Entity parent = 0, const char* name = nullptr, ConstructEntityActionFn construct_entity = nullptr)
    {
		DoUndoStack& stack = dev.do_undo_stack;
		stack.lock();

		stack.push_action(do_entity_create, undo_entity_create);
	
		EntityCreate_Action data;
		data.entity = 0;
		data.parent = parent;
		data.construct_entity = construct_entity;
	
		stack.push_data(&data, sizeof(data));

		size_t name_size = name ? strlen(name) : 0u;
		if (name_size)
			stack.push_data(name, name_size + 1u);
		else {
			char c = '\0';
			stack.push_data(&c, 1u);
		}

		Entity res;
		stack.do_action(&res);
	
		stack.unlock();

		return res;
    }

    /////////////////////////////////////////////// CAMERA ///////////////////////////////////////

    static void control_camera()
    {
		if (dev.postprocessing) {

			CameraComponent* cam = get_main_camera();

			if (cam) {
				dev.camera.bloom = cam->bloom;
			}
			else dev.postprocessing = false;
		}
		else {
			dev.camera.bloom = {};
		}
	
		if (!input.unused)
			return;

		if (dev.camera.projection_type == ProjectionType_Perspective) {

			XMVECTOR rotation = vec4_to_dx(dev.camera.rotation);

			XMVECTOR direction;
			XMMATRIX rotation_matrix;

			// Rotation matrix
			rotation_matrix = XMMatrixRotationQuaternion(rotation);

			// Camera direction
			direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
			direction = XMVector3Transform(direction, rotation_matrix);

			// Zoom
			if (input.mouse_wheel != 0.f) {

				f32 force = dev.camera.velocity;
				if (input.keys[Key_Shift] == InputState_Hold)
					force *= 3.f;

				dev.camera.position += v3_f32(direction) * input.mouse_wheel * force;
				input.unused = false;
			}

			// Camera rotation
			if (input.mouse_buttons[MouseButton_Center] == InputState_Pressed) {

				editor.camera_focus = true;
			}
			else if (input.mouse_buttons[MouseButton_Center] == InputState_Released) {
				editor.camera_focus = false;
			}

			if (editor.camera_focus && (input.mouse_dragged.x != 0.f || input.mouse_dragged.y != 0.f)) {

				v2_f32 drag = editor.absolute_mouse_dragged * 3.f;

				// TODO: pitch limit
				XMVECTOR pitch = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), -drag.y);
				XMVECTOR yaw = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), drag.x);

				rotation = XMQuaternionMultiply(pitch, rotation);
				rotation = XMQuaternionMultiply(rotation, yaw);
				rotation = XMQuaternionNormalize(rotation);
				input.unused = false;
			}

			dev.camera.rotation = v4_f32(rotation);
		}
		else {

			InputState button_state = input.mouse_buttons[MouseButton_Center];

			if (button_state == InputState_Pressed) {
				editor.camera_focus = true;
			}
			else if (button_state == InputState_Released) {
				editor.camera_focus = false;
			}

			if (editor.camera_focus) {

				v2_f32 drag = editor.absolute_mouse_dragged / editor.editor_view_size;

				dev.camera.position -= vec2_to_vec3((drag * v2_f32{ dev.camera.width, dev.camera.height }));
				input.unused = false;
			}
			else editor.camera_focus = false;

			if (input.mouse_wheel != 0.f) {

				f32 force = 0.05f;
				if (input.keys[Key_Shift] == InputState_Hold) force *= 3.f;

				f32 length = dev.camera.getProjectionLength();

				f32 new_length = length - input.mouse_wheel * length * force;
				dev.camera.setProjectionLength(new_length);

				input.unused = false;
			}
		}
    }

    /////////////////////////////////////////////// GIZMOS ///////////////////////////////////////////////////////

    SV_AUX v2_f32 world_point_to_screen(const XMMATRIX& vpm, const v3_f32& position)
    {
		XMVECTOR p = vec3_to_dx(position, 1.f);

		p = XMVector4Transform(p, vpm);
	
		f32 d = XMVectorGetW(p);
		p = XMVectorDivide(p, XMVectorSet(d, d, d, 1.f));
	
		return v2_f32(p);
    }

    SV_AUX v2_f32 project_line_onto_line(v2_f32 origin, v2_f32 d0, v2_f32 d1)
    {
		return origin + (vec2_dot(d1, d0) / vec2_dot(d1, d1)) * d1;
    }
    SV_AUX v3_f32 project_line_onto_line(v3_f32 origin, v3_f32 d0, v3_f32 d1)
    {
		return origin + (vec3_dot(d1, d0) / vec3_dot(d1, d1)) * d1;
    }

    SV_AUX void intersect_line_vs_plane(v3_f32 line_point, v3_f32 line_direction, v3_f32 plane_point, v3_f32 plane_normal, v3_f32& intersection, f32& distance)
    {
		distance = vec3_dot(plane_point - line_point, plane_normal) / vec3_dot(line_direction, plane_normal);
		intersection = line_point + distance * line_direction;
    }

    SV_AUX void closest_points_between_two_lines(v3_f32 l0_pos, v3_f32 l0_dir, v3_f32 l1_pos, v3_f32 l1_dir, v3_f32& l0_res, v3_f32& l1_res)
    {
		v3_f32 u = l0_dir;
		v3_f32 v = l1_dir;
		v3_f32 w = l0_pos - l1_pos;
		f32 a = vec3_dot(u, u);         // always >= 0
		f32 b = vec3_dot(u, v);
		f32 c = vec3_dot(v, v);         // always >= 0
		f32 d = vec3_dot(u, w);
		f32 e = vec3_dot(v, w);
		f32 D = a*c - b*b;        // always >= 0
		f32 sc, tc;

		constexpr f32 SMALL_NUM = 0.00000001f;
	
		// compute the line parameters of the two closest points
		if (D < SMALL_NUM) {          // the lines are almost parallel
			sc = 0.0;
			tc = (b>c ? d/b : e/c);    // use the largest denominator
		}
		else {
			sc = (b*e - c*d) / D;
			tc = (a*e - b*d) / D;
		}

		l0_res = l0_pos + sc * u;
		l1_res = l1_pos + tc * v;
    }

    SV_AUX f32 relative_scalar(f32 value, v3_f32 position)
    {
		if (dev.camera.projection_type == ProjectionType_Perspective) {
		
			XMVECTOR pos = vec3_to_dx(position, 1.f);
			pos = XMVector4Transform(pos, dev.camera.view_matrix);
	    
			f32 distance = XMVectorGetZ(pos);

			f32 w_near_distance = math_sqrt(dev.camera.near * dev.camera.near + dev.camera.width);
			f32 w_near_prop = value / w_near_distance;

			w_near_distance = math_sqrt(distance * distance + dev.camera.width);

			f32 h_near_distance = math_sqrt(dev.camera.near * dev.camera.near + dev.camera.height);
			f32 h_near_prop = value / h_near_distance;

			h_near_distance = math_sqrt(distance * distance + dev.camera.height);

			return (w_near_distance * w_near_prop + h_near_distance * h_near_prop) * 0.5f;
		}
		else {

			return value * dev.camera.getProjectionLength();
		}
    }

	SV_AUX v3_f32 compute_selected_entities_position()
	{
		v3_f32 position;
		
		f32 mult = 1.f / f32(editor.selected_entities.size());
		for (Entity e : editor.selected_entities) {

			position += get_entity_world_position(e) * mult;
		}

		return position;
	}

    SV_INTERNAL void update_gizmos()
    {
		GizmosData& info = editor.tool_data.gizmos_data;

		if (!input.unused || editor.selected_entities.empty()) {
			info.focus = false;
			return;
		}

		v3_f32 position = compute_selected_entities_position();
		
		// Compute axis size and selection size
		{
			info.axis_size = relative_scalar(GIZMOS_SIZE, position);
			info.selection_size = relative_scalar(0.008f, position);
		}

		if (!info.focus) {

			info.object = GizmosObject_None;
	    
			v2_f32 mouse_position = input.mouse_position * 2.f;

			switch (info.mode)
			{

			case GizmosTransformMode_Position:
			{
				GizmosObject obj[] = {
					GizmosObject_AxisX,
					GizmosObject_AxisY,
					GizmosObject_AxisZ
				};
		
				if (dev.camera.projection_type == ProjectionType_Perspective) {

					Ray ray = screen_to_world_ray(input.mouse_position, dev.camera.position, dev.camera.rotation, &dev.camera);
		    
					v3_f32 axis[3u];
					axis[0] = v3_f32(info.axis_size, 0.f, 0.f);
					axis[1] = v3_f32(0.f, info.axis_size, 0.f);
					axis[2] = v3_f32(0.f, 0.f, info.axis_size);

					// TODO: Sort axis update by the distance to the camera
					foreach(i, 3u) {

						v3_f32 p0, p1;
						closest_points_between_two_lines(ray.origin, ray.direction, position, axis[i], p0, p1);

						f32 dist0 = vec3_length(p0 - p1);
			
						f32 dist1 = -1.f;

						switch(i) {
			    
						case 0:
							dist1 = p1.x - position.x;
							break;
			    
						case 1:
							dist1 = p1.y - position.y;
							break;
			    
						case 2:
							dist1 = p1.z - position.z;
							break;
						}

						if (dist0 < info.selection_size && dist1 > 0.f && dist1 <= info.axis_size) {
							info.object = obj[i];
							info.start_offset = p1 - position;
						}
					}
				}
				else {

					v2_f32 mouse = input.mouse_position * v2_f32(dev.camera.width, dev.camera.height) + vec3_to_vec2(dev.camera.position);
					v2_f32 to_mouse = mouse - vec3_to_vec2(position);
			
					foreach (i, 2u) {

						v2_f32 axis_direction = ((i == 0) ? v2_f32::right() : v2_f32::up()) * info.axis_size;

						v2_f32 projection = project_line_onto_line(vec3_to_vec2(position), to_mouse, axis_direction);
						f32 dist0 = vec2_length(mouse - projection);
						f32 dist1 = ((i == 0) ? (projection.x - position.x) : (projection.y - position.y));
			
						if (dist0 < info.selection_size && dist1 > 0.f && dist1 <= info.axis_size) {
							info.object = obj[i];

							if (i == 0) info.start_offset = { dist1, 0.f, 0.f };
							else info.start_offset = { 0.f, dist1, 0.f };
						}
					}
				}
			}
			break;

			}

			if (info.object != GizmosObject_None && input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

		
				info.focus = true;
				input.unused = false;
			}
		}

		else {

			input.unused = false;

			if (input.mouse_buttons[MouseButton_Left] == InputState_None) {

				info.focus = false;
			}
			else {

				v3_f32 init_pos = position;

				if (dev.camera.projection_type == ProjectionType_Perspective) {

					Ray ray = screen_to_world_ray(input.mouse_position, dev.camera.position, dev.camera.rotation, &dev.camera);

					v3_f32 axis;
		    
					switch (info.object) {

					case GizmosObject_AxisX:
						axis = v3_f32::right();
						break;

					case GizmosObject_AxisY:
						axis = v3_f32::up();
						break;

					case GizmosObject_AxisZ:
						axis = v3_f32::forward();
						break;
		    
					}

					v3_f32 p0, p1;
					closest_points_between_two_lines(ray.origin, ray.direction, position, axis, p0, p1);
					position = p1 - info.start_offset;
				}
				else {

					v2_f32 mouse = input.mouse_position * v2_f32(dev.camera.width, dev.camera.height) + vec3_to_vec2(dev.camera.position);
		
					if (info.object == GizmosObject_AxisX)
						position.x = mouse.x - info.start_offset.x;
					else if (info.object == GizmosObject_AxisY)
						position.y = mouse.y - info.start_offset.y;
				}

				// Set new position
				v3_f32 move = position - init_pos;
				
				for (Entity e : editor.selected_entities) {

					v3_f32& pos = *get_entity_position_ptr(e);

					Entity parent = get_entity_parent(e);

					if (entity_exists(parent)) {

						XMMATRIX wm = get_entity_world_matrix(parent);

						wm = XMMatrixInverse(NULL, wm);
						
						pos += (v3_f32)XMVector4Transform(vec3_to_dx(move), wm);
					}
					else pos += move;
				}
			}
		}
    }

    SV_INTERNAL void draw_gizmos(GPUImage* offscreen, CommandList cmd)
    {
		if (editor.selected_entities.empty()) return;

		GizmosData& info = editor.tool_data.gizmos_data;

		v3_f32 position = compute_selected_entities_position();

		f32 axis_size = info.axis_size;

		switch (info.mode)
		{

		case GizmosTransformMode_Position:
		{
			Color color = ((info.object == GizmosObject_AxisX) ? (info.focus ? Color::Silver() : Color{255u, 50u, 50u, 255u}) : Color::Red());
			imrend_draw_line(position, position + v3_f32::right() * axis_size, color, cmd);

			color = ((info.object == GizmosObject_AxisY) ? (info.focus ? Color::Silver() : Color::Lime()) : Color::Green());
			imrend_draw_line(position, position + v3_f32::up() * axis_size, color, cmd);

			color = ((info.object == GizmosObject_AxisZ) ? (info.focus ? Color::Silver() : Color{50u, 50u, 255u, 255u}) : Color::Blue());
			imrend_draw_line(position, position + v3_f32::forward() * axis_size, color, cmd);
		}
		break;

		}
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    bool _editor_initialize()
    {
		SV_CHECK(_gui_initialize());

		load_asset_from_file(editor.image, "$system/images/editor.png");

		event_register("user_callbacks_initialize", show_reset_popup, 0u);

		dev.engine_state = EngineState_ProjectManagement;
		_gui_load("PROJECT");
		engine.update_scene = false;

		// Create offscreen
		GPUImageDesc desc;
		desc.format = OFFSCREEN_FORMAT;
		desc.layout = GPUImageLayout_ShaderResource;
		desc.type = GPUImageType_ShaderResource;
		// TODO
		desc.width = 1920u;
		desc.height = 1080u;

		SV_CHECK(graphics_image_create(&desc, &editor.offscreen));

		return true;
    }

    bool _editor_close()
    {
		SV_CHECK(_gui_close());

		unload_asset(editor.image);

		graphics_destroy(editor.offscreen);
		
		return true;
    }
    
    SV_INTERNAL bool show_component_info(CompID comp_id, Component* comp)
    {
		bool remove;

		if (egui_begin_component(comp_id, &remove)) {

			if (get_component_id("Sprite") == comp_id) {

				SpriteComponent& spr = *reinterpret_cast<SpriteComponent*>(comp);

				gui_sprite("Sprite", spr.sprite_sheet, spr.sprite_id, 0u);
				gui_drag_color("Color", spr.color, 1u);

				bool xflip = spr.flags & SpriteComponentFlag_XFlip;
				bool yflip = spr.flags & SpriteComponentFlag_YFlip;

				if (gui_checkbox("XFlip", xflip, 3u)) spr.flags = spr.flags ^ SpriteComponentFlag_XFlip;
				if (gui_checkbox("YFlip", yflip, 4u)) spr.flags = spr.flags ^ SpriteComponentFlag_YFlip;
			}

			if (get_component_id("Animated Sprite") == comp_id) {

				AnimatedSpriteComponent& spr = *reinterpret_cast<AnimatedSpriteComponent*>(comp);

				SpriteSheet* sheet = spr.sprite_sheet.get();
				u32 max_index = 0u;

				if (sheet && sheet->sprite_animations.exists(spr.animation_id))
					max_index = sheet->sprite_animations[spr.animation_id].frames;

				gui_sprite_animation("Animation", spr.sprite_sheet, spr.animation_id, 0u);
				gui_drag_color("Color", spr.color, 1u);
				gui_drag_u32("Index", spr.index, 1u, 0u, max_index, 2u);
				gui_drag_f32("Time Mult", spr.time_mult, 0.01f, 0.f, f32_max, 6u);

				bool xflip = spr.flags & SpriteComponentFlag_XFlip;
				bool yflip = spr.flags & SpriteComponentFlag_YFlip;

				if (gui_checkbox("XFlip", xflip, 9u)) spr.flags = spr.flags ^ SpriteComponentFlag_XFlip;
				if (gui_checkbox("YFlip", yflip, 10u)) spr.flags = spr.flags ^ SpriteComponentFlag_YFlip;
			}
	    
			if (get_component_id("Mesh") == comp_id) {

				MeshComponent& m = *reinterpret_cast<MeshComponent*>(comp);

				egui_comp_mesh("Mesh", 0u, &m.mesh);
				egui_comp_material("Material", 1u, &m.material);

				if (gui_button("Edit Material")) {

					editor.material_editor_data.material = m.material;
				}

				if (gui_button("Set cube")) {

					create_asset_from_name(m.mesh, "Mesh", "Cube");
				}
				/*if (m.material.get())
				  gui_material(*m.material.get());*/
			}

			if (get_component_id("Camera") == comp_id) {

				CameraComponent& cam = *reinterpret_cast<CameraComponent*>(comp);

				f32 dimension = SV_MIN(cam.width, cam.height);

				f32 near_min;
				f32 near_max;
				f32 near_adv;
				f32 far_min;
				f32 far_max;
				f32 far_adv;

				if (cam.projection_type == ProjectionType_Perspective) {
					near_min = 0.001f;
					near_max = f32_max;
					near_adv = 0.01f;
					far_min = cam.near;
					far_max = f32_max;
					far_adv = 0.3f;
				}
				else {
					near_min = f32_min;
					near_max = cam.far;
					near_adv = 0.3f;
					far_min = cam.near;
					far_max = f32_max;
					far_adv = 0.3f;
				}

				bool perspective = cam.projection_type == ProjectionType_Perspective;

				if (gui_checkbox("Perspective", perspective)) {

					if (perspective) {
						cam.projection_type = ProjectionType_Perspective;
						cam.near = 0.2f;
						cam.far = 10000.f;
						cam.width = 0.1f;
						cam.height = 0.1f;
					}
					else {
						cam.projection_type = ProjectionType_Orthographic;
						cam.near = -1000.f;
						cam.far = 1000.f;
						cam.width = 100.f;
						cam.height = 100.f;
					}
				}

				gui_drag_f32("Near", cam.near, near_adv, near_min, near_max);
				gui_drag_f32("Far", cam.far, far_adv, far_min, far_max);
				if (gui_drag_f32("Dimension", dimension, 0.01f, 0.01f, f32_max)) {
					cam.width = dimension;
					cam.height = dimension;
				}

				gui_checkbox("Bloom", cam.bloom.active);
				if (cam.bloom.active) {

					gui_drag_f32("Threshold", cam.bloom.threshold, 0.001f, 0.f, 1.f);
					gui_drag_f32("Intensity", cam.bloom.intensity, 0.001f, 0.f, 1.f);
				}
			}

			if (get_component_id("Light") == comp_id) {

				LightComponent& l = *reinterpret_cast<LightComponent*>(comp);

				bool direction = l.light_type == LightType_Direction;
				if (gui_checkbox("Directional", direction, 10u)) {
					if (direction) l.light_type = LightType_Direction;
					else l.light_type = LightType_Point;
				}

				gui_drag_color("Color", l.color);
				gui_drag_f32("Intensity", l.intensity, 0.05f, 0.0f, f32_max);
				gui_drag_f32("Range", l.range, 0.1f, 0.0f, f32_max);
				gui_drag_f32("Smoothness", l.smoothness, 0.005f, 0.0f, 1.f);

				gui_separator(1);

				gui_checkbox("Shadow Mapping", l.shadow_mapping_enabled);

				if (l.shadow_mapping_enabled) {
					// TODO
				}
			}

			if (get_component_id("Body") == comp_id) {

				BodyComponent& body = *(BodyComponent*)comp;

				bool dynamic = body_type_get(body) == BodyType_Dynamic;

				if (gui_checkbox("Dynamic", dynamic)) {
					if (dynamic) body_type_set(body, BodyType_Dynamic);
					else body_type_set(body, BodyType_Static);
				}

				if (dynamic) {

					f32 linear_damping = body_linear_damping_get(body);
					f32 angular_damping = body_angular_damping_get(body);
					v3_f32 velocity = body_velocity_get(body);
					v3_f32 angular_velocity = body_angular_velocity_get(body);
					f32 mass = body_mass_get(body);

					if (gui_drag_f32("Linear Damping", linear_damping, 0.01f, 0.f, f32_max)) {
						body_linear_damping_set(body, linear_damping);
					}
					if (gui_drag_f32("Angular Damping", angular_damping)) {
						body_angular_damping_set(body, angular_damping);
					}
					if (gui_drag_f32("Mass", mass)) {
						body_mass_set(body, mass);
					}
					if (gui_drag_v3_f32("Velocity", velocity)) {
						body_velocity_set(body, velocity);
					}
					if (gui_drag_v3_f32("Angular Velocity", angular_velocity)) {
						body_angular_velocity_set(body, angular_velocity);
					}
				}
			}

			if (get_component_id("Box Collider") == comp_id) {

				BoxCollider& box = *(BoxCollider*)comp;

				gui_drag_v3_f32("Size", box.size, 0.01f, 0.00001f, f32_max);
			}

			if (get_component_id("Sphere Collider") == comp_id) {

				SphereCollider& sphere = *(SphereCollider*)comp;

				gui_drag_f32("Radius", sphere.radius, 0.01f, 0.00001f, f32_max);
			}

			DisplayComponentEvent e;
			e.comp_id = comp_id;
			e.comp = comp;

			gui_push_id("User");
			event_dispatch("display_component_data", &e);
			gui_pop_id();

			egui_end_component();
		}

		return remove;
    }

    SV_AUX void select_entity()
    {
		v2_f32 mouse = input.mouse_position;

		Ray ray = screen_to_world_ray(mouse, dev.camera.position, dev.camera.rotation, &dev.camera);

		XMVECTOR ray_origin = vec3_to_dx(ray.origin, 1.f);
		XMVECTOR ray_direction = vec3_to_dx(ray.direction, 0.f);

		Entity selected = 0;
		f32 distance = f32_max;

		XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
		XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
		XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
		XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

		XMVECTOR v0;
		XMVECTOR v1;
		XMVECTOR v2;
		XMVECTOR v3;

		u32 light_id = get_component_id("Light");
		u32 mesh_id = get_component_id("Mesh");
		u32 sprite_id = get_component_id("Sprite");

		// Select lights
		for (CompIt it = comp_it_begin(light_id);
			 it.has_next;
			 comp_it_next(it))
		{
			Entity entity = it.entity;
			
			if (is_entity_selected(entity))
				break;
			
			v3_f32 pos = get_entity_world_position(entity);
			
			f32 min_scale = relative_scalar(0.02f, pos);
			f32 scale = SV_MAX(min_scale, 1.f);
			
			XMMATRIX tm = XMMatrixScaling(scale, scale, 1.f) * XMMatrixRotationQuaternion(vec4_to_dx(dev.camera.rotation)) * XMMatrixTranslation(pos.x, pos.y, pos.z);
			
			v0 = XMVector3Transform(p0, tm);
			v1 = XMVector3Transform(p1, tm);
			v2 = XMVector3Transform(p2, tm);
			v3 = XMVector3Transform(p3, tm);
			
			f32 dis = f32_max;
			
			v3_f32 intersection;

			// TODO: Ray vs Quad intersection
			
			if (intersect_ray_vs_traingle(ray, v3_f32(v0), v3_f32(v1), v3_f32(v2), intersection)) {
				
				dis = vec3_length(intersection);
			}
			else if (intersect_ray_vs_traingle(ray, v3_f32(v1), v3_f32(v3), v3_f32(v2), intersection)) {
				
				dis = SV_MIN(vec3_length(intersection), dis);
			}
			
			if (dis < distance) {
				distance = dis;
				selected = entity;
			}
		}

		if (selected == 0) {

			// Select meshes
			for (CompIt it = comp_it_begin(mesh_id);
				 it.has_next;
				 comp_it_next(it))
			{
				Entity entity = it.entity;
				MeshComponent& m = *(MeshComponent*)it.comp;
				
				if (is_entity_selected(entity))
					break;
				
				if (m.mesh.get() == nullptr) break;

				XMMATRIX wm = get_entity_world_matrix(entity);

				XMMATRIX itm = XMMatrixInverse(0, wm);

				ray.origin = v3_f32(XMVector4Transform(ray_origin, itm));
				ray.direction = v3_f32(XMVector4Transform(ray_direction, itm));

				Mesh& mesh = *m.mesh.get();

				u32 triangles = u32(mesh.indices.size()) / 3u;

				for (u32 i = 0u; i < triangles; ++i) {

					u32 i0 = mesh.indices[i * 3u + 0u];
					u32 i1 = mesh.indices[i * 3u + 1u];
					u32 i2 = mesh.indices[i * 3u + 2u];

					v3_f32 p0 = mesh.positions[i0];
					v3_f32 p1 = mesh.positions[i1];
					v3_f32 p2 = mesh.positions[i2];

					v3_f32 intersection;

					if (intersect_ray_vs_traingle(ray, p0, p1, p2, intersection)) {

						f32 dis = vec3_length(intersection);
						if (dis < distance) {
							distance = dis;
							selected = entity;
						}

					}
				}
			}

			// Select sprites
			{
				ray.origin = v3_f32(ray_origin);
				ray.direction = v3_f32(ray_direction);

				for (CompIt it = comp_it_begin(sprite_id);
					 it.has_next;
					 comp_it_next(it))	
				{
					Entity entity = it.entity;
					
					if (is_entity_selected(entity))
						break;

					XMMATRIX tm = get_entity_world_matrix(entity);

					v0 = XMVector3Transform(p0, tm);
					v1 = XMVector3Transform(p1, tm);
					v2 = XMVector3Transform(p2, tm);
					v3 = XMVector3Transform(p3, tm);

					f32 dis = f32_max;

					v3_f32 intersection;

					// TODO: Ray vs Quad intersection

					if (intersect_ray_vs_traingle(ray, v3_f32(v0), v3_f32(v1), v3_f32(v2), intersection)) {

						dis = vec3_length(intersection);
					}
					else if (intersect_ray_vs_traingle(ray, v3_f32(v1), v3_f32(v3), v3_f32(v2), intersection)) {
			
						dis = SV_MIN(vec3_length(intersection), dis);
					}

					if (dis < distance) {
						distance = dis;
						selected = entity;
					}
					
				}
			}
		}
		
		select_entity(selected);
	}
    
    static void show_entity_popup(Entity entity, bool& destroy)
    {
		if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

			f32 y = 0.f;
			constexpr f32 H = 20.f;
	    
			destroy = gui_button("Destroy", 0u);
			y += H;
	    
			if (gui_button("Duplicate", 1u)) {
				duplicate_entity(entity);
			}
			y += H;
	    
			if (gui_button("Create Child", 2u)) {
				editor_create_entity(entity);
			}

			gui_end_popup();
		}
    }

    SV_INTERNAL void show_entity(Entity entity)
    {
		gui_push_id(entity);

		const char* name = get_entity_name(entity);
		if (string_size(name) == 0u) name = "Unnamed";

		u32 child_count = get_entity_childs_count(entity);

		bool destroy = false;

		if (child_count == 0u) {

			u32 flags = 0u;
			bool selected = is_entity_selected(entity);

			if (selected) {
				flags = GuiElementListFlag_Selected;
			}
	    
			if (gui_element_list(name, entity, flags)) {

				if (selected) {
					if (editor.selected_entities.size() == 1u)
						unselect_entity(entity);
					else {
						select_entity(entity, true);
					}
				}
				else {
					select_entity(entity);
				}
			}

			show_entity_popup(entity, destroy);
		}
		else {
	    
			if (gui_element_list(name, 0u)) {
		
				editor.selected_entities.reset();
				editor.selected_entities.push_back(entity);
			}

			show_entity_popup(entity, destroy);

			const Entity* childs;
			child_count = get_entity_childs_count(entity);
			get_entity_childs(entity, &childs);

			foreach(i, child_count) {

				show_entity(childs[i]);

				get_entity_childs(entity, &childs);
				child_count = get_entity_childs_count(entity);

				if (i < child_count)
					i += get_entity_childs_count(childs[i]);
			}
		}

		if (destroy) {
			destroy_entity(entity);
			
			foreach(i, editor.selected_entities.size()) {
				
				if (editor.selected_entities[i] == entity) {
					editor.selected_entities.erase(i);
					break;
				}
			}
		}

		gui_pop_id();
    }

    // Returns if the folder should be destroyed
    SV_AUX bool display_create_popup()
    {
		bool destroy = false;
	
		gui_push_id("Create Popup");
	
		if (gui_begin_popup(GuiPopupTrigger_Root)) {
		
			if (gui_button("Create Entity", 1u)) {
				editor_create_entity();
			}
		
			if (gui_button("Create Sprite", 2u)) {

				editor_create_entity(0, "Sprite", construct_entity_sprite);
			}

			if (gui_button("Create 2D Camera", 3u)) {

				editor_create_entity(0, "Camera", construct_entity_2D_camera);
			}

			gui_end_popup();
		}

		gui_pop_id();

		return destroy;
    }
    
    void display_entity_hierarchy()
    {	
		if (gui_begin_window("Hierarchy")) {

			u32 entity_count = get_entity_count();

			gui_begin_list(69u);
			
			for (u32 i = 0u; i < entity_count;) {

				Entity entity = get_entity_by_index(i);
				
				show_entity(entity);
				entity_count = get_entity_count();
	    
				if (entity_exists(entity)) {

					i += get_entity_childs_count(entity) + 1u;
				}
			}

			gui_push_id("Show Prefabs");
			
			for (PrefabIt it = prefab_it_begin();
				 it.has_next;
				 prefab_it_next(it))
			{
				Prefab prefab = it.prefab;

				const char* name = get_prefab_name(prefab);

				bool selected = editor.selected_prefab == prefab;
				u32 flags = selected ? GuiElementListFlag_Selected : 0u;

				if (gui_element_list(name, prefab, flags)) {

					if (selected) editor.selected_prefab = 0;
					else select_prefab(prefab);
				}

				if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

					if (gui_button("Create Entity")) {

						create_entity(0, "Pene", prefab);
					}
					
					gui_end_popup();
				}
			}

			gui_pop_id();

			gui_end_list();

			PrefabPackage* package;
			if (gui_recive_package((void**)&package, ASSET_BROWSER_PREFAB, GuiReciverTrigger_Root)) {

				load_prefab(package->filepath);
			}

			display_create_popup();
	    
			gui_end_window();
		}
    }

    void display_entity_inspector()
    {
		Entity selected = (editor.selected_entities.size() == 1u) ? editor.selected_entities.back() : 0;

		if (gui_begin_window("Inspector")) {

			if (selected != 0) {

				// Entity name
				{
					const char* entity_name = get_entity_name(selected);
					egui_header(entity_name, 0u);
				}

				// Entity flag
				{
					gui_text("Tag");

					char tag_name[TAG_NAME_SIZE + 1u];
					const char* tag = string_validate(get_entity_tag(selected));
					
					string_copy(tag_name, tag, TAG_NAME_SIZE + 1u);

					if (gui_text_field(tag_name, TAG_NAME_SIZE + 1u, 324894)) {
						set_entity_tag(selected, tag_name);
					}
				}

				// Entity transform
				egui_transform(selected);

				// Entity components
				{
					u32 comp_count = get_entity_component_count(selected);

					gui_push_id("Entity Components");

					foreach(comp_index, comp_count) {

						CompRef ref = get_entity_component_by_index(selected, comp_index);

						if (show_component_info(ref.comp_id, ref.comp)) {

							remove_entity_component(selected, ref.comp_id);
							comp_count = get_entity_component_count(selected);
						}
					}

					gui_pop_id();
				}

				// Entity Info
				gui_separator(3);
				
				if (gui_collapse("Entity Data")) {
					
					gui_push_id("Entity Data");

					SceneData* scene = get_scene_data();

					{
						bool main = scene->main_camera == selected;

						if (gui_checkbox("Main Camera", main)) {

							if (main) scene->main_camera = selected;
							else scene->main_camera = 0;
						}
					}

					gui_push_id("User");

					DisplayEntityEvent event;
					event.entity = selected;

					event_dispatch("display_entity_data", &event);
		    
					gui_pop_id(2u);
				}

				if (gui_begin_popup(GuiPopupTrigger_Root)) {
		    
					u32 count = get_component_register_count();
					foreach(i, count) {

						CompID comp_id = CompID(i);

						if (has_entity_component(selected, comp_id))
							continue;
			
						if (gui_button(get_component_name(comp_id), comp_id)) {
			    
							add_entity_component(selected, comp_id);
						}
					}

					gui_end_popup();
				}
			}
			else if (editor.selected_prefab) {

				Prefab prefab = editor.selected_prefab;
				
				// Entity name
				{
					const char* prefab_name = get_prefab_name(prefab);
					egui_header(prefab_name, 0u);
				}

				// Prefab components
				{
					u32 comp_count = get_prefab_component_count(prefab);

					gui_push_id("Prefab Components");

					foreach(comp_index, comp_count) {

						CompRef ref = get_prefab_component_by_index(prefab, comp_index);

						if (show_component_info(ref.comp_id, ref.comp)) {

							remove_prefab_component(prefab, ref.comp_id);
							comp_count = get_prefab_component_count(prefab);
						}
					}

					gui_pop_id();
				}

				if (gui_begin_popup(GuiPopupTrigger_Root)) {
		    
					u32 count = get_component_register_count();
					foreach(i, count) {

						CompID comp_id = CompID(i);

						if (has_prefab_component(prefab, comp_id))
							continue;
			
						if (gui_button(get_component_name(comp_id), comp_id)) {
			    
							add_prefab_component(prefab, comp_id);
						}
					}

					gui_end_popup();
				}

				if (gui_button("Save")) {
					save_prefab(prefab);
				}
			}

			gui_end_window();
		}
    }

    SV_INTERNAL void display_asset_browser()
    {
		AssetBrowserInfo& info = editor.asset_browser;
		
		bool update_browser = false;
		char next_filepath[FILEPATH_SIZE + 1] = "";

		string_copy(next_filepath, info.filepath, FILEPATH_SIZE + 1u);

		if (gui_begin_window("Create Folder", GuiWindowFlag_Temporal)) {

			static char foldername[FILENAME_SIZE + 1u] = "";
			
			gui_text_field(foldername, FILENAME_SIZE + 1u, 0u);

			if (gui_button("Create")) {
				
				update_browser = true;

				char filepath[FILEPATH_SIZE + 1u] = "assets/";
				string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
				string_append(filepath, foldername, FILEPATH_SIZE + 1u);

				if (!folder_create(filepath)) {
					SV_LOG_ERROR("Can't create the folder '%s'", filepath);
				}

				gui_hide_window("Create Folder");
				string_copy(foldername, "", FILENAME_SIZE + 1u);
			}

			gui_end_window();
		}

		if (gui_begin_window("Create Prefab", GuiWindowFlag_Temporal)) {

			CreatePrefabData& data = editor.create_prefab_data;

			gui_text_field(data.name, FILENAME_SIZE + 1u, 0u);

			if (gui_button("Create")) {

				char filepath[FILEPATH_SIZE + 1u] = "assets/";
				string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
				string_append(filepath, data.name, FILEPATH_SIZE + 1u);

				const char* extension = filepath_extension(data.name);
					
				if (!string_equals(string_validate(extension), ".prefab")) {
					string_append(filepath, ".prefab", FILEPATH_SIZE + 1u);
				}

				create_prefab_file(data.name, filepath);

				gui_hide_window("Create Prefab");
				
				data.name[0] = '\0';
			}
			
			gui_end_window();
		}

		if (gui_begin_window("Create Asset", GuiWindowFlag_Temporal)) {

			auto& data = info.create_asset_data;

			switch (data.state) {

			case 0:
				if (gui_button("Create SpriteSheet")) {
					data.state = 2u;
				}

				if (gui_button("Create Material")) {
					data.state = 1u;
				}
				break;

				// Create Material
			case 1:
			{
				gui_text_field(data.assetname, FILENAME_SIZE + 1u, 0u);

				if (gui_button("Create")) {

					Material mat;

					update_browser = true;

					char filepath[FILEPATH_SIZE + 1u] = "assets/";
					string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
					string_append(filepath, data.assetname, FILEPATH_SIZE + 1u);

					const char* extension = filepath_extension(data.assetname);
					extension = extension ? extension : "";
					
					if (!string_equals(extension, ".mat")) {
						string_append(filepath, ".mat", FILEPATH_SIZE + 1u);
					}
					
					save_material(mat, filepath);

					gui_hide_window("Create Asset");
				}
			}
			break;
			
			// Create Sprite Sheet
			case 2:
			{
				
			}
			break;
			
			}
			
			gui_end_window();
		}

		if (gui_begin_window("Import Model", GuiWindowFlag_Temporal)) {

			auto& data = editor.import_model_data;

			if (gui_collapse("Meshes")) {

				gui_push_id("Meshes");

				foreach(i, data.model_info.meshes.size()) {

					MeshInfo& mesh = data.model_info.meshes[i];
					gui_checkbox(mesh.name.c_str(), mesh.import, i);
				}

				gui_pop_id();
			}

			if (gui_collapse("Materials")) {

				gui_push_id("Materials");

				foreach(i, data.model_info.materials.size()) {

					MaterialInfo& mat = data.model_info.materials[i];
					gui_checkbox(mat.name.c_str(), mat.import, i);
				}

				gui_pop_id();
			}

			if (gui_button("Import")) {

				char dst[FILEPATH_SIZE + 1u];
				string_copy(dst, "assets/", FILEPATH_SIZE + 1u);
				string_append(dst, info.filepath, FILEPATH_SIZE + 1u);
				
				if (import_model(dst, data.model_info)) {
					SV_LOG_INFO("Model imported at '%s'", dst);
				}
				else SV_LOG_ERROR("Can't import the model at '%s'", dst);
				
				gui_hide_window("Import Model");
				data = {};
			}
			
			gui_end_window();
		}

		if (gui_begin_window("Asset Browser")) {

			// TEMP
			if (input.unused && input.keys[Key_Control] && input.keys[Key_B] == InputState_Pressed) {

				string_copy(next_filepath, info.filepath, FILEPATH_SIZE + 1u);

				char* end = next_filepath;
				char* it = next_filepath + string_size(next_filepath) - 1u;

				while (it > end && *it != '/') --it;

				if (it >= end) next_filepath[0] = '\0';
				else if (*it == '/') *(it + 1u) = '\0';
		
		
				update_browser = true;
			}

			if (gui_select_filepath(info.filepath, next_filepath, 0u)) {

				update_browser = true;
			}
	    
			{
				gui_begin_grid(85.f, 10.f, 1u);

				foreach(i, info.elements.size()) {

					const AssetElement& e = info.elements[i];

					gui_push_id((u64)e.type);

					// TODO: ignore unused elements
					switch (e.type) {

					case AssetElementType_Directory:
					{
						if (gui_asset_button(e.name, editor.image.get(), editor.TEXCOORD_FOLDER, 0u)) {
			    
							if (e.type == AssetElementType_Directory && !update_browser) {

								update_browser = true;

								size_t new_size = strlen(info.filepath) + strlen(e.name) + 1u;
								if (new_size < FILEPATH_SIZE)
									sprintf(next_filepath, "%s%s/", info.filepath, e.name);
								else
									SV_LOG_ERROR("This filepath exceeds the max filepath size");
							}
						}

						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove", 0u)) {

								update_browser = true;

								char filepath[FILEPATH_SIZE + 1u] = "assets/";
								string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
								string_append(filepath, e.name, FILEPATH_SIZE + 1u);

								if (folder_remove(filepath)) {
									SV_LOG_INFO("Folder '%s' removed", filepath);
								}
								else {
									SV_LOG_ERROR("Can't remove the folder '%s'", filepath);
								}
							}
							
							gui_end_popup();
						}
					}
					break;

					case AssetElementType_Prefab:
					{
						PrefabPackage pack;

						sprintf(pack.filepath, "assets/%s%s", info.filepath, e.name);
						
						gui_asset_button(e.name, nullptr, {0.f, 0.f, 1.f, 1.f}, 0u);
						gui_send_package(&pack, sizeof(PrefabPackage), ASSET_BROWSER_PREFAB);
					}
					break;

					case AssetElementType_Texture:
					case AssetElementType_Mesh:
					case AssetElementType_Material:
					case AssetElementType_SpriteSheet:
					{
						AssetPackage pack;

						sprintf(pack.filepath, "assets/%s%s", info.filepath, e.name);

						if (e.type == AssetElementType_Texture) {

							TextureAsset tex;

							if (load_asset_from_file(tex, pack.filepath, AssetLoadingPriority_GetIfExists)) {

								gui_asset_button(e.name, tex.get(), {0.f, 0.f, 1.f, 1.f}, 0u);
							}
							// TODO: Set default image
							else {
								gui_asset_button(e.name, nullptr, {0.f, 0.f, 1.f, 1.f}, 0u);
							}
						}
						else {

							gui_asset_button(e.name, nullptr, {0.f, 0.f, 1.f, 1.f}, 0u);
						}

						u32 id;

						switch (e.type) {

						case AssetElementType_Texture:
							id = ASSET_BROWSER_PACKAGE_TEXTURE;
							break;

						case AssetElementType_Mesh:
							id = ASSET_BROWSER_PACKAGE_MESH;
							break;

						case AssetElementType_Material:
							id = ASSET_BROWSER_PACKAGE_MATERIAL;
							break;

						case AssetElementType_SpriteSheet:
							id = ASSET_BROWSER_PACKAGE_SPRITE_SHEET;
							break;

						default:
							id = u32_max;
							break;
						}

						if (id != u32_max) {
			    
							gui_send_package(&pack, sizeof(AssetPackage), id);
						}

						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove")) {
								
							}

							if (id == ASSET_BROWSER_PACKAGE_SPRITE_SHEET) {

								if (gui_button("Edit")) {

									char filepath[FILEPATH_SIZE + 1u];
									string_copy(filepath, "assets/", FILEPATH_SIZE + 1u);
									string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
									string_append(filepath, e.name, FILEPATH_SIZE + 1u);
									
									edit_sprite_sheet(filepath);
								}
							}
							
							gui_end_popup();
						}
					}
					break;

					}
				}

				gui_end_grid();
			}

			// Update per time
			if (!update_browser) {
				f64 now = timer_now();

				if (now - info.last_update > 1.0) {
					update_browser = true;
					sprintf(next_filepath, "%s", info.filepath);
				}
			}

			// Update browser elements
			if (update_browser) {

				strcpy(info.filepath, next_filepath);
				sprintf(next_filepath, "assets/%s", info.filepath);
		
				while (!file_exists(next_filepath) && next_filepath[0]) {

					size_t size = strlen(next_filepath) - 1u;
		    
					next_filepath[size] = '\0';
		    
					while (size && next_filepath[size] != '/') {
						--size;
						next_filepath[size] = '\0';
					}
				}

				if (strlen(next_filepath) < strlen("assets/")) {
					strcpy(next_filepath, "assets/");
				}
		
				// Clear browser data
				info.elements.clear();

				FolderIterator it;
				FolderElement e;
		
				bool res = folder_iterator_begin(next_filepath, &it, &e);

				if (res) {

					do {

						if (strcmp(e.name, ".") == 0 || strcmp(e.name, "..") == 0)
							continue;
			
						AssetElement element;
			
						// Select element type
						if (e.is_file) {

							if (strcmp(e.extension, "mesh") == 0) element.type = AssetElementType_Mesh;
							else if (strcmp(e.extension, "mat") == 0) element.type = AssetElementType_Material;
							else if (strcmp(e.extension, "png") == 0 || strcmp(e.extension, "jpg") == 0 || strcmp(e.extension, "gif") == 0 || strcmp(e.extension, "jpeg") == 0) element.type = AssetElementType_Texture;
							else if (strcmp(e.extension, "sprites") == 0) element.type = AssetElementType_SpriteSheet;
							else if (strcmp(e.extension, "prefab") == 0) element.type = AssetElementType_Prefab;
							else element.type = AssetElementType_Unknown;
						}
						else {
							element.type = AssetElementType_Directory;
						}

						// Set name
						size_t name_size = strlen(e.name);
						memcpy(element.name, e.name, name_size);
						element.name[name_size] = '\0';

						info.elements.emplace_back(element);
					}
					while (folder_iterator_next(&it, &e));
				}
				else {

					SV_LOG_ERROR("Can't create asset browser content at '%s'", next_filepath);
				}
		
		
				strcpy(info.filepath, next_filepath + strlen("assets/"));
				info.last_update = timer_now();
			}

			if (gui_begin_popup(GuiPopupTrigger_Root)) {

				u64 id = 0u;

				if (gui_button("Create Folder", id++)) {

					gui_show_window("Create Folder");
					gui_close_popup();
				}

				gui_separator(1);

				if (gui_button("Import Model")) {

					auto& data = editor.import_model_data;
					data.model_info = {};

					char filepath[FILEPATH_SIZE + 1u] = "";

					const char* filters[] = {
						"Wavefront OBJ (.obj)", "*.obj",
						"All", "*",
						""
					};
			
					if (file_dialog_open(filepath, 2u, filters, "")) {
			    
						if (load_model(filepath, data.model_info)) {

							gui_show_window("Import Model");
							gui_close_popup();
						}
						else {
							SV_LOG_ERROR("Can't load the model '%s'", filepath);
						}
					}
				}

				if (gui_button("Create Prefab")) {
					gui_show_window("Create Prefab");
					gui_close_popup();
				}

				if (gui_button("Create Asset")) {

					auto& data = info.create_asset_data;

					data.state = 0u;
					gui_show_window("Create Asset");
					gui_close_popup();
				}

				if (gui_button("Create Entity Model")) {

					char filepath[FILEPATH_SIZE + 1u] = "assets/";
					string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);

					Entity parent = create_entity(0, filepath_name(filepath));

					if (parent) {

						if (create_entity_model(parent, filepath)) {
							SV_LOG_INFO("Entity model created: '%s'", filepath);
						}
						else {
							SV_LOG_ERROR("Can't create entity model: '%s'", filepath);
						}
					}
				}

				if (gui_button("Create Sprite Sheet")) {

					char filepath[FILEPATH_SIZE + 1u] = "assets/";

					string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
					string_append(filepath, "test.sprites", FILEPATH_SIZE + 1u);

					bool show_win = false;

					if (!file_exists(filepath)) {
					
						Serializer s;
						serialize_begin(s);

						serialize_sprite_sheet(s, {});

						if (!serialize_end(s, filepath)) {
							SV_LOG_ERROR("Can't create the sprite sheet at '%s'", filepath);
						}
						else {

							show_win = true;
						}
					}
					else show_win = true;

					if (show_win) {

						edit_sprite_sheet(filepath);
					}
				}
		
				gui_end_popup();
			}
	    
			gui_end_window();
		}
    }

    void display_scene_settings()
    {
		SceneData& s = *get_scene_data();

		if (gui_begin_window("Go to scene", GuiWindowFlag_Temporal)) {
		
			gui_text_field(editor.next_scene_name, SCENENAME_SIZE + 1u, 0u);

			gui_separator(1);

			if (gui_button("GO!")) {
				set_scene(editor.next_scene_name);
				string_copy(editor.next_scene_name, "", SCENENAME_SIZE + 1u);

				gui_hide_window("Go to scene");
			}

			gui_end_window();
		}
	
		if (gui_begin_window("Scene Settings")) {

			gui_text(get_scene_name(), 0u);

			if (gui_button("Go to scene"))
				gui_show_window("Go to scene");

			if (gui_collapse("Rendering")) {

				gui_drag_color("Ambient Light", s.ambient_light);

				// Skybox
				{
					gui_button("Skybox");

					AssetPackage* package;
					if (gui_recive_package((void**)&package, ASSET_BROWSER_PACKAGE_TEXTURE, GuiReciverTrigger_LastWidget)) {

						set_skybox(package->filepath);
					}
				}
			}

			if (gui_collapse("Physics")) {

				if (s.physics.in_3D) {
					gui_drag_v3_f32("Gravity", s.physics.gravity, 0.01f, -f32_max, f32_max);
				}
				else {
					v2_f32 v2;

					v2 = vec3_to_vec2(s.physics.gravity);
					if (gui_drag_v2_f32("Gravity", v2, 0.01f, -f32_max, f32_max))
						s.physics.gravity = vec2_to_vec3(v2);
				}
				
				gui_checkbox("3D", s.physics.in_3D);
			}
	    
			gui_end_window();
		}
    }

    SV_INTERNAL void display_spritesheet_editor()
    {
		if (gui_begin_window("SpriteSheet Editor")) {

			auto& data = editor.sprite_sheet_editor_data;

			SpriteSheet* sheet = data.current_sprite_sheet.get();

			if (sheet) {

				bool save = false;

				GPUImage* image = sheet->texture.get();

				// TODO: back button in the window
				
				switch (data.state) {

				case SpriteSheetEditorState_Main:
				{
					if (gui_button("Sprites", 0u)) {
						data.next_state = SpriteSheetEditorState_SpriteList;
					}
					if (gui_button("Sprite Animations", 1u)) {
						data.next_state = SpriteSheetEditorState_AnimationList;
					}
				}
				break;

				case SpriteSheetEditorState_SpriteList:
				{
					u64 id = 0u;

					if (egui_comp_texture("Texture", id++, &sheet->texture))
						save = true;
					
					if (gui_button("New Sprite", id++)) {
						data.next_state = SpriteSheetEditorState_NewSprite;
					}

					u32 remove_id = u32_max;

					GuiSpritePackage package;
					string_copy(package.sprite_sheet_filepath, data.current_sprite_sheet.get_filepath(), FILEPATH_SIZE + 1u);
					
					for (auto it = sheet->sprites.begin();
						 it.has_next();
						 ++it)
					{
						u32 index = it.get_index();
						
						Sprite& sprite = *it;
						if (gui_image_button(sprite.name, image, sprite.texcoord, index + 28349u)) {

							data.modifying_id = index;
							data.next_state = SpriteSheetEditorState_ModifySprite;
						}

						package.sprite_id = it.get_index();
						gui_send_package(&package, sizeof(GuiSpritePackage), GUI_PACKAGE_SPRITE);
						
						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove", 0u)) {

								remove_id = index;
							}

							gui_end_popup();
						}
					}

					if (remove_id != u32_max) {

						sheet->sprites.erase(remove_id);
						save = true;
					}
				}
				break;

				case SpriteSheetEditorState_NewSprite:
				{
					Sprite& sprite = data.temp_sprite;
					
					gui_image(image, 80.f, sprite.texcoord, 0u);

					gui_text_field(sprite.name, SPRITE_NAME_SIZE + 1u, 1u);

					gui_drag_v4_f32("Texcoord", sprite.texcoord, 0.001f, 0.f, 1.f, 2u);

					if (gui_button("Save", 3u)) {

						if (sheet->add_sprite(NULL, sprite.name, sprite.texcoord)) {
							
							save = true;
							data.next_state = SpriteSheetEditorState_SpriteList;
						}
					}
				}
				break;

				case SpriteSheetEditorState_ModifySprite:
				{
					if (!sheet->sprites.exists(data.modifying_id))
						data.next_state = SpriteSheetEditorState_SpriteList;
					else {
						
						Sprite& sprite = data.temp_sprite;
					
						gui_image(image, 80.f, sprite.texcoord, 0u);

						gui_text_field(sprite.name, SPRITE_NAME_SIZE + 1u, 1u);

						gui_drag_v4_f32("Texcoord", sprite.texcoord, 0.001f, 0.f, 1.f, 2u);

						if (gui_button("Save sprite", 3u)) {
							
							if (sheet->modify_sprite(data.modifying_id, sprite.name, sprite.texcoord)) {
								
								// TEMP
								data.next_state = SpriteSheetEditorState_SpriteList;

								save = true;
							}
						}
					}
				}
				break;

				case SpriteSheetEditorState_AnimationList:
				{
					u64 id = 0u;
					
					if (gui_button("New Sprite Animation", id++)) {
						data.next_state = SpriteSheetEditorState_NewAnimation;
					}

					u32 remove_id = u32_max;

					GuiSpriteAnimationPackage package;
					string_copy(package.sprite_sheet_filepath, data.current_sprite_sheet.get_filepath(), FILEPATH_SIZE + 1u);
					
					for (auto it = sheet->sprite_animations.begin();
						 it.has_next();
						 ++it)
					{
						u32 index = it.get_index();
						SpriteAnimation& anim = *it;

						u32 current_sprite = u32(data.simulation_time / anim.frame_time) % anim.frames;
						v4_f32 texcoord = sheet->get_sprite_texcoord(anim.sprites[current_sprite]);
						
						if (gui_image_button(anim.name, image, texcoord, index + 28349u)) {

							data.modifying_id = index;
							data.next_state = SpriteSheetEditorState_ModifyAnimation;
						}

						package.animation_id = it.get_index();

						gui_send_package(&package, sizeof(GuiSpriteAnimationPackage), GUI_PACKAGE_SPRITE_ANIMATION);
						
						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove", 0u)) {

								remove_id = index;
							}

							gui_end_popup();
						}
					}

					if (remove_id != u32_max) {

						sheet->sprite_animations.erase(remove_id);
						save = true;
					}

					data.simulation_time += engine.deltatime;
				}
				break;

				case SpriteSheetEditorState_NewAnimation:
				{
					SpriteAnimation& anim = data.temp_anim;

					if (anim.frames) {
						
						u32 current_spr = u32(data.simulation_time / anim.frame_time) % anim.frames;
						v4_f32 texcoord = sheet->get_sprite_texcoord(anim.sprites[current_spr]);
					
						gui_image(image, 80.f, texcoord, 0u);
					}
					else gui_image(NULL, 80.f, {}, 0u);

					gui_text_field(anim.name, SPRITE_NAME_SIZE + 1u, 1u);
					
					foreach(i, anim.frames) {

						u32 spr_id = anim.sprites[i];
						
						if (sheet->sprites.exists(spr_id)) {

							const Sprite& spr = sheet->sprites[spr_id];

							if (gui_image_button(spr.name, image, spr.texcoord, spr_id + 84390u)) {
								
							}
						}
						else {
							// TODO
						}
					}

					if (gui_button("Add Sprite", 2u)) {
						data.next_state = SpriteSheetEditorState_AddSprite;
					}
					if (gui_button("Save", 3u)) {
						if (sheet->add_sprite_animation(NULL, anim.name, anim.sprites, anim.frames, anim.frame_time)) {
							
							save = true;
							data.next_state = SpriteSheetEditorState_AnimationList;
						}
					}

					data.simulation_time += engine.deltatime;
				}
				break;

				case SpriteSheetEditorState_AddSprite:
				{
					for (auto it = sheet->sprites.begin();
						 it.has_next();
						 ++it)
					{
						if (gui_image_button(it->name, image, it->texcoord, it.get_index() + 38543u)) {

							data.next_state = SpriteSheetEditorState_NewAnimation;
							u32& spr = data.temp_anim.sprites[data.temp_anim.frames++];
							spr = it.get_index();
						}
					}
				}
				break;
					
				}

				// Initialize new state
				if (data.state != data.next_state) {

					switch (data.next_state) {

					case SpriteSheetEditorState_NewSprite:
					{
						string_copy(data.temp_sprite.name, "Name", SPRITE_NAME_SIZE + 1u);
						data.temp_sprite.texcoord = {0.f, 0.f, 1.f, 1.f};
					}
					break;

					case SpriteSheetEditorState_NewAnimation:
					{
						if (data.state != SpriteSheetEditorState_AddSprite) {
							
							string_copy(data.temp_anim.name, "Name", SPRITE_NAME_SIZE + 1u);
							data.temp_anim.frames = 0u;
							data.temp_anim.frame_time = 0.1f;
						}
					}
					break;

					case SpriteSheetEditorState_ModifySprite:
					{
						Sprite& s = sheet->sprites[data.modifying_id];
						string_copy(data.temp_sprite.name, s.name, SPRITE_NAME_SIZE + 1u);
						data.temp_sprite.texcoord = s.texcoord;
					}
					break;

					case SpriteSheetEditorState_AnimationList:
					{
						data.simulation_time = 0.f;
					}
					break;
						
					}

					data.state = data.next_state;
				}

				// Save SpriteSheet
				if (save) {

					Serializer s;
					serialize_begin(s);

					serialize_sprite_sheet(s, *sheet);

					serialize_end(s, data.current_sprite_sheet.get_filepath());
				}
			}
	    
			gui_end_window();
		}
		// Free asset
		else {
			unload_asset(editor.sprite_sheet_editor_data.current_sprite_sheet);
		}
    }

	SV_INTERNAL void display_material_editor()
	{
		if (gui_begin_window("Material Editor")) {

			auto& data = editor.material_editor_data;

			MaterialAsset& mat = data.material;

			if (mat.get()) {
	
				gui_text("Pipeline Settings");
				gui_separator(1);
			
				gui_checkbox("Transparent", mat->transparent);
			
				gui_text("Values");
				gui_separator(1);
			
				gui_drag_color("Ambient Color", mat->ambient_color);
				gui_drag_color("Diffuse Color", mat->diffuse_color);
				gui_drag_color("Specular Color", mat->specular_color);
				gui_drag_color("Emissive Color", mat->emissive_color);
				gui_drag_f32("Shininess", mat->shininess, 0.01f, 0.f, 300.f);
			}
			
			gui_end_window();
		}
	}

    SV_INTERNAL void display_gui()
    {
		if (_gui_begin()) {

			if (there_is_scene() && dev.debug_draw) {

				gui_begin_top(GuiTopLocation_Center);

				if (gui_image_button(NULL, editor.image.get(), GlobalEditorData::TEXCOORD_PLAY, 324, GuiImageButtonFlag_NoBackground)) {
					dev.next_engine_state = EngineState_Play;
				}
				u32 flags = (dev.engine_state == EngineState_Edit) ? GuiImageButtonFlag_Disabled : 0u;
				if (gui_image_button(NULL, editor.image.get(), GlobalEditorData::TEXCOORD_PAUSE, 754, GuiImageButtonFlag_NoBackground | flags)) {
					dev.next_engine_state = EngineState_Play;
				}

				gui_end_top();

				if (gui_begin_window("Editor View")) {

					gui_image(editor.offscreen, 0, 0, GuiImageFlag_Fullscreen);

					editor.editor_view_size = gui_root_size();
					editor.editor_view_position = gui_root_position();
					editor.in_editor_view = gui_image_catch_input(0);
					
					gui_end_window();
				}

				// Window management
				if (gui_begin_window("Window Manager", GuiWindowFlag_NoClose)) {

					const char* windows[] = {
						"Hierarchy",
						"Inspector",
						"Asset Browser",
						"Scene Settings",
						"SpriteSheet Editor",
						"Material Editor",
					};

					foreach(i, SV_ARRAY_SIZE(windows)) {

						const char* name = windows[i];

						if (!gui_showing_window(name)) {

							if (gui_button(name, i)) {

								gui_show_window(name);
							}
						}
					}

					gui_separator(3);

					gui_checkbox("Colisions", dev.draw_collisions);
					gui_checkbox("Postprocessing", dev.postprocessing);

					if (gui_button("Exit Project")) {
						dev.next_engine_state = EngineState_ProjectManagement;
					}

					// TEMP

					gui_separator(2);

					auto& type = editor.tool_data.tool_type;
					bool gizmos = type == EditorToolType_Gizmos;
					bool terrain_brush = type == EditorToolType_TerrainBrush;

					if (gui_checkbox("Gizmos", gizmos)) {
						if (gizmos) type = EditorToolType_Gizmos;
						else type = EditorToolType_None;
					}
					if (gui_checkbox("Terrain Brush", terrain_brush)) {
						if (terrain_brush) type = EditorToolType_TerrainBrush;
						else type = EditorToolType_None;
					}

					// TEMP
					gui_separator(2);
					{
						auto& data = editor.tool_data.terrain_brush_data;
						gui_drag_f32("Strength", data.strength, 0.1f, 0.f, f32_max);
						gui_drag_f32("Range", data.range, 0.1f, 0.f, f32_max);
						gui_drag_f32("Min Height", data.min_height, 0.1f, 0.f, f32_max);
						gui_drag_f32("Max Height", data.max_height, 0.1f, 0.f, f32_max);
					}
					
					gui_end_window();
				}
		
				display_entity_hierarchy();
				display_entity_inspector();
				display_asset_browser();
				display_scene_settings();
				display_spritesheet_editor();
				display_material_editor();
				gui_display_style_editor();

				event_dispatch("display_gui", NULL);

			}
			else {
				// TODO
			}

			_gui_end();
		}

		// Change input and adjust cameras
		{
			f32 aspect = os_window_aspect() * (SV_MAX(editor.editor_view_size.x, 0.01f) / SV_MAX(editor.editor_view_size.y, 0.01f));
			dev.camera.adjust(aspect);

			editor.absolute_mouse_position = input.mouse_position;
			editor.absolute_mouse_last_position = input.mouse_last_pos;
			editor.absolute_mouse_dragged = input.mouse_dragged;

			input.mouse_position = ((input.mouse_position + 0.5f) - (editor.editor_view_position - editor.editor_view_size * 0.5f)) / editor.editor_view_size;
			input.mouse_position -= 0.5f;
			
			input.mouse_last_pos = ((input.mouse_last_pos + 0.5f) - (editor.editor_view_position - editor.editor_view_size * 0.5f)) / editor.editor_view_size;
			input.mouse_last_pos -= 0.5f;

			input.mouse_dragged *= editor.editor_view_size;

			if (editor.in_editor_view || editor.camera_focus) {

				input.unused = true;
			}
		}
    }

	SV_AUX v3_f32 compute_terrain_position(f32 h, u32 x, u32 z, u32 size_x, u32 size_z)
	{
		v3_f32 p;
		p.y = h;
	
		p.x = f32(x) / f32(size_x) - 0.5f;
		p.z = -(f32(z) / f32(size_z) - 0.5f);

		return p;
	}

	SV_AUX void update_terrain_brush()
	{
		if (input.unused) {

			bool left = input.mouse_buttons[MouseButton_Left];
			bool right = input.mouse_buttons[MouseButton_Right];

			if (left || right) {

				input.unused = false;
				
				Ray ray = screen_to_world_ray(input.mouse_position, dev.camera.position, dev.camera.rotation, &dev.camera);

				v3_f32 closest_pos;
				TerrainComponent* terrain = NULL;
				f32 closest_distance = f32_max;
				Entity entity = 0;

				for (CompIt it = comp_it_begin(get_component_id("Terrain"));
					 it.has_next;
					 comp_it_next(it))
				{
					TerrainComponent* t = (TerrainComponent*)it.comp;
					v3_f32 pos;
					
					if (terrain_intersect_ray(*t, it.entity, ray, pos)) {

						f32 distance = vec3_distance(pos, ray.origin);
						
						if (distance < closest_distance) {

							closest_distance = distance;
							terrain = t;
							closest_pos = pos;
							entity = it.entity;
						}
					}
				}

				if (terrain) {

					TerrainBrushData& data = editor.tool_data.terrain_brush_data;
					XMMATRIX matrix = get_entity_world_matrix(entity);

					foreach(z, terrain->resolution.y) {
						foreach(x, terrain->resolution.x) {

							f32& height = terrain->heights[x + z * terrain->resolution.x];

							v3_f32 pos = compute_terrain_position(height, x, z, terrain->resolution.x, terrain->resolution.y);
							pos = XMVector4Transform(vec3_to_dx(pos, 1.f), matrix);

							f32 distance = vec3_distance(pos, closest_pos);
								
							if (distance < data.range) {

								f32 value = ((data.range - distance) / data.range) * engine.deltatime * data.strength;

								if (left) height += value;
								if (right) height -= value;
								
								height = SV_MAX(SV_MIN(height, data.max_height), data.min_height);
							}
						}
					}

					terrain->dirty = true;
				}
			}
		}
	}

		

    SV_INTERNAL void do_picking_stuff()
    {
		// Update tool
		{
			EditorToolData& data = editor.tool_data;
			
			switch (data.tool_type) {

			case EditorToolType_TerrainBrush:
				update_terrain_brush();
				break;

			case EditorToolType_Gizmos:
				update_gizmos();
				break;
				
			}
		}
	
		// Entity selection
		if (input.unused && input.mouse_buttons[MouseButton_Left] == InputState_Released)
			select_entity();
    }
    
    SV_INTERNAL void update_edit_state()
    {
		for (u32 i = 0u; i < editor.selected_entities.size();) {

			if (!entity_exists(editor.selected_entities[i]))
				editor.selected_entities.erase(i);
			else ++i;
		}
	
		display_gui();

		if (dev.debug_draw && there_is_scene()) {

			control_camera();
		}

		if (there_is_scene())
			do_picking_stuff();
    }

    SV_INTERNAL void update_play_state()
    {
		if (dev.debug_draw) {
			display_gui();

			if (there_is_scene()) {
				control_camera();
				do_picking_stuff();
			}
		}
    }

    void update_project_state()
    {
		if (_gui_begin()) {

			// TEST
			{
				if (gui_begin_window("Test")) {

					TextureAsset tex;

					if (load_asset_from_file(tex, "$system/images/skymap.jpeg"))
						gui_image(tex.get(), 0.f, 0u, GuiImageFlag_Fullscreen);
		    
					gui_end_window();
				}
			}

			if (gui_button("New project", 0u)) {
		
				char path[FILEPATH_SIZE + 1u] = "";
		    
				if (file_dialog_save(path, 0u, nullptr, "")) {

					char* extension = filepath_extension(path);
					if (extension != nullptr) {
						*extension = '\0';
					}
					strcat(path, ".silver");

					const char* content = "test";
					bool res = file_write_text(path, content, strlen(content));

					if (res) {
			    
						char* name = filepath_name(path);
						*name = '\0';

						char folderpath[FILEPATH_SIZE + 1u];
			    
						sprintf(folderpath, "%s%s", path, "assets");
						res = folder_create(folderpath);
			    
						if (res) {
							sprintf(folderpath, "%s%s", path, "src");
							res = folder_create(folderpath);
						}

						if (res) {

							sprintf(folderpath, "%s%s", path, "src/build_unit.cpp");
							res = file_copy("$system/default_code.cpp", folderpath);
						}
					}
		    
			
					if (res)
						SV_LOG_INFO("Project in '%s' created", path);
		    
					else {
						SV_LOG_ERROR("Can't create the project in '%s'", path);
					}
				}
			}
			if (gui_button("Open project", 1u)) {

				char path[FILEPATH_SIZE + 1u] = "";

				const char* filter[] = {
					"Silver Engine (.silver)", "*.silver",
					"All", "*",
					""
				};

				if (file_dialog_open(path, 2u, filter, "")) {
			
					*filepath_name(path) = '\0';
					_engine_initialize_project(path);
					dev.next_engine_state = EngineState_Edit;
				}
			}
	    
			_gui_end();
		}    
    }
    
    void _editor_update()
    {
		bool exit = false;
	
		// CHANGE EDITOR MODE
		if (dev.next_engine_state != EngineState_None) {

			switch (dev.next_engine_state) {

			case EngineState_ProjectManagement:
			{
				_engine_close_project();
				editor.selected_entities.reset();
				exit = true;
				engine.update_scene = false;
				_gui_load("PROJECT");
			}
			break;

			case EngineState_Edit:
			{
				SV_LOG_INFO("Starting edit state");
				// TODO: Handle error
				_start_scene(get_scene_name());
		
				dev.debug_draw = true;
				engine.update_scene = false;
				_gui_load(engine.project_path);
			} break;

			case EngineState_Play:
			{
				SV_LOG_INFO("Starting play state");

				engine.update_scene = true;
			
				if (dev.engine_state == EngineState_Edit) {

					// TODO: handle error
					save_scene();
					_start_scene(get_scene_name());

					dev.debug_draw = false;
					dev.draw_collisions = false;
					editor.selected_entities.reset();
				}
			} break;

			case EngineState_Pause:
			{
				SV_LOG_INFO("Game paused");
				engine.update_scene = false;
			} break;
			
			}

			dev.engine_state = dev.next_engine_state;
			dev.next_engine_state = EngineState_None;

			if (exit)
				return;
		}

		update_key_shortcuts();
	
		switch (dev.engine_state) {

		case EngineState_Edit:
			update_edit_state();
			break;

		case EngineState_Play:
			update_play_state();
			break;

		case EngineState_ProjectManagement:
			update_project_state();
			break;
	    
		}
    }

    SV_INTERNAL void draw_edit_state(CommandList cmd)
    {
		if (!dev.debug_draw) return;
	
		imrend_begin_batch(cmd);

		imrend_camera(ImRendCamera_Editor, cmd);

		u32 light_id = get_component_id("Light");
		u32 mesh_id = get_component_id("Mesh");
		u32 sprite_id = get_component_id("Sprite");
		u32 body_id = get_component_id("Body");
		u32 box_id = get_component_id("Box Collider");

		// Draw selected entity
		for (Entity entity : editor.selected_entities) {

			MeshComponent* mesh_comp = (MeshComponent*)get_entity_component(entity, mesh_id);
			SpriteComponent* sprite_comp = (SpriteComponent*)get_entity_component(entity, sprite_id);

			if (mesh_comp && mesh_comp->mesh.get()) {

				u8 alpha = 5u + u8(f32(sin(timer_now() * 3.5) + 1.0) * 30.f * 0.5f);
				XMMATRIX wm = get_entity_world_matrix(entity);

				imrend_push_matrix(wm, cmd);
				imrend_draw_mesh_wireframe(mesh_comp->mesh.get(), Color::Red(alpha), cmd);
				imrend_pop_matrix(cmd);
			}
			if (sprite_comp) {

				XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
				XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
				XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
				XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

				XMMATRIX tm = get_entity_world_matrix(entity);

				p0 = XMVector3Transform(p0, tm);
				p1 = XMVector3Transform(p1, tm);
				p2 = XMVector3Transform(p2, tm);
				p3 = XMVector3Transform(p3, tm);

				u8 alpha = 50u + u8(f32(sin(timer_now() * 3.5) + 1.0) * 200.f * 0.5f);
				Color selection_color = Color::Red(alpha);

				imrend_draw_line(v3_f32(p0), v3_f32(p1), selection_color, cmd);
				imrend_draw_line(v3_f32(p1), v3_f32(p3), selection_color, cmd);
				imrend_draw_line(v3_f32(p3), v3_f32(p2), selection_color, cmd);
				imrend_draw_line(v3_f32(p2), v3_f32(p0), selection_color, cmd);
			}
		}

		// Draw 2D grid
		if (dev.camera.projection_type == ProjectionType_Orthographic && dev.debug_draw) {

			f32 width = dev.camera.width;
			f32 height = dev.camera.height;
			f32 mag = dev.camera.getProjectionLength();

			u32 count = 0u;
			for (f32 i = 0.01f; count < 3u; i *= 10.f) {

				if (mag / i <= 50.f) {

					Color color;

					switch (count++)
					{
					case 0:
						color = Color::Gray(50);
						break;

					case 1:
						color = Color::Gray(100);
						break;

					case 2:
						color = Color::Gray(150);
						break;

					case 3:
						color = Color::Gray(200);
						break;
					}

					color.a = 10u;

					imrend_draw_orthographic_grip(vec3_to_vec2(dev.camera.position), {}, { width, height }, i, color, cmd);
				}
			}
		}

		// Draw light probes
		{
			XMMATRIX tm;
			
			for (CompIt it = comp_it_begin(light_id);
				 it.has_next;
				 comp_it_next(it))
			{
				Entity entity = it.entity;
				LightComponent& light = *(LightComponent*)it.comp;
				
				v3_f32 pos = get_entity_world_position(entity);

				f32 min_scale = relative_scalar(0.02f, pos);
				f32 scale = SV_MAX(min_scale, 1.f);
					
				tm = XMMatrixScaling(scale, scale, 1.f) * XMMatrixRotationQuaternion(vec4_to_dx(dev.camera.rotation)) * XMMatrixTranslation(pos.x, pos.y, pos.z);

				imrend_push_matrix(tm, cmd);

				imrend_draw_sprite({}, { 1.f, 1.f }, Color::White(), editor.image.get(), GPUImageLayout_ShaderResource, editor.TEXCOORD_LIGHT_PROBE, cmd);

				imrend_pop_matrix(cmd);

				if (is_entity_selected(entity) && light.light_type == LightType_Direction) {

					// Draw light direction

					v3_f32 dir = v3_f32::forward();

					XMVECTOR quat = vec4_to_dx(get_entity_world_rotation(entity));

					tm = XMMatrixRotationQuaternion(quat);

					dir = XMVector3Transform(vec3_to_dx(dir), tm);

					dir = vec3_normalize(dir) * scale * 1.5f;
					imrend_draw_line(pos, pos + dir, light.color, cmd);
				}	
			}
		}

		// Draw collisions
		if (dev.draw_collisions) {
			
			XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.5f, 1.f);
			XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.5f, 1.f);
			XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.5f, 1.f);
			XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.5f, 1.f);
			XMVECTOR p4 = XMVectorSet(-0.5f, 0.5f, -0.5f, 1.f);
			XMVECTOR p5 = XMVectorSet(0.5f, 0.5f, -0.5f, 1.f);
			XMVECTOR p6 = XMVectorSet(-0.5f, -0.5f, -0.5f, 1.f);
			XMVECTOR p7 = XMVectorSet(0.5f, -0.5f, -0.5f, 1.f);

			XMVECTOR v0, v1, v2, v3, v4, v5, v6, v7;

			XMMATRIX tm;
			XMMATRIX m;

			for (CompIt it = comp_it_begin(box_id);
				 it.has_next;
				 comp_it_next(it))
			{
				BoxCollider& box = *(BoxCollider*)it.comp;

				if (!has_entity_component(it.entity, body_id))
					continue;
					
				tm = get_entity_world_matrix(it.entity);

				m = XMMatrixScalingFromVector(vec3_to_dx(box.size)) * tm;
		    
				v0 = XMVector3Transform(p0, m);
				v1 = XMVector3Transform(p1, m);
				v2 = XMVector3Transform(p2, m);
				v3 = XMVector3Transform(p3, m);
				v4 = XMVector3Transform(p4, m);
				v5 = XMVector3Transform(p5, m);
				v6 = XMVector3Transform(p6, m);
				v7 = XMVector3Transform(p7, m);

				imrend_draw_line(v3_f32(v0), v3_f32(v1), Color::Green(), cmd);
				imrend_draw_line(v3_f32(v1), v3_f32(v3), Color::Green(), cmd);
				imrend_draw_line(v3_f32(v3), v3_f32(v2), Color::Green(), cmd);
				imrend_draw_line(v3_f32(v0), v3_f32(v2), Color::Green(), cmd);

				imrend_draw_line(v3_f32(v4), v3_f32(v5), Color::Green(), cmd);
				imrend_draw_line(v3_f32(v5), v3_f32(v7), Color::Green(), cmd);
				imrend_draw_line(v3_f32(v7), v3_f32(v6), Color::Green(), cmd);
				imrend_draw_line(v3_f32(v4), v3_f32(v6), Color::Green(), cmd);

				imrend_draw_line(v3_f32(v0), v3_f32(v4), Color::Green(), cmd);
				imrend_draw_line(v3_f32(v1), v3_f32(v5), Color::Green(), cmd);
				imrend_draw_line(v3_f32(v2), v3_f32(v6), Color::Green(), cmd);
				imrend_draw_line(v3_f32(v3), v3_f32(v7), Color::Green(), cmd);
			}
		}

		// Draw gizmos
		switch (editor.tool_data.tool_type) {

		case EditorToolType_Gizmos:
			draw_gizmos(renderer->gfx.offscreen, cmd);
			break;

		}

		XMMATRIX vpm = XMMatrixIdentity();

		if (dev.debug_draw)
			vpm = dev.camera.view_projection_matrix;
		else {

			CameraComponent* cam = get_main_camera();
		
			if (cam) {

				vpm = cam->view_projection_matrix;
			}
		}

		imrend_flush(cmd);
    }

    void _editor_draw()
    {
		CommandList cmd = graphics_commandlist_get();

		switch (dev.engine_state) {

		case EngineState_Edit:
		case EngineState_Play:
			draw_edit_state(cmd);
			break;
    
		}

		{
			GPUImage* off = renderer_offscreen();
			const GPUImageInfo& info = graphics_image_info(off);
			
			GPUImageBlit blit;
			blit.src_region.offset0 = { 0, (i32)info.height, 0 };
			blit.src_region.offset1 = { (i32)info.width, 0, 1 };
			blit.dst_region.offset0 = { 0, 0, 0 };
			blit.dst_region.offset1 = { (i32)info.width, (i32)info.height, 1 };
			
			graphics_image_blit(off, editor.offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource, 1u, &blit, SamplerFilter_Nearest, cmd);
		}

		// Draw gui
		if (dev.debug_draw)
			_gui_draw(cmd);
    }

	List<Entity>& editor_selected_entities()
	{
		return editor.selected_entities;
	}

}

#endif
