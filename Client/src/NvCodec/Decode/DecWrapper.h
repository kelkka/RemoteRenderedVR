#pragma once
#include "cuviddec.h"

class NvDecoder;

class DecWrapper
{
public:
	DecWrapper(CUcontext _context, int _width, int _height, cudaVideoCodec_enum _codec);
	~DecWrapper();

	int Decode(const unsigned char *pData, int nSize, unsigned char ***pppFrame);

	void ColorConversion(unsigned char ** _frame, CUdeviceptr deviceFrame, int nPitch);
private:

	NvDecoder* m_decoder;
};

