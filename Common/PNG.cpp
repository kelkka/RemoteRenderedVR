#include "PNG.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int PNG::WritePNG(const char* _path, int _w, int _h, int _channels, unsigned char* _data)
{
	return stbi_write_png(_path, _w, _h, _channels, _data, _w * _channels);
}

unsigned char* PNG::ReadPNG(const char* _path, int& _w, int& _h, int& _channels, int req_comp)
{
	return stbi_load(_path, &_w, &_h, &_channels, req_comp);
}
