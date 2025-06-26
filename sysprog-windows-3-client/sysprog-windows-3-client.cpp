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

#define BUFFER_SIZE 2048

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
    client_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
      print_err_wsa(TEXT("socket"));
      return;
    }
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    if (::InetPton(AF_INET, server_address, &sockaddr.sin_addr) != 1) {
      print_err(TEXT("InetPton"));
      ::closesocket(client_socket);
      return;
    }
    // Attempt to connect to the server
    ::_tprintf(TEXT("Connecting to server %s on port %d...\n"), server_address, port);
    if (::connect(client_socket, reinterpret_cast<SOCKADDR*>(&sockaddr), sizeof(sockaddr)) == SOCKET_ERROR) {
      print_err_wsa(TEXT("connect"));
      ::closesocket(client_socket);
      return;
    }
    ::_tprintf(TEXT("Connected to server %s on port %d\n"), server_address, port);
  }

  ~ChatClient() {
    if (client_socket != INVALID_SOCKET) {
      if (::closesocket(client_socket) == SOCKET_ERROR) {
        print_err_wsa(TEXT("closesocket"));
      }
      client_socket = INVALID_SOCKET;
    }
    if (::WSACleanup() == SOCKET_ERROR) {
      print_err_wsa(TEXT("WSACleanup"));
    }
  }

  BOOL is_connected() const {
    return client_socket != INVALID_SOCKET;
  }

  BOOL start_session() {
    if (!is_connected()) {
      print_err(TEXT("start_session"));
      return FALSE;
    }
    
    // put the receive loop in a separate thread
    ::_tprintf(TEXT("Starting session with server...\n"));

    ::CreateThread(nullptr, 0, [](LPVOID param) -> DWORD {
      ChatClient* client = static_cast<ChatClient*>(param);
      while (client->receive_message()) {
        // Keep receiving messages until disconnected
      }
      return 0;
    }, this, 0, nullptr);

    while (true) {
      TCHAR message[BUFFER_SIZE] = { 0 };
      ::_tprintf(TEXT("Enter message to send (or 'exit' to quit): "));
      ::_fgetts(message, BUFFER_SIZE, stdin);
      // Remove newline character if present
      size_t len = _tcslen(message);
      if (len > 0 && message[len - 1] == '\n') {
        message[len - 1] = '\0';
      }
      if (_tcscmp(message, TEXT("exit")) == 0) {
        break; // Exit the loop if user types 'exit'
      }
      if (!send_message(message)) {
        print_err(TEXT("send_message"));
        return FALSE;
      }
    }
    return TRUE;
  }

  BOOL send_message(const TCHAR* message) {
    if (!is_connected()) {
      print_err(TEXT("send_message: Not connected to server"));
      return FALSE;
    }
     
    char send_buffer[BUFFER_SIZE] = { 0 };
    ::WideCharToMultiByte(CP_UTF8, 0, message, -1, send_buffer, sizeof(send_buffer) - 1, NULL, NULL);
    int bytes_sent = ::send(client_socket, send_buffer, static_cast<int>(strlen(send_buffer)), 0);
    if (bytes_sent == SOCKET_ERROR) {
      print_err_wsa(TEXT("send"));
      return FALSE;
    }
    ::_tprintf(TEXT("Sent message: %s\n"), message);
    return TRUE;
  }

  BOOL receive_message() {
    if (!is_connected()) {
      print_err(TEXT("loop_receive_message"));
      return FALSE;
    }
    char recv_buffer[BUFFER_SIZE] = { 0 };
    int bytes_received = ::recv(client_socket, recv_buffer, sizeof(recv_buffer) - 1, 0);
    if (bytes_received == SOCKET_ERROR) {
      //print_err_wsa(TEXT("recv"));
      return FALSE;
    }
    else if (bytes_received == 0) {
      ::_tprintf(TEXT("Server disconnected.\n"));
      return FALSE;
    }
    recv_buffer[bytes_received] = '\0'; // Null-terminate the received string
    TCHAR message[BUFFER_SIZE] = { 0 };
    ::MultiByteToWideChar(CP_UTF8, 0, recv_buffer, -1, message, BUFFER_SIZE);
    ::_tprintf(TEXT("%s\n"), message);
    return TRUE;
  }

private:
  SOCKET client_socket = INVALID_SOCKET;
  SOCKADDR_IN sockaddr;
  WSADATA wsadata;

  TCHAR username[BUFFER_SIZE];
};

int _tmain(int argc, TCHAR *argv[]) {
  if (argc != 3) {
    _tprintf(TEXT("Usage: %s <server_address> <port>\n"), argv[0]);
    return 1;
  }

  TCHAR* server_address = argv[1];
  int __convert = StrToInt(argv[2]);
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
  
  if (!client.start_session()) {
    ::_tprintf(TEXT("Failed to start session with server %s on port %d\n"), server_address, port);
    return 1;
  }

  return 0;
}
