#include "GLWindow.h"
#include <iostream>
#include <stdio.h>
#include <chrono>
//#include <SDL_ttf.h>



GLWindow::GLWindow(void)
{
	m_width = 0;
	m_height = 0;
	m_mainEvent = new SDL_Event();
}

GLWindow::~GLWindow(void)
{
	// Once finished with OpenGL functions, the SDL_GLContext can be deleted.
	SDL_GL_DeleteContext(m_glcontext);
	SDL_DestroyWindow(m_SDLWindow);
	delete m_mainEvent;
}

bool GLWindow::InitWindow(int _width, int _height, int _res)
{
	m_res = _res;
	m_width = _width*m_res;
	m_height = _height*m_res;
	
	return InitWnd();
}

bool GLWindow::InitSDL()
{
	//Initialize SDL 
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		return false;
	}

	return true;
}

bool GLWindow::InitWnd()
{

	// Window mode MUST include SDL_WINDOW_OPENGL for use with OpenGL.
	m_SDLWindow = SDL_CreateWindow("ML Network", 10, 10, m_width + m_res * 2, m_height + m_res * 2,SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

	// Create an OpenGL context associated with the window.
	m_glcontext = SDL_GL_CreateContext(m_SDLWindow);

	MakeCurrent();

	// now you can make GL calls.
	SDL_GL_SetSwapInterval(0);

	return true;
}

void GLWindow::MakeCurrent()
{
	SDL_GL_MakeCurrent(m_SDLWindow, m_glcontext);
}

bool GLWindow::InitGL()
{
	//Initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		printf("mError initializing GLEW! %s\n", glewGetErrorString(glewError));
		return false;
	}

	//Make sure OpenGL 2.1 is supported
	//if (!GLEW_VERSION_4_5)
	//{
	//	printf("mOpenGL 2.1 not supported!\n");
	//	return false;
	//}

	//Set the viewport
	glViewport(0, 0, m_width + m_res * 2, m_height + m_res * 2);

	//Initialize Projection Matrix
	//glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, m_width, m_height, 0.0, 1.0, -1.0);
	//glFrustum();
	//Initialize Modelview Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//Initialize clear color
	glClearColor(.7f, .7f, .7f, 1.f);

	//Enable texturing
	//glEnable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_2D);

	//Set blending
	//glEnable(GL_BLEND);
	//glEnable(GL_DEPTH_TEST);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Check for error
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		printf("mError initializing OpenGL!\n");
		return false;
	}

	return true;
}

int GLWindow::Run()
{
	//SDL_PollEvent(m_mainEvent);
	SDL_GL_SwapWindow(m_SDLWindow);
	GLERROR();

	return m_mainEvent->type;
}

void GLWindow::BeginRender(float scale, GLenum mode)
{
	if (mode == GL_POINTS)
		glPointSize((GLfloat)(m_res*scale));
	else if (mode == GL_LINES)
		glLineWidth((GLfloat)(m_res*scale));
	glBegin(mode);
}

void GLWindow::EndRender()
{
	glEnd();
	GLERROR();
}

void GLWindow::GetGridRenderCoords(glm::vec2& _pos)
{
	_pos.x = ((_pos.x * m_res * 2.0f + m_res) / (float)m_width) - 1.0f;
	_pos.y = ((_pos.y * m_res * 2.0f + m_res) / (float)m_height) - 1.0f;
}

int GLWindow::Render(glm::vec2& _pos, const glm::vec3& _color)
{
	GetGridRenderCoords(_pos);

	glColor4f(_color.r, _color.g, _color.b, 1);
	glVertex2f(_pos.x, _pos.y);

	return 0;
}

void GLWindow::SetTitle(const char* _title)
{
	SDL_SetWindowTitle(m_SDLWindow, _title);
}

void GLWindow::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GLERROR();
}

void GLWindow::GLERROR()
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		printf("Error %d\n", error);
	}
}