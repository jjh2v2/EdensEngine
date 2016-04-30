#include "EdenEngine.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	EdenEngine *engine = new EdenEngine();

	engine->Run();

	delete engine;
	engine = NULL;

	return 0;
}