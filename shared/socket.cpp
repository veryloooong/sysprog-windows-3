#include "socket.h"
#include "shared.h"

WindowsSocket::WindowsSocket() {
  if (::WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
    print_err_wsa(TEXT("WSAStartup"));
  }
  
  socket_handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_handle == INVALID_SOCKET) {
    print_err_wsa(TEXT("socket"));
    close();
  }
}

BOOL WindowsSocket::connect() {
  
}

BOOL WindowsSocket::bind(const UINT16 port) {

}

BOOL WindowsSocket::close() {
  
}

WindowsSocket::~WindowsSocket() {
  if (close() == FALSE) {
    print_err(TEXT("WindowsSocket::close"));
  }

  if (::WSACleanup() != 0) {
    print_err_wsa(TEXT("WSACleanup"));
  }
}