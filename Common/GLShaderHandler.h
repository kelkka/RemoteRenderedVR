#ifndef SHADERHANDLER_H
#define SHADERHANDLER_H
#include <GL/glew.h>
#include <string>
#include <vector>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <unordered_map>

class ShaderHandler
{
public:
	ShaderHandler();
	~ShaderHandler();

	void AddUniform(const char* _str);
	void CreateShaderProgram();
	bool AddShader(const char* _path, GLenum _shaderType);
	void LinkShaderProgram();

	void UseProgram();

	GLuint GetProgramHandle() { return m_program; }
	GLuint* GetProgramHandlePointer() { return &m_program; }

	int SetUniformV(const char* variable, float value);
	int SetUniformV(const char* variable, glm::vec2 _value);
	int SetUniformV(const char* variable, glm::ivec2 _value);
	int SetUniformV(const char* variable, glm::vec3 value);
	int SetUniformV(const char* variable, glm::vec4 value);
	int SetUniformV(const char* variable, glm::mat3 value);
	int SetUniformV(const char* variable, glm::mat4 value);
	int SetUniformV(const char* variable, int value);

	void SetUniformV_loc(const char* _str, int _items, const float* _value);
	void SetUniformV_loc(const char* _str, int _items, glm::vec2* _value);
	void SetUniformV_loc(const char* _str, int _items, glm::ivec2* _value);
	void SetUniformV_loc(const char* _str, int _items , glm::vec3* _value);
	void SetUniformV_loc(const char* _str, int _items , glm::vec4* _value);
	void SetUniformV_loc(const char* _str, int _items , glm::mat3* _value);
	void SetUniformV_loc(const char* _str, int _items , glm::mat4* _value);
	void SetUniformMatrix_loc(const char * _str, int _items, const float * _value);
	void SetUniformV_loc(const char* _str, int _items, int* _value);
	void SetUniformV_loc(const char* _str, int _items, unsigned int* _value);


	void SetUniformV_loc(const char* _str, glm::mat4& _value);
	void SetUniformV_loc(const char* _str, glm::vec2& _value);
	void SetUniformV_loc(const char* _str, glm::ivec2& _value);
	void SetUniformV_loc(const char* _str, glm::vec3& _value);
	void SetUniformV_loc(const char* _str, glm::vec4& _value);
	void SetUniformV_loc(const char* _str, glm::mat3& _value);
	void SetUniformV_loc(const char* _str, float _value);
	void SetUniformV_loc(const char* _str, int _value);
	void SetUniformV_loc(const char* _str, unsigned int _value);

private:
	std::vector<GLuint> m_shaders;
	GLenum	m_program;
	std::unordered_map<std::string, GLint> m_uniforms;
	std::string m_path;
};

#endif