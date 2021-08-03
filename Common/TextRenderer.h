#pragma once

#include <GL/glew.h>
#include <unordered_map>
#include <vector>
#include "GLShaderHandler.h"

struct C_RenderString
{
	enum class Align
	{
		LEFT,
		CENTER,
		RIGHT,
	};

	glm::vec3 Color;
	float Scale;

	glm::vec3 Pos;
	glm::vec3 Speed;
	unsigned int ID;

	const char* Text_char;
	std::string Text_ptr;

	unsigned char Visible;
	unsigned char IsWorldSpace;
	Align Alignment;
	unsigned char Group;
	unsigned char Clicked;
	unsigned char Selected;
	bool Clickable;

	C_RenderString(glm::vec3 _color, float _scale, glm::vec3 _pos, unsigned int _id, std::string _text, Align _align, unsigned char _worldSpace, unsigned char _group, glm::vec3 _speed = glm::vec3(0,0,0), bool _clickable = true)
	{
		Color = _color;
		Scale = _scale;

		Pos = _pos;
		Speed = _speed;
		ID = _id;

		Text_ptr = _text;

		Visible = 0;
		IsWorldSpace = _worldSpace;
		Alignment = _align;
		Group = _group;
		Text_char = Text_ptr.c_str();
		Clicked = 0;
		Selected = 0;
		Clickable = _clickable;
	}
};

class TextRenderer
{
public:
	void Destroy();
	void DeleteShaders();
	~TextRenderer();

	static TextRenderer& GetInstance();

	void LoadText(int _w, int _h);

	void CreateShaders();

	void Bind2DQuad();

	void RenderText(unsigned int _w, unsigned int _h, float deltaTime, unsigned char _worldSpace, float _globalTextScale, const float * _viewProj, bool _background);

	bool IsVisible(unsigned int _id);

	void SetGroupVisibility(unsigned char _group, bool _visible);

	unsigned int AddString(C_RenderString * _string);

	C_RenderString* GetString(unsigned int _id);

	void SelectString(unsigned int _id);

	bool IsClicked(unsigned int _id);

	void ClickString(unsigned int _id, unsigned char _group);

	void DeselectAll();


	void DeselectGroup(unsigned char _group);

private:
	void DrawTextCall(int _items, bool _background);


	std::unordered_map<unsigned int, C_RenderString*> m_renderStrings;
	std::unordered_map<std::string, GLuint> m_TextureMap;
	std::vector<GLbyte(*)[64]> m_letters64;
	TextRenderer();
	ShaderHandler* m_TextShader = nullptr;
	ShaderHandler* m_BackgroundShader = nullptr;
	//std::vector<C_RenderString*> m_RenderStrings;
	unsigned int m_currentStringID = 0;
	glm::mat4* m_view = nullptr;
	glm::mat4* m_proj = nullptr;
	int	m_width = 0;
	int m_height = 0;

	static const int MAX_RENDERED_TEXT = 64;
	glm::vec3 m_posBuffer[MAX_RENDERED_TEXT];
	glm::vec2 m_scaleBuffer[MAX_RENDERED_TEXT];
	glm::vec3 m_colorBuffer[MAX_RENDERED_TEXT];
	unsigned int m_letterBuffer[MAX_RENDERED_TEXT];

	unsigned int m_stringID = 0;
	unsigned int m_selectedString = 0;
	GLuint m_alphabetTexture = 0;
	std::unordered_map<char, bool> m_isLargeChar;
	GLuint m_2DVAO = 0;
	GLuint m_2DVBO[2] = { 0 };

};