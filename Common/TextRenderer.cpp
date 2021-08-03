#include "TextRenderer.h"

#include "PNG.h"

static const int TEXT_SIZE = 22;
static const int TEXT_BORDER = 12;

TextRenderer& TextRenderer::GetInstance()
{
	static TextRenderer instance = TextRenderer();
	return instance;
}

TextRenderer::TextRenderer()
{

}

TextRenderer::~TextRenderer()
{
}

void TextRenderer::Destroy()
{
	glDeleteBuffers(2, m_2DVBO);
	DeleteShaders();
	for (std::unordered_map<unsigned int, C_RenderString*>::iterator it = m_renderStrings.begin(); it != m_renderStrings.end(); it++)
	{
		delete it->second;
	}
	glDeleteTextures(1,&m_alphabetTexture);
}

void TextRenderer::DeleteShaders()
{
	delete m_TextShader;
}

void TextRenderer::LoadText(int _w, int _h)
{
	if (m_letters64.size() > 0)
		return;

	m_width = _w;
	m_height = _h;

	CreateShaders();

	m_isLargeChar['M'] = true;
	m_isLargeChar['Q'] = true;
	m_isLargeChar['W'] = true;
	m_isLargeChar['m'] = true;
	m_isLargeChar['O'] = true;
	m_isLargeChar['w'] = true;
	m_isLargeChar['@'] = true;
	m_isLargeChar['%'] = true;
	m_isLargeChar['&'] = true;
	m_isLargeChar['G'] = true;

	m_isLargeChar['i'] = false;
	m_isLargeChar['l'] = false;
	m_isLargeChar['!'] = false;
	m_isLargeChar['.'] = false;
	m_isLargeChar[','] = false;

	int nImageWidth, nImageHeight, channels;

	unsigned char* imageRGBA = PNG::ReadPNG("../content/textures/alphabet.png", nImageWidth, nImageHeight, channels, 3);

	if (imageRGBA == 0)
	{
		printf("Alphabet load error! %s\n", "../content/textures/alphabet.png");
		return;
	}

	GLuint textureId = 0;

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &m_alphabetTexture);
	glBindTexture(GL_TEXTURE_2D, m_alphabetTexture);

	// Copy file to OpenGL
	//glActiveTexture(GL_TEXTURE0); error = glGetError();
	glGenTextures(1, &m_alphabetTexture);
	glBindTexture(GL_TEXTURE_2D, m_alphabetTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, channels, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nImageWidth, nImageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageRGBA[0]);

	free(imageRGBA);

	//m_TextInfo.Height = h;
	//m_TextInfo.Width = w;
	//m_TextInfo.X = 0;
	//m_TextInfo.Y = 0;

	float positionData[] = {
		-1.0, -1.0,
		-1.0, 1.0,
		1.0, -1.0,
		-1.0, 1.0,
		1.0, 1.0,
		1.0, -1.0 };

	float texCoordData[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f };

	int nrOfPoints = 12;

	glGenBuffers(2, m_2DVBO);

	// "Bind" (switch focus to) first buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_2DVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, nrOfPoints * sizeof(float), positionData, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_2DVBO[1]);
	glBufferData(GL_ARRAY_BUFFER, nrOfPoints * sizeof(float), texCoordData, GL_STATIC_DRAW);

	// create 1 VAO
	glGenVertexArrays(1, &m_2DVAO);
	glBindVertexArray(m_2DVAO);

	// enable "vertex attribute arrays"
	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // texCoord

								  // map index 0 to position buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_2DVBO[0]);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte *)NULL);

	glBindBuffer(GL_ARRAY_BUFFER, m_2DVBO[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte *)NULL);

	glBindVertexArray(0); // disable VAO
	glUseProgram(0); // disable shader program
	GLenum error = glGetError();

	if (error != GL_NO_ERROR)
	{
		printf("Error loading text %d\n", error);
	}
}

void TextRenderer::CreateShaders()
{
	m_TextShader = new ShaderHandler();

	m_TextShader->CreateShaderProgram();
	m_TextShader->AddShader("../content/shaders/vsText.glsl", GL_VERTEX_SHADER);
	m_TextShader->AddShader("../content/shaders/fsText.glsl", GL_FRAGMENT_SHADER);
	m_TextShader->LinkShaderProgram();

	m_TextShader->AddUniform("gAlphabetSize");
	m_TextShader->AddUniform("gLetter");
	m_TextShader->AddUniform("gPos");
	m_TextShader->AddUniform("gScale");
	m_TextShader->AddUniform("gTexture");
	m_TextShader->AddUniform("gViewProj");

	m_TextShader->UseProgram();

	m_TextShader->SetUniformV_loc("gAlphabetSize", 92.0f);

	m_BackgroundShader = new ShaderHandler();

	m_BackgroundShader->CreateShaderProgram();
	m_BackgroundShader->AddShader("../content/shaders/vsTextBackground.glsl", GL_VERTEX_SHADER);
	m_BackgroundShader->AddShader("../content/shaders/fsTextBackground.glsl", GL_FRAGMENT_SHADER);
	m_BackgroundShader->LinkShaderProgram();

	m_BackgroundShader->AddUniform("gPos");
	m_BackgroundShader->AddUniform("gScale");
	m_BackgroundShader->AddUniform("gColor");
	m_BackgroundShader->AddUniform("gViewProj");

	glUseProgram(0);
}

