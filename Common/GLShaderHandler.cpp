#include "GLShaderHandler.h"
#include <fstream>
#include <stdio.h>

//#define GPU_UNIFORM_GET_ERROR ErrorCheck(_str);
#define GPU_UNIFORM_GET_ERROR

static void ErrorCheck(const char* _str)
{
	GLenum err = glGetError(); 
	if (err != GL_NO_ERROR)
		printf("Uniform error: %s - %d\n", _str, err);
}

ShaderHandler::ShaderHandler()
{
}

ShaderHandler::~ShaderHandler()
{
    /* REMOVE PROGRAM */
    glDeleteProgram( m_program );

    /* REMOVE SHADERS */
    for ( size_t i = 0; i < m_shaders.size( ); i++ )
    {
		glDeleteShader( m_shaders[i] );
    }
}

void ShaderHandler::AddUniform(const char* _str)
{
	m_uniforms[_str] = glGetUniformLocation(GetProgramHandle(), _str);
	if (m_uniforms[_str] == -1)
		printf("No uniform value: %s\n", _str);

}

//void ShaderHandler::Setuniform(const char* _str)
//{
//	SetUniformV_loc(m_uniforms[_str], m_shadowViewMatrix);
//}

void ShaderHandler::CreateShaderProgram()
{
    m_program = glCreateProgram();

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::string err = "Error creating shader program: ";
		err += std::to_string(error);
		printf("%s\n",err.c_str());
	}
}

bool ShaderHandler::AddShader(const char* _path, GLenum _shaderType)
{
	m_path = _path;

	FILE *fp;
	fopen_s(&fp, _path, "r");
	char* buffer = 0;
	size_t fileSize = 0;

	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		buffer = (char*)malloc(fileSize+1);
		memset(buffer, 0, fileSize + 1);

		fread_s(buffer, fileSize, 1, fileSize, fp);

		fclose(fp);
    }
    else
	{
		printf("Unable to find shader path: %s!\n", _path);
		return false;
    }

    GLuint shaderID = glCreateShader(_shaderType);
    const GLchar* shaderSource = buffer;

    glShaderSource(shaderID, 1, &shaderSource, NULL);

    glCompileShader(shaderID);

    GLint vShaderCompiled = GL_FALSE;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &vShaderCompiled);

	memset(buffer, 0, fileSize + 1);
	free(buffer);

    if (vShaderCompiled != GL_TRUE)
    {
        int length = 0;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);

        if(length > 0) 
        {
			// create a log of error messages
			char* errorLog = new char[length];
			int written = 0;
			glGetShaderInfoLog(shaderID, length, &written, errorLog);
			printf("%s Shader error log;\n%s\n", _path, errorLog);

			std::string err = errorLog;
			printf("%s\n",err.c_str());

			delete [] errorLog;
        }
		else
		{
			printf("Unable to compile shader %s", _path);
		}

        return false;
    }

    glAttachShader(m_program, shaderID);
    m_shaders.push_back(shaderID);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::string err = "Error compiling shader: ";
		err += std::to_string(error);
		err += " ";
		err += _path;
		printf("%s\n",err.c_str());
	}
	else
		printf("%s OK\n", _path);

    return true;
}

void ShaderHandler::LinkShaderProgram()
{
    glLinkProgram(m_program);
    
    /* ERROR HANDELING */
    GLint success = 0;
    glGetProgramiv( m_program, GL_LINK_STATUS, &success );

    if ( success == GL_FALSE )
    {
        /* GET THE SIZE OF THE ERROR LOG */
        GLint logSize = 0;
        glGetProgramiv( m_program, GL_INFO_LOG_LENGTH, &logSize );

        /* GET THE ERROR LOG*/
        GLchar *errorLog;
        errorLog = new GLchar[logSize];
        glGetProgramInfoLog( m_program, logSize, &logSize, &errorLog[0] );

        /* PRINT THE ERROR LOG */
        for ( int i = 0; i < logSize; i++ )
        {
			printf( "%c", errorLog[i] );
        }
        printf("\n");

        /* DO SOME CLEANING :) */
        delete(errorLog);

        /* REMOVE PROGRAM */
        glDeleteProgram( m_program );

        /* REMOVE SHADERS */
        for (size_t i = 0; i < m_shaders.size( ); i++ )
        {
			glDeleteShader( m_shaders[i] );
        }
        m_shaders.clear( );
    }

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		printf("Error linking shader %d\n", error);
	}
}

void ShaderHandler::UseProgram()
{
    glUseProgram(m_program);
}

int ShaderHandler::SetUniformV(const char* _variable, float _value)
{
	//	Set as current program
	glUseProgram(m_program);

	//	Get pointer for variable
	int location = glGetUniformLocation(m_program, _variable);
	if (location >= 0)
		glUniform1fv(location, 1, &_value);
	else
		return 1;

	return location;
}

int ShaderHandler::SetUniformV(const char* _variable, glm::vec2 _value)
{
	//	Set as current program
	glUseProgram(m_program);

	//	Get pointer for variable
	int location = glGetUniformLocation(m_program, _variable);
	if (location >= 0)
		glUniform2fv(location, 1, &_value[0]);
	else
		return 1;

	return location;
}

