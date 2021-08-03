Remote Rendered VR
---

This is an experimental prototype remote renderer for VR intended for testing the applicability of remote VR technologies in any network of choice. 

I wrote the code for this project on my own with the sole purpose of enabling the testing of hypotheses for various papers during my Ph.D.
Therefore, implementations are usually just at the bare minimum for them to work for the test in question, and the code is not super well organized. I have tried to tidy things up a bit before publishing here, but be aware that the code was not initially intended for anyone else to see.

### Requirements

Windows 10
Visual Studio 2015 (Newer might work with some tinkering)
CUDA Toolkit 11.1

### Usage

1. Build the client and server using Visual Studio.
2. Run Server_release.exe where ever you want and forward the ports if testing across the internet (currently UDP 17112 and TCP 17113).
3. Set the IP the client should connect to in netConfig_Client.txt (e.g. "string IP 127.0.0.1")
4. Run the Client_release.exe with parameters of interest set in netConfig_Client.txt and set server parameters in netConfig_Server.txt (e.g. "double H265 1" enables H265 encoding instead of H264 and so on).
5. After the test, Client/content/timers1.txt will be created and filled with test data, this file can be viewed for example using Excel, see examples in e.g. Data/LAN.xlsx.

Note that there is no reconnect/restart implemented yet, so a new test requires a restart of both programs. 
Personally I have just created a batch script that restarts the server on close and run the client manually with various settings depending on the test.

This is the bare minimum to get started, but I will expand this section in the future if there should be some interest in this software.

### Publications

This software was developed for and used in the following publications:

Synchronous Remote Rendering for VR, (2021)
https://www.hindawi.com/journals/ijcgt/2021/6676644/

Remapping of Hidden Area Mesh Pixels for Codec Speed-up in Remote VR, (2021)
https://ieeexplore.ieee.org/document/9465408

Bitrate Requirements of Non-Panoramic VR Remote Rendering, (2020)
https://dl.acm.org/doi/10.1145/3394171.3413681


### TODO

These are the most obvious limitations I am currently aware of:

MSAA in D3D11 			-	MSAA is not yet implemented in D3D11, only in OpenGL.
OpenGL World scene		-	The "World-scene", i.e. cube-map of a mountain-view is only implemented in D3D11 so far, not OpenGL.
D3D11 encoder crash		-	There is some low level bug in the Nvidia decoder when using D3D11 that can cause random crashes when using D3D11.
D3D11 COM object leaks	-	Some COM objects are currently leaking in D3D11.
Restarting				-	Server stops when test is complete and must be restarted manually or e.g. by batch script.
Reconnecting			-	If the server is not running when the client starts, the connection will fail, so the server must be started first.
Hand controllers		-	The input data of hand controllers are never sent to the server and rendering of hand controllers is not yet implemented.

