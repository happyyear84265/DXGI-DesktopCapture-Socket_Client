#include "stdafx.h"
#include "capture.hpp"
#include "wcomm.h"

#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" ) 
using namespace std;

void runclient(char* ip)
{
	WComm w;
	char rec[32] = "";

	// Connect To Server
	w.connectServer(ip, 8787);

	printf("Connected to server...\n");

	// Sending File
	char sed[50] = "FileSend";
	w.sendData(sed);
	w.recvData(rec, 32);
	w.fileSend("ScreenShot0.jpg");
	printf("File Sent.............\n");

	char sed1[50] = "EndConnection";
	// Send Close Connection Signal
	w.sendData(sed1);
	w.recvData(rec, 32);
	printf("Connection ended......\n");
	w.closeConnection();
}

int main(int argc, char* argv[])
{
	if (argc == 1) return 0;

    HRESULT return_bk = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(return_bk)) return 0;
    MFStartup(MF_VERSION);
    DESKTOPCAPTUREPARAMS dp;
    DesktopCapture_ONE(dp);
	runclient(argv[1]);
	return 0;
}


