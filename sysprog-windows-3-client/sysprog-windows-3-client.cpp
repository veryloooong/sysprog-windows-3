#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <tchar.h>
#include <Shlwapi.h>

void print_err(TCHAR const* where) {
  ::_tprintf(TEXT("%s failed with error %d\n"), where, ::GetLastError());
}

void print_err_wsa(TCHAR const* where) {
  ::_tprintf(TEXT("%s failed with error %d\n"), where, ::WSAGetLastError());
}

class ChatClient {
public:
  // Constructor initializes the client and connects to the server
  ChatClient(const TCHAR* server_address, const UINT16 port) {
    if (WSAStartup(WINSOCK_VERSION, &wsadata) != 0) {
      print_err_wsa(TEXT("WSAStartup"));
      return;
    }
    socket_handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_handle == INVALID_SOCKET) {
      print_err_wsa(TEXT("socket"));
      return;
    }
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    if (::InetPton(AF_INET, server_address, &sockaddr.sin_addr) != 1) {
      print_err(TEXT("InetPton"));
      ::closesocket(socket_handle);
      return;
    }
    if (::connect(socket_handle, reinterpret_cast<SOCKADDR*>(&sockaddr), sizeof(sockaddr)) == SOCKET_ERROR) {
      print_err_wsa(TEXT("connect"));
      ::closesocket(socket_handle);
      return;
    }
  }

  ~ChatClient() {
    if (socket_handle != INVALID_SOCKET) {
      if (::closesocket(socket_handle) == SOCKET_ERROR) {
        print_err_wsa(TEXT("closesocket"));
      }
      socket_handle = INVALID_SOCKET;
    }
    if (::WSACleanup() == SOCKET_ERROR) {
      print_err_wsa(TEXT("WSACleanup"));
    }
  }

  BOOL is_connected() const {
    return socket_handle != INVALID_SOCKET;
  }

private:
  SOCKET socket_handle = INVALID_SOCKET;
  SOCKADDR_IN sockaddr;
  WSADATA wsadata;
};

int _tmain(int argc, TCHAR *argv[]) {
  if (argc != 3) {
    _tprintf(TEXT("Usage: %s <server_address> <port>\n"), argv[0]);
    return 1;
  }

  TCHAR* server_address = argv[1];
  int __convert = StrToInt(argv[1]);
  if (__convert > MAXUINT16 || __convert <= 0) {
    ::_tprintf(TEXT("Invalid port number"));
    return 1;
  }

  UINT16 port = __convert;
  ChatClient client(server_address, port);

  if (!client.is_connected()) {
    ::_tprintf(TEXT("Failed to connect to server %s on port %d\n"), server_address, port);
    return 1;
  }

  return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
