
#include "socket.h"

#define _IPHOST wIpHost

#define SIO_KEEPALIVE_VALS IOC_IN | IOC_VENDOR | 4
//---------------------------------------------------------------------------
LException::LException(const TCHAR* info)
{
	m_info = info ? _tcsdup(info) : NULL;
	//if(m_info)	OutputDebugString(m_info);
}
/*
LException::LException(const char *info)
{
	//m_info = info ? _tcsdup(info) : NULL;
}
*/
//===============================================================
LException::~LException()
{
	FREE(m_info);
}

typedef struct TCPBASE_KEEPALIVE
{
	ULONG onoff;
	ULONG keepalivetime;
	ULONG keepaliveinterval;
}TCPBASE_KEEPALIVE, * PTCPBASE_KEEPALIVE;
//---------------------------------------------------------------------------
LTcpBase::LTcpBase()
{
	mRecvTimeOut = DEFAULT_TIMEOUT;
	mSendTimeOut = DEFAULT_TIMEOUT;
	mSock = NULL;
}
//---------------------------------------------------------------------------
LTcpBase::LTcpBase(SOCKET sock)
{
	mRecvTimeOut = DEFAULT_TIMEOUT;
	mSendTimeOut = DEFAULT_TIMEOUT;
	mSock = sock;
}
//---------------------------------------------------------------------------
LTcpBase::~LTcpBase()
{
	shutdown(mSock, SD_BOTH);
	CloseSocket();
}
//---------------------------------------------------------------------------
BOOL LTcpBase::InitWSAStartup()
{
	WSADATA WsaData;

	if (0 == WSAStartup(MAKEWORD(2, 2), &WsaData))
	{
		if (LOBYTE(WsaData.wVersion) != 2 ||
			HIBYTE(WsaData.wVersion) != 2)
		{
			mErrCode = WSAGetLastError();
			WSACleanup();
			return FALSE;
		}
		return TRUE;
	}
	else
		mErrCode = WSAGetLastError();
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL LTcpBase::SetTimeOut(DWORD _RecvTimeOut, DWORD _SendTimeOut)
{
	mRecvTimeOut = _RecvTimeOut;
	mSendTimeOut = _SendTimeOut;

	//if(LOBYTE(winsockVersion) < 2)
	//	return VFalse;
	int sendtimeout = 1000 * _SendTimeOut;
	int revctimeout = 1000 * _RecvTimeOut;



	if (setsockopt(mSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&revctimeout, sizeof(revctimeout)) == SOCKET_ERROR)
	{
		return FALSE;
	}

	if (setsockopt(mSock, SOL_SOCKET, SO_SNDTIMEO, (char*)&sendtimeout, sizeof(sendtimeout)) == SOCKET_ERROR)
	{
		return FALSE;
	}


	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LTcpBase::SetKeepAlive()
{
	int iKeepAlive = -1;
	int iOptLen = sizeof(iKeepAlive);
	if (getsockopt(mSock, SOL_SOCKET, SO_KEEPALIVE, (char*)&iKeepAlive, &iOptLen) == SOCKET_ERROR)
		return FALSE;

	iKeepAlive = 1;
	if (setsockopt(mSock, SOL_SOCKET, SO_KEEPALIVE, (char*)&iKeepAlive, iOptLen) == SOCKET_ERROR)
		return FALSE;

	iKeepAlive = -1;
	if (getsockopt(mSock, SOL_SOCKET, SO_KEEPALIVE, (char*)&iKeepAlive, &iOptLen) == SOCKET_ERROR)
		return FALSE;

	if (iKeepAlive == 1)
	{
		TCPBASE_KEEPALIVE inKeepAlive = { 0 };
		unsigned long ulInLen = sizeof(TCPBASE_KEEPALIVE);
		TCPBASE_KEEPALIVE outKeepAlive = { 0 };
		unsigned long ulOutLen = sizeof(TCPBASE_KEEPALIVE);
		unsigned long ulBytesReturn = 0;
		//設至socket的keep alive為2秒，並且發送次數為三
		inKeepAlive.onoff = 1;
		inKeepAlive.keepaliveinterval = 3000;
		inKeepAlive.keepalivetime = 3;
		//為選定的SOCKET設置Keep Alive，成功后SOCKET可過Keep Alive自動檢測連接是否斷開
		if (WSAIoctl(mSock, SIO_KEEPALIVE_VALS, (LPVOID)&inKeepAlive, ulInLen,
			(LPVOID)&outKeepAlive, ulOutLen, &ulBytesReturn, NULL, NULL) == SOCKET_ERROR)
		{
			return FALSE;//cout << "WSAIoctl failed. error code = " << WSAGetLastError() << endl;
		}
		return TRUE;
	}
	return FALSE;
}
void LTcpBase::SetBlocking(bool bBlock)
{
	int blocking = 1;
	if (bBlock) blocking = 0;
	ioctlsocket(mSock, FIONBIO, (unsigned long*)&blocking);

}
void LTcpBase::SetSocketOpt()
{
	//case 1 :
	int flag = 1;
	/* add 097.9.24 */
	SetBlocking(true);
	setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));

	SetKeepAlive();
	SetBlocking(true);

	/*************/

	// case 2 :

	BOOL sopt;
	sopt = TRUE;

	setsockopt(mSock, IPPROTO_TCP, TCP_NODELAY, (char*)&sopt, sizeof(BOOL));
	sopt = TRUE;
	setsockopt(mSock, SOL_SOCKET, SO_DONTLINGER, (char*)&sopt, sizeof(BOOL));

}
//---------------------------------------------------------------------------
BOOL LTcpBase::BindSocket(int IpPort, int MaxLink)
{
	mSock = socket(AF_INET, SOCK_STREAM, 0);//AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mSock == INVALID_SOCKET)
	{
		mErrCode = WSAGetLastError();
		return FALSE;
	}
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_family = PF_INET;
	address.sin_port = htons(IpPort);

	SetSocketOpt();

	if (bind(mSock, (struct sockaddr*)&address, sizeof(address)) != 0)
	{
		mErrCode = WSAGetLastError();
		return FALSE;
	}
	if (listen(mSock, MaxLink))
	{
		mErrCode = WSAGetLastError();
		return FALSE;
	}
	return TRUE;

}
//---------------------------------------------------------------------------
/*
 需改成 要的動作
*/
LTcpBase* LTcpBase::AcceptSocket()
{
	sockaddr_in addr;
	SOCKET sock;
	int nLen;
	nLen = sizeof(addr);
	sock = accept(mSock, (sockaddr*)&addr, &nLen);
	if (sock != INVALID_SOCKET)
	{
		LTcpBase* pSock = new LTcpBase(sock);
		if (!pSock)
		{
			closesocket(sock);
			return NULL;
		}
		else
		{
			return pSock;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------
BOOL LTcpBase::ConnectSocket(TCHAR* _IPHOST, int IpPort)
{
	mSock = socket(AF_INET, SOCK_STREAM, 0);//AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mSock == INVALID_SOCKET)
	{
		mErrCode = WSAGetLastError();
		return FALSE;
	}
#ifdef _UNICODE
	size_t size = wcstombs(0, _IPHOST, 0);
	char* IpHost = (char*)malloc(size + 1);
	ZeroMemory(IpHost, size + 1);
	wcstombs(IpHost, _IPHOST, size);
#else
	char* IpHost = _IPHOST;
#endif

	DWORD  dwIp = inet_addr(IpHost);

	if (dwIp == INADDR_NONE)
	{
		struct hostent* hostPtr = gethostbyname(IpHost);
		if (hostPtr && hostPtr->h_addr_list[0])
		{
			struct in_addr addr;
			memmove(&addr, hostPtr->h_addr_list[0], 4);
			char* ip = inet_ntoa(addr);
			dwIp = inet_addr(ip);

			if (dwIp == INADDR_NONE)    return FALSE;
		}
	}

	SOCKADDR_IN addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");///  inet_addr(IpHost);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(IpPort);

	SetSocketOpt();

#ifdef _UNICODE
	free(IpHost);
#endif	
	if (connect(mSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		mErrCode = WSAGetLastError();
		//gLog.DbgLog(L"Connect Socket FAILED,error code:%d ", mErrCode);
		return FALSE;
	}
	//gLog.DbgLog(L"Connect Socket Success ");
	return TRUE;
}
//---------------------------------------------------------------------------
int LTcpBase::TcpSelect(BOOL bRead)
{
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(mSock, &fd);
	timeval tm = { 0, 0 };
	if (bRead)
	{
		tm.tv_sec = mRecvTimeOut;
		return select(0, &fd, NULL, NULL, &tm);
	}
	else
	{
		tm.tv_sec = mSendTimeOut;
		return select(0, NULL, &fd, NULL, &tm);
	}
}
//---------------------------------------------------------------------------
int LTcpBase::TcpSelectWithTimeout(BOOL bRead, int iTOSecs)
{
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(mSock, &fd);

	timeval tm = { iTOSecs, 500 };
	if (bRead)
	{
		//        tm.tv_sec = mRecvTimeOut;
		return select(0, &fd, NULL, NULL, &tm);
	}
	else
	{
		//        tm.tv_sec = mSendTimeOut;
		return select(0, NULL, &fd, NULL, &tm);
	}
}
//---------------------------------------------------------------------------

BOOL LTcpBase::SocketConnected()
{
	if (mSock <= 0) return 0;


	//TODO - make this better, because its probably wrong (but seems to work)

	int optval;
	int optlen = sizeof(optval);

	int res = getsockopt(mSock, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen);
	TCHAR szoptval[5], szres[5];
	_itow(optval, szoptval, 10);
	_itow(res, szres, 10);
	OutputDebugString(szoptval);
	OutputDebugString(szres);
	if (optval == 0 && res == 0)
		return true;
	else
		return false;

}
//---------------------------------------------------------------------------
DWORD LTcpBase::GetSocketError()
{
	return mErrCode;
}
//---------------------------------------------------------------------------
BOOL LTcpBase::GetSockPeer(OUT TCHAR* IpHost, OUT UINT& IpPort)
{
	sockaddr_in addr;
	char* tmp;
	int len;
	len = 16 * sizeof(addr);
	if (getpeername(mSock, (sockaddr*)&addr, &len) == SOCKET_ERROR)
	{
		mErrCode = WSAGetLastError();
		return FALSE;
	}
	else
	{
		tmp = inet_ntoa(addr.sin_addr);
#ifdef _UNICODE
		size_t leng = strlen(tmp);
		size_t wlen = MultiByteToWideChar(CP_ACP, 0, (const char*)tmp, int(leng), NULL, 0);
		MultiByteToWideChar(CP_ACP, 0, (const char*)tmp, int(leng), IpHost, int(wlen));
#else
		strcpy(IpHost, tmp);
#endif
		//IpHost = inet_ntoa(addr.sin_addr);

		IpPort = ntohs(addr.sin_port);
		return TRUE;
	}
}
//---------------------------------------------------------------------------
BOOL LTcpBase::GetSockPeer(OUT DWORD& IpHost, OUT  UINT& IpPort)
{
	sockaddr_in addr;
	int len;
	len = 16 * sizeof(addr);
	if (getpeername(mSock, (sockaddr*)&addr, &len) == SOCKET_ERROR)
	{
		mErrCode = WSAGetLastError();
		//gLog.DbgLog(L"get peer name,error code:%d", mErrCode);
		return FALSE;
	}
	else
	{
		IpHost = addr.sin_addr.s_addr;
		IpPort = ntohs(addr.sin_port);
		return TRUE;
	}
}
//---------------------------------------------------------------------------
void LTcpBase::CloseSocket()
{
	if (mSock == NULL)return;
	closesocket(mSock);
	mSock = NULL;
}
//---------------------------------------------------------------------------
void LTcpBase::ReadBytes(std::string _buf, int NeedLen)
{
	char Data[100];

	int getlen = 0;
	//u_long val = 1;
	do
	{
		//Data = Data + getlen;
		//ioctlsocket(mSock, FIONBIO, &val);
		getlen = recv(mSock, Data, 100, 0);
		if (getlen < 1) // )//  ==SOCKET_ERROR//假設零時為一方斷線(097.9.23 test 待證實)
		{
			throw LException(_T("ReadBytes recv packages failed."));
		}
		else if (getlen > 0) {
			NeedLen = NeedLen - getlen;
		}
		else {
			OutputDebugString(_T("------------break"));
			break;
		}
	} while (NeedLen);
}

#define SEND_SUCCESS 0

int LTcpBase::Write(char* Buff, size_t Len)
{
	char* Data = (char*)Buff;
	int RetMsg;
	while (TRUE)
	{
		RetMsg = send(mSock, Data, Len, 0);
		if (RetMsg == SOCKET_ERROR)
		{
			mErrCode = WSAGetLastError();
			return SOCKET_ERROR;
		}
		if (RetMsg == 0)return 1; //這裡的意義..忘了
		if (RetMsg == (int)Len)break;
		Data += RetMsg;
		Len -= RetMsg;
	}
	return 0;
}