#pragma once

class Engine
{
public:

	Engine(int argc, char* argv[]);
	Engine();
	~Engine();
	virtual bool Init() = 0;
	virtual void Shutdown() = 0;
	virtual void GameLoop() = 0;

};
