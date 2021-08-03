#pragma once

#define _WINSOCKAPI_

#include "EncoderBase.h"
#include "../NetworkEngine.h"

//#include "../NvEncoderCuda.h"
//#include "../NvEncoderD3D11.h"
//#include "../Utils/NvCodecUtils.h"

//#include "../Utils/NvEncoderCLIOptions.h"

#include <cuviddec.h>
#include "../DecodedFrame.h"

#include <d3d11.h>
#include <cuda.h>

class EncoderD3D : public EncoderBase
{
public:

	void InitEncoder(NetworkEngine* _net,unsigned char _eye, int _resolutionBaseLevel, ID3D11Device* _device, ID3D11DeviceContext* _deviceContext);

	void StartEncoding(long long _t_begin_render, long long _t_begin_encode, long long _t_input, ID3D11Texture2D* _texture);
	void SpecificProcess();

	void CopyTexture();

	void CudaLock();
	void CudaUnlock();

	EncoderD3D();
	~EncoderD3D();

private:
	static void ThreadStart(EncoderD3D* _self);

	std::vector<std::vector<uint8_t>> m_packets;

	ID3D11Device* m_device;
	ID3D11Texture2D* m_sceneTexture;
	ID3D11DeviceContext* m_deviceContext;

	static std::mutex m_mutex;
};