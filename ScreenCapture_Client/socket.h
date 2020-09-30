#pragma once

#include <winsock2.h>
#include <atlbase.h>
#include <stdlib.h>
#include <string>
#define DEFAULT_TIMEOUT 30
//=============================================================
#define  FREE(x)     if(x){ free(x) ; }
class LException
{
public:
    LException(const TCHAR* info = NULL);
    virtual ~LException();
    TCHAR* m_info;
};


class LTcpException
{
public:
    LTcpException(DWORD dwErr) { dwErrMsg = dwErr; };
    DWORD dwErrMsg;

};
//---------------------------------------------------------------------------
bool static InitWSA()
{
    WSADATA WsaData;

    if (0 == WSAStartup(MAKEWORD(2, 2), &WsaData))
    {
        if (LOBYTE(WsaData.wVersion) != 2 ||
            HIBYTE(WsaData.wVersion) != 2)
        {
            //mErrCode = WSAGetLastError();
            WSACleanup();
            return FALSE;
        }
        return TRUE;
    }

    return FALSE;
}
//---------------------------------------------------------------------------
bool static ClearWSA()
{
    return (0 == WSACleanup());
}
//=============================================================
class LTcpBase
{
public:
    LTcpBase();
    LTcpBase(SOCKET sock);
    virtual ~LTcpBase();
    BOOL InitWSAStartup();
    BOOL SetTimeOut(DWORD RecvTimeOut, DWORD SendTimeOut);
    BOOL BindSocket(int IpPort, int MaxLink);
    LTcpBase* AcceptSocket();
    int TcpSelect(BOOL bRead);
    int TcpSelectWithTimeout(BOOL bRead, int iTOSecs);
    BOOL ConnectSocket(TCHAR* IpHost, int IpPort);
    DWORD GetSocketError();
    BOOL GetSockPeer(OUT TCHAR* IpHost, OUT UINT& IpPort);
    BOOL GetSockPeer(OUT DWORD& IpHost, OUT UINT& IpPort);
    void CloseSocket();

    int  Write(char* Buff, size_t Len);      //寫入指定長度 Bytes 
    void ReadBytes(std::string _buf, int NeedLen); //讀取指定長度 buffer

    void SetBlocking(bool bBlock);
    unsigned SessionId =0;            // 為Tcp Server 所建立的頻道 的託管Id值,或許可以以dwTrdClientReadID 取代	
    BOOL SocketConnected();
private:
    DWORD mRecvTimeOut;
    DWORD mSendTimeOut;

    void  SetSocketOpt();
    BOOL  SetKeepAlive();
protected:
    SOCKET mSock;
    DWORD mErrCode =0;
};