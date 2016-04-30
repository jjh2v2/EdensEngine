#pragma once
#include "EdenEngineWindow.h"

class EdenEngine
{
public:
	EdenEngine();
	~EdenEngine();
	void Run();

private:
	bool Update(float delta);
	bool Render();

	EdenEngineWindow *mEngineWindow;
};