#pragma once

#include <SilverEngine>

class GameState : public SV::State {

	SV::Scene scene;

public:
	void Load() override
	{
		scene.Initialize();
	}
	void Initialize() override 
	{
		
	}
	void Update(float dt) override
	{
	}
	void Render() override
	{
	}
	void Unload() override
	{
	}
	void Close() override
	{

	}

};