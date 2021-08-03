#include "D3DShader.h"
#include <stdio.h>
#include <d3dcompiler.h>
#include "D3DHelper.h"

D3DShader::D3DShader()
{

}

D3DShader::~D3DShader()
{
	if (VS != nullptr)
		VS->Release();
	if (PS != nullptr)
		PS->Release();
	if (InputLayout != nullptr)
		InputLayout->Release();

	VS = nullptr;
	PS = nullptr;
	InputLayout = nullptr;
}

HRESULT D3DShader::CompileShader(const wchar_t* path, const char* type, ID3DBlob** shaderBlob)
{
	ID3DBlob* shaderErrors;

	HR_CHECK(D3DCompileFromFile(
		path,
		NULL,
		NULL,
		"main",
		type,
		D3DCOMPILE_DEBUG,
		0,
		shaderBlob,
		&shaderErrors
	))

		if (shaderErrors != 0)
		{
			printf("%s\n", (char*)shaderErrors->GetBufferPointer());
			shaderErrors->Release();
			return -1;
		}

	return 0;
}