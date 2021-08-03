#pragma once

class PNG
{
public:
	 static int WritePNG(const char* _path, int _w, int _h, int _channels, unsigned char* _data);
	 static unsigned char* ReadPNG(const char* _path, int& _w, int& _h, int& _channels, int req_comp);
private:

};