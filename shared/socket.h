#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

class WindowsSocket {
public:
  WindowsSocket();
  ~WindowsSocket();
  
  // connection/disconnection functions
  BOOL connect();
  BOOL bind(const UINT16 port);
  BOOL close();
  
  // receive/transmit functions
  
private:
  WSADATA wsadata;
  SOCKET socket_handle = INVALID_SOCKET;
};