void TextRenderer::Bind2DQuad()
{
	glBindVertexArray(m_2DVAO);
}

void TextRenderer::RenderText(unsigned int _w, unsigned int _h, float deltaTime, unsigned char _worldSpace, float _globalTextScale, const float* _viewProj, bool _background)
{
	ShaderHandler* shader = m_TextShader;

	if (_background)
		shader = m_BackgroundShader;

	shader->UseProgram();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);

	glm::mat4 viewproj = glm::mat4(0);

	memcpy(&viewproj, _viewProj, 16 * 4);

	if (!_background)
		shader->SetUniformV_loc("gTexture", 0);

	shader->SetUniformV_loc("gViewProj", viewproj);

	glBindVertexArray(m_2DVAO);
	glBindTexture(GL_TEXTURE_2D, m_alphabetTexture);

	int currentItem = 0;

	for (std::unordered_map<unsigned int, C_RenderString*>::iterator it = m_renderStrings.begin(); it != m_renderStrings.end(); it++)
	{
		if (!it->second->Visible)
			continue;

		bool isPasswordString = 0;

		if (it->second->Text_ptr != "")
		{
			it->second->Text_char = it->second->Text_ptr.c_str();
		}

		it->second->Pos.x += it->second->Speed.x*deltaTime;
		it->second->Pos.y += it->second->Speed.y*deltaTime;

		float textScale = it->second->Scale * _globalTextScale;

		if (it->second->Selected)
			textScale *= 1.25f;

		float pushWidth = (((TEXT_SIZE - TEXT_BORDER) * textScale) / _w);
		float pushHeight = (((TEXT_SIZE) * textScale) / _h);
		float width = (((TEXT_SIZE)* textScale) / _w);
		float lowerCaseOffset = ((1.0f * textScale) / _w);
		float height = ((TEXT_SIZE * textScale) / _h);

		glm::vec3 pos = it->second->Pos;

		bool newLine = 1;

		int i = -1;
		float startX = pos.x;

		if (!isPasswordString) //Cut prefix from render
		{
			if (it->second->Text_char[0] == '$')
				i += 3;
		}

		while (newLine)
		{
			newLine = 0;
			pos.x = startX;

			if (it->second->Alignment != C_RenderString::Align::LEFT)
			{
				int index = i + 1;
				float push = 0;

				while (true)
				{
					char curChar = it->second->Text_char[index++];

					if (curChar == '\0' || curChar == '\n')
						break;

					if (index > 0)
					{
						char prevChar = it->second->Text_char[index - 1];

						if (m_isLargeChar.find(curChar) != m_isLargeChar.end())
						{
							if (m_isLargeChar[curChar] == true)
								push += (((TEXT_SIZE)* textScale * 0.1f) / _w);
							else
								push -= (((TEXT_SIZE)* textScale * 0.1f) / _w);
						}
						if (m_isLargeChar.find(prevChar) != m_isLargeChar.end())
						{
							if (m_isLargeChar[prevChar] == true)
								push += (((TEXT_SIZE)* textScale * 0.1f) / _w);
							else
								push -= (((TEXT_SIZE)* textScale * 0.1f) / _w);
						}
					}

					push += (pushWidth - (curChar >= 97) * lowerCaseOffset);
				}

				if (it->second->Alignment == C_RenderString::Align::CENTER)
				{
					pos.x -= push * 0.5f;
				}

				if (it->second->Alignment == C_RenderString::Align::RIGHT)
				{
					pos.x -= push;
				}
			}

			while (true)
			{
				i++;

				char curChar = it->second->Text_char[i];

				if (curChar == '\n')
				{
					newLine = 1;
				}

				if (curChar == '\t')
					continue;

				if (curChar == '\0' || i < 0 || newLine)
					break;

				if (i > 0)
				{
					char prevChar = it->second->Text_char[i - 1];

					if (m_isLargeChar.find(curChar) != m_isLargeChar.end())
					{
						if (m_isLargeChar[curChar] == true)
							pos.x += (((TEXT_SIZE)* textScale * 0.1f) / _w);
						else
							pos.x -= (((TEXT_SIZE)* textScale * 0.1f) / _w);
					}
					if (m_isLargeChar.find(prevChar) != m_isLargeChar.end())
					{
						if (m_isLargeChar[prevChar] == true)
							pos.x += (((TEXT_SIZE)* textScale * 0.1f) / _w);
						else
							pos.x -= (((TEXT_SIZE)* textScale * 0.1f) / _w);
					}
				}

				glm::vec3 color = it->second->Color;

				if (it->second->Selected)
					color = glm::vec3(1.0f, 1.0f, 0.0f);
				if (it->second->Clicked)
					color = glm::vec3(0.0f, 1.0f, 0.0f);

				if (!_background)
				{
					m_letterBuffer[currentItem] = curChar - 32; //not entire alphabet availalbe -32
				}
				else
					m_colorBuffer[currentItem] = color;

				m_posBuffer[currentItem] = 2.0f * pos - 1.0f;
				m_scaleBuffer[currentItem] = glm::vec2(width, height);


				pos.x += (pushWidth - (curChar >= 97) * lowerCaseOffset);

				currentItem++;

				if (currentItem >= MAX_RENDERED_TEXT)
				{
					DrawTextCall(currentItem, _background);
					currentItem = 0;
				}
			}

			pos.y -= (pushHeight);
		}
	}

	if (currentItem > 0)
	{
		DrawTextCall(currentItem, _background);
		currentItem = 0;
	}

	//glBindVertexArray(0);
	glUseProgram(0);

	glEnable(GL_DEPTH_TEST);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		printf("Error rendering text %d\n", error);
	}
}