int ShaderHandler::SetUniformV(const char* _variable, glm::ivec2 _value)
{
	//	Set as current program
	glUseProgram(m_program);

	//	Get pointer for variable
	int location = glGetUniformLocation(m_program, _variable);
	if (location >= 0)
		glUniform2iv(location, 1, &_value[0]);
	else
		return 1;

	return location;
}


int ShaderHandler::SetUniformV(const char* _variable, glm::vec3 _value)
{
	//	Set as current program
	glUseProgram(m_program);

	//	Get pointer for variable
	int location = glGetUniformLocation(m_program, _variable);
	if (location >= 0)
		glUniform3fv(location, 1, &_value[0]);
	else
		return 1;

	return location;
}

int ShaderHandler::SetUniformV(const char* _variable, glm::vec4 _value)
{
	//	Set as current program
	glUseProgram(m_program);

	//	Get pointer for variable
	int location = glGetUniformLocation(m_program, _variable);
	if (location >= 0)
		glUniform4fv(location, 1, &_value[0]);
	else
		return 1;

	return location;
}

int ShaderHandler::SetUniformV(const char* _variable, glm::mat3 _value)
{
	//	Set as current program
	glUseProgram(m_program);

	//	Get pointer for variable
	int location = glGetUniformLocation(m_program, _variable);
	if (location >= 0)
		glUniformMatrix3fv(location, 1, GL_FALSE, &_value[0][0]);
	else 
		return 1;

	return location;
}

int ShaderHandler::SetUniformV(const char* _variable, glm::mat4 _value)
{
	//	Set as current program
	glUseProgram(m_program);

	//	Get pointer for variable
	int location = glGetUniformLocation(m_program, _variable);
	if (location >= 0)
		glUniformMatrix4fv(location, 1, GL_FALSE, &_value[0][0]);
	else
		return 1;

	return location;
}

int ShaderHandler::SetUniformV(const char* _variable, int _value)
{
	//	Set as current program
	glUseProgram(m_program);

	//	Get pointer for variable
	int location = glGetUniformLocation(m_program, _variable);
	if (location >= 0)
		glUniform1i(location, _value);
	else return 1;

	return location;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//POINTERS

void ShaderHandler::SetUniformV_loc(const char* _str, int _items ,const float* _value)
{
	glUniform1fv(m_uniforms[_str], _items, _value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, int _items, glm::vec2* _value)
{
	glUniform2fv(m_uniforms[_str], _items, (GLfloat*)(void*)_value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, int _items, glm::ivec2* _value)
{
	glUniform2iv(m_uniforms[_str], _items, (GLint*)(void*)_value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, int _items, glm::vec3* _value)
{
	glUniform3fv(m_uniforms[_str], _items, (GLfloat*)(void*)_value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, int _items, glm::vec4* _value)
{
	glUniform4fv(m_uniforms[_str], _items, (GLfloat*)(void*)_value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, int _items, glm::mat3* _value)
{
	glUniformMatrix3fv(m_uniforms[_str], _items, GL_FALSE, (GLfloat*)(void*)_value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, int _items, glm::mat4* _value)
{
	glUniformMatrix4fv(m_uniforms[_str], _items, GL_FALSE, (GLfloat*)(void*)_value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformMatrix_loc(const char* _str, int _items, const float* _value)
{
	glUniformMatrix4fv(m_uniforms[_str], _items, GL_FALSE, _value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, int _items, int* _value)
{
	glUniform1iv(m_uniforms[_str],_items, _value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, int _items, unsigned int* _value)
{
	glUniform1uiv(m_uniforms[_str], _items, _value);
	GPU_UNIFORM_GET_ERROR
}

//SINGLES

void ShaderHandler::SetUniformV_loc(const char* _str, int _value)
{
	glUniform1i(m_uniforms[_str], _value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, unsigned int _value)
{
	glUniform1ui(m_uniforms[_str], _value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, float _value)
{
	glUniform1f(m_uniforms[_str], _value);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, glm::vec2& _value)
{
	glUniform2f(m_uniforms[_str], _value.x, _value.y);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, glm::ivec2& _value)
{
	glUniform2i(m_uniforms[_str], _value.x, _value.y);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, glm::vec3& _value)
{
	glUniform3f(m_uniforms[_str], _value.x, _value.y,_value.z);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, glm::vec4& _value)
{
	glUniform4f(m_uniforms[_str], _value.x, _value.y, _value.z, _value.w);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, glm::mat3& _value)
{
	glUniformMatrix3fv(m_uniforms[_str], 1, GL_FALSE, &_value[0][0]);
	GPU_UNIFORM_GET_ERROR
}

void ShaderHandler::SetUniformV_loc(const char* _str, glm::mat4& _value)
{
	glUniformMatrix4fv(m_uniforms[_str], 1, GL_FALSE, &_value[0][0]);
	GPU_UNIFORM_GET_ERROR
}