#pragma once

#include <SDL.h>

#include <GL/glew.h>
#include <glm.hpp>

class GLWindow
{
public:

	GLWindow(void);
	~GLWindow(void);

	int Render(glm::vec2& _pos, const glm::vec3& _color);
	void SetTitle(const char* _title);
	int	 Run();
	void BeginRender(float scale, GLenum mode = GL_LINES);
	void EndRender();
	void GetGridRenderCoords(glm::vec2& _pos);
	bool InitWindow(int _width, int _height, int _res);
	bool InitSDL();
	bool InitGL();
	//SDL_Window* GetSDLWindow() const { return m_SDLWindow; }
	void Clear();
	void MakeCurrent();
	void GLERROR();
private:

	//HWND					g_hWndMain = NULL;

	GLuint m_DistanceSB;
	GLuint m_JumpPointSB;
	GLuint m_MapTexture;
	bool InitWnd();

	GLuint m_instanceIDBuffer;
	//GLuint renderHandle;
	GLuint m_texHandle;
	
	int m_width, m_height, m_res;

	int* m_pickingTexZeros;
	SDL_Event *m_mainEvent = NULL;
	SDL_Window* m_SDLWindow;
	SDL_GLContext m_glcontext;
};