bool TextRenderer::IsVisible(unsigned int _id)
{
	return bool(m_renderStrings[_id]->Visible != 0);
}

void TextRenderer::DeselectGroup(unsigned char _group)
{
	for (std::unordered_map<unsigned int, C_RenderString*>::iterator it = m_renderStrings.begin(); it != m_renderStrings.end(); it++)
	{
		if (it->second->Group == _group)
		{
			it->second->Selected = 0;
			it->second->Clicked = 0;
		}
	}
}

void TextRenderer::SetGroupVisibility(unsigned char _group, bool _visible)
{
	for (std::unordered_map<unsigned int, C_RenderString*>::iterator it = m_renderStrings.begin(); it != m_renderStrings.end(); it++)
	{
		if (it->second->Group == _group)
			it->second->Visible = _visible;
	}
}

unsigned int TextRenderer::AddString(C_RenderString* _string)
{
	_string->ID = m_stringID;
	m_renderStrings[m_stringID] = _string;

	m_stringID++;

	return _string->ID;
}

C_RenderString* TextRenderer::GetString(unsigned int _id)
{
	if (m_renderStrings.find(_id) == m_renderStrings.end())
		return nullptr;

	return m_renderStrings[_id];
}

void TextRenderer::SelectString(unsigned int _id)
{
	for (std::unordered_map<unsigned int, C_RenderString*>::iterator it = m_renderStrings.begin(); it != m_renderStrings.end(); it++)
	{
		it->second->Selected = 0;
	}

	if(m_renderStrings[_id]->Visible)
		m_renderStrings[_id]->Selected = 1;
}

bool TextRenderer::IsClicked(unsigned int _id)
{
	return bool(m_renderStrings[_id]->Clicked != 0);
}

void TextRenderer::ClickString(unsigned int _id, unsigned char _group)
{
	for (std::unordered_map<unsigned int, C_RenderString*>::iterator it = m_renderStrings.begin(); it != m_renderStrings.end(); it++)
	{
		if (it->second->Group == _group)
			it->second->Clicked = 0;
	}

	if (m_renderStrings[_id]->Visible)
		m_renderStrings[_id]->Clicked = 1;
}

void TextRenderer::DeselectAll()
{
	for (std::unordered_map<unsigned int, C_RenderString*>::iterator it = m_renderStrings.begin(); it != m_renderStrings.end(); it++)
	{
		it->second->Clicked = 0;
		it->second->Selected = 0;
	}
}

void TextRenderer::DrawTextCall(int _items, bool _background)
{
	ShaderHandler* shader = m_TextShader;

	if (_background)
		shader = m_BackgroundShader;

	shader->SetUniformV_loc("gPos", _items, &m_posBuffer[0]);
	shader->SetUniformV_loc("gScale", _items, &m_scaleBuffer[0]);

	if(!_background)
		shader->SetUniformV_loc("gLetter", _items, m_letterBuffer);
	else
		shader->SetUniformV_loc("gColor", _items, &m_colorBuffer[0]);

	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, _items);
}