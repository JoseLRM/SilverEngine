#include "core/particles.h"
#include "core/renderer.h"

namespace sv {

	void ParticleSystem::serialize(Serializer& s)
	{
		
	}
	
	void ParticleSystem::deserialize(Deserializer& d, u32 version)
	{
		
	}

	ParticleEmitter::~ParticleEmitter()
	{
		if (particles) {
			SV_FREE_MEMORY(particles);
			particles = NULL;
		}
	}

	SV_AUX void emit_particle(ParticleEmitter& e, v3_f32 system_position)
	{
		Particle& p = e.particles[e.particle_count++];

		switch (e.shape.type) {

		case ParticleShapeType_Point:
		{
			p.position = {};
		}
		break;

		case ParticleShapeType_Cube:
		{
			// TODO: Fill
			p.position.x = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.cube.size.x;
			p.position.y = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.cube.size.y; 
			p.position.z = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.cube.size.z;
		}
		break;

		case ParticleShapeType_Plane:
		{
			p.position.x = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.plane.size.x;
			p.position.y = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.plane.size.y;
			p.position.z = 0.f;
		}
		break;

		case ParticleShapeType_Sphere:
		{
			p.position.x = math_random_f32(e.seed++, -0.5f, 0.5f);
			p.position.y = math_random_f32(e.seed++, -0.5f, 0.5f);
			p.position.z = math_random_f32(e.seed++, -0.5f, 0.5f);

			if (e.shape.fill)
				p.position = vec3_normalize(p.position);
			
			p.position *= e.shape.sphere.radius;
		}
		break;
			
		}

		p.position += system_position;

		p.velocity.x = math_random_f32(e.seed++, e.min_velocity.x, e.max_velocity.x);
		p.velocity.y = math_random_f32(e.seed++, e.min_velocity.y, e.max_velocity.y);
		p.velocity.z = math_random_f32(e.seed++, e.min_velocity.z, e.max_velocity.z);

		p.size = math_random_f32(e.seed++, e.min_size, e.max_size);

		f32 lifetime = math_random_f32(e.seed++, e.min_lifetime, e.max_lifetime);
		p.time_mult = 1.f / lifetime;
		p.time_count = 0.f;
	}

	SV_INTERNAL void update_and_draw_particles()
	{
		XMMATRIX rm = dev.camera.inverse_view_matrix;
		{
			XMFLOAT3X3 m;
			XMStoreFloat3x3(&m, rm);
			rm = XMLoadFloat3x3(&m);
		}

		f32 dt = engine.deltatime;
		SceneData& scene = *get_scene_data();
		CommandList cmd = graphics_commandlist_get();

		imrend_begin_batch(cmd);

		// TODO
		imrend_camera(ImRendCamera_Editor, cmd);
			
		CompID ps_id = get_component_id("Particle System");

		for (CompIt it = comp_it_begin(ps_id);
			 it.has_next;
			 comp_it_next(it))
		{
			ParticleSystem& ps = *(ParticleSystem*)it.comp;
			Entity entity = it.entity;
			
			if (ps.emitter_count > PARTICLE_EMITTER_MAX) {
				SV_LOG_ERROR("The particle system can't have more than '%u' particle emitters", PARTICLE_EMITTER_MAX);
				ps.emitter_count = PARTICLE_EMITTER_MAX;
			}

			// TODO: Free unused emitters memory

			bool emit;
			bool reset = false;

			ps.time_count += dt;

			if (ps.time_count < ps.simulation_time) {
				emit = true;
			}
			else {
				emit = false;

				f32 total_time = ps.simulation_time + ps.repeat_time;

				// Reset
				if (ps.repeat && ps.time_count > total_time) {
						
					ps.time_count -= total_time;
					reset = true;
				}
			}

			foreach(emitter_index, ps.emitter_count) {

				ParticleEmitter& e = ps.emitters[emitter_index];

				// Allocate particles memory
					
				if (e._last_max_particles != e.max_particles) {

					if (e.particles) {
						SV_FREE_MEMORY(e.particles);
					}
					e.particles = (Particle*)SV_ALLOCATE_MEMORY(e.max_particles * sizeof(Particle));
					e._last_max_particles = e.max_particles;

					// TODO: Save particle state
					e.particle_count = 0u;
				}

				// Update, erase and draw particles

				GPUImage* image = e.texture.get();
					
				for (u32 i = 0u; i < e.particle_count;) {

					Particle& p = e.particles[i];

					p.time_count += dt * p.time_mult;

					// Erase
						
					if (p.time_count >= 1.f) {

						if (i != e.particle_count - 1u) {

							// TODO: Optimize								
							memcpy(e.particles + i, e.particles + i + 1u, sizeof(Particle) * (e.particle_count - i - 1u));
						}
							
						--e.particle_count;
					}

					// Update and draw
						
					else {

						p.velocity += scene.physics.gravity * dt * e.gravity_mult;
						p.position += p.velocity * dt;

						Color color = color_interpolate(e.init_color, e.final_color, p.time_count);

						imrend_push_matrix(rm * XMMatrixTranslation(p.position.x, p.position.y, p.position.z), cmd);
						imrend_draw_sprite({}, { p.size, p.size }, color, image, GPUImageLayout_ShaderResource, e.texcoord, cmd);
						imrend_pop_matrix(cmd);
							
						++i;
					}
				}

				// Reset
				if (reset) {

					e.emission.count = 0.f;
					e.emission.spawn_count = 0.f;
					emit = true;
				}

				// Emit
				if (emit && e.particle_count < e.max_particles) {

					v3_f32 system_position = get_entity_world_position(entity);

					e.emission.count += dt;

					if (e.emission.count > e.emission.offset_time) {

						f32 end_time = e.emission.offset_time + e.emission.spawn_time;

						if (e.emission.count < end_time) {

							e.emission.spawn_count += dt;
								
							f32 frq = 1.f / e.emission.rate;

							while (e.emission.spawn_count > frq) {
							
								emit_particle(e, system_position);

								e.emission.spawn_count -= frq;

								if (e.particle_count == e.max_particles)
									break;
							}
						}
					}
				}
			}
			
			
			imrend_flush(cmd);
		}
	}

#if SV_EDITOR

	void display_particle_system_data(DisplayComponentEvent* event)
	{
		if (event->comp_id == get_component_id("Particle System")) {
			
			ParticleSystem& p = *(ParticleSystem*)event->comp;

			static u32 show_emitter_index = 0u;

			gui_drag_f32("Simulation Time", p.simulation_time, 0.01f, 0.f, f32_max);
			if (p.repeat)
				gui_drag_f32("Repeat Time", p.repeat_time, 0.01f, 0.f, f32_max);
			gui_checkbox("Repeat", p.repeat);
			gui_drag_u32("Emitter Count", p.emitter_count, 1u, 0u, PARTICLE_EMITTER_MAX);

			if (p.emitter_count) {
				
				gui_drag_u32("Show Emitter", show_emitter_index, 1u, 0u, p.emitter_count - 1u);

				gui_separator(1);

				ParticleEmitter& e = p.emitters[show_emitter_index];

				egui_comp_texture("Texture", 69u, &e.texture);
				gui_drag_v4_f32("Texcoord", e.texcoord, 0.001f, 0.f, 1.f);

				gui_drag_u32("Max Particles", e.max_particles, 1u, 0u, 100000u);
				gui_drag_v3_f32("Min Velocity", e.min_velocity, 0.01f, -f32_max, f32_max);
				gui_drag_v3_f32("Max Velocity", e.max_velocity, 0.01f, -f32_max, f32_max);
				gui_drag_f32("Min Lifetime", e.min_lifetime, 0.01f, 0.f, f32_max);
				gui_drag_f32("Max Lifetime", e.max_lifetime, 0.01f, 0.f, f32_max);
				gui_drag_f32("Min Size", e.min_size, 0.01f, 0.f, f32_max);
				gui_drag_f32("Max Size", e.max_size, 0.01f, 0.f, f32_max);
				gui_drag_color("Init color", e.init_color);
				gui_drag_color("Final color", e.final_color);
				gui_drag_f32("Gravity Mult", e.gravity_mult, 0.001f, -f32_max, f32_max);

				gui_separator(1);

				const char* preview = "";
				switch (e.shape.type) {
					
				case ParticleShapeType_Point:
					preview = "Point";
					break;
					
				case ParticleShapeType_Cube:
					preview = "Cube";
					break;
					
				case ParticleShapeType_Sphere:
					preview = "Sphere";
					break;

				case ParticleShapeType_Plane:
					preview = "Plane";
					break;
					
				}

				if (gui_begin_combobox(preview, 345345)) {

					if (e.shape.type != ParticleShapeType_Point && gui_button("Point")) {
						e.shape.type = ParticleShapeType_Point;
					}

					if (e.shape.type != ParticleShapeType_Sphere && gui_button("Sphere")) {
						e.shape.type = ParticleShapeType_Sphere;
						e.shape.sphere.radius = 1.f;
					}

					if (e.shape.type != ParticleShapeType_Cube && gui_button("Cube")) {
						e.shape.type = ParticleShapeType_Cube;
						e.shape.cube.size.x = 1.f;
						e.shape.cube.size.y = 1.f;
						e.shape.cube.size.z = 1.f;
					}
					
					if (e.shape.type != ParticleShapeType_Plane && gui_button("Plane")) {
						e.shape.type = ParticleShapeType_Plane;
						e.shape.plane.size.x = 1.f;
						e.shape.plane.size.y = 1.f;
					}
					
					gui_end_combobox();
				}
				
				

				gui_drag_f32("Emission Rate", e.emission.rate, 0.1f, 0.f, f32_max);
				gui_drag_f32("Emission Offset Time", e.emission.offset_time, 0.01f, 0.f, f32_max);
				gui_drag_f32("Emission Spawn Time", e.emission.spawn_time, 0.01f, 0.f, f32_max);
			}
		}
	}
	
#endif

	void _particle_initialize()
	{
		event_register("update_and_draw_particles", update_and_draw_particles, 0);

#if SV_EDITOR
		event_register("display_component_data", display_particle_system_data, 0);
#endif
	}
	
	void _particle_close()
	{
		
	}
	
}
