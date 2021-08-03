#pragma once
#include <d3d11.h>

struct D3DShader
{

public:

	D3DShader();
	~D3DShader();

	static HRESULT CompileShader(const wchar_t* path, const char* type, ID3DBlob** shaderBlob);
	ID3D11VertexShader* VS = nullptr;
	ID3D11PixelShader* PS = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;

private:

};