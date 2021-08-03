#pragma once

#include <cuda.h>
#include <nvEncodeAPI.h>
#include <vector>

struct cudaGraphicsResource;
class NvEncoder;
struct ID3D11Texture2D;
struct ID3D11Device;

class EncWrapper
{
public:
	EncWrapper();
	~EncWrapper();

	//GL

	int CreateWithGL(CUcontext _context, unsigned int _width, unsigned int _height, int _bitrate, bool _isH265);
	void EncodeWithGL(CUcontext _context, std::vector<std::vector<unsigned char>>& _packets);
	void CopyToCuda(void* devPtr, size_t _size, cudaGraphicsResource* _ptr);


	//D3D

	int CreateWithD3D(ID3D11Device* _device, int _width, int _height, int _bitrate, bool _isH265);
	void EncodeWithD3D(std::vector<std::vector<unsigned char>>& _packets);
	ID3D11Texture2D* GetFrameD3D();

	//Common

	void Reconfigure(unsigned int _width, unsigned int _height, int _bitrate);


private:

	NvEncoder* m_encoder;
	NV_ENC_INITIALIZE_PARAMS m_initializeParams;
	NV_ENC_CONFIG m_encodeConfig;

	CUdeviceptr m_cudaMemory;

};

