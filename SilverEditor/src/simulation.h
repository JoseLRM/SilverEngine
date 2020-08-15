#pragma once

#include "core_editor.h"
#include "scene.h"

namespace sve {

	bool simulation_initialize();
	bool simulation_close();
	void simulation_update(float dt);
	void simulation_render();

	void simulation_run();		// Start/Reset the simulation
	void simulation_continue(); // If it was paused, continue
	void simulation_pause();	// If it was running, pause but not stop the simulation
	void simulation_stop();		// Destroy the simulation

	bool simulation_running();	// Return true if the simulation is running or paused
	bool simulation_paused();	// Return true if the simulation is paused

	// Scene Management

	sv::Scene	simulation_scene_create_default();
	void		simulation_scene_load();
	sv::Scene	simulation_scene_get();

}