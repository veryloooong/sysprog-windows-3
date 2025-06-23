#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <WinSock2.h>
#include <tchar.h>
#include <Shlwapi.h>

void print_err(TCHAR const* where) {
  ::_tprintf(TEXT("%s failed with error %d\n"), where, ::GetLastError());
}

void print_err_wsa(TCHAR const* where) {
  ::_tprintf(TEXT("%s failed with error %d\n"), where, ::WSAGetLastError());
}

class ChatServer {
public:
  // Default constructor initializes the server on specified port
  ChatServer(const UINT16 port) {
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
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(socket_handle, reinterpret_cast<SOCKADDR*>(&sockaddr), sizeof(sockaddr)) == SOCKET_ERROR) {
      print_err_wsa(TEXT("bind"));
      ::closesocket(socket_handle);
      return;
    }
  }

  ~ChatServer() {
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

  // Check if the server is initialized successfully
  BOOL is_initialized() const {
    return socket_handle != INVALID_SOCKET;
  }

  BOOL accept() {
    int listen_result = ::listen(socket_handle, SOMAXCONN);
    if (listen_result == SOCKET_ERROR) {
      print_err_wsa(TEXT("listen"));
      return FALSE;
    }
    FD_ZERO(&all_sockets);
    FD_SET(socket_handle, &all_sockets);
    timeval timeout = { 2, 0 };
    
    while (true) {
      available_sockets = all_sockets;
      int socket_count = ::select(0, &available_sockets, nullptr, nullptr, &timeout);

      if (socket_count == SOCKET_ERROR) {
        print_err_wsa(TEXT("select"));
        return FALSE;
      }
      else if (socket_count == 0) {
        // Timeout, no activity
        continue;
      }
      else {
        // check for incoming connections
        if (FD_ISSET(socket_handle, &available_sockets)) {
          SOCKADDR_IN client_addr;
          int addr_len = sizeof(client_addr);
          SOCKET client_socket = ::accept(socket_handle, reinterpret_cast<SOCKADDR*>(&client_addr), &addr_len);
          if (client_socket == INVALID_SOCKET) {
            print_err_wsa(TEXT("accept"));
            return FALSE;
          }
          FD_SET(client_socket, &all_sockets);

          // Handle client communication in a separate function or thread
          // For simplicity, we will just close the socket immediately
          if (::closesocket(client_socket) == SOCKET_ERROR) {
            print_err_wsa(TEXT("closesocket (client)"));
          }
        }

        // check for data from existing clients
        for (int i = 0; i < socket_count; i++) {
          SOCKET current_socket = available_sockets.fd_array[i];
          if (current_socket != socket_handle) {
            char buffer[2048];
            int bytes_received = ::recv(current_socket, buffer, sizeof(buffer), 0);
            if (bytes_received > 0) {
              // Process received data
              ::_tprintf(TEXT("Received %d bytes from client\n"), bytes_received);
            }
            else if (bytes_received == 0) {
              // Client disconnected
              ::_tprintf(TEXT("Client disconnected\n"));
              FD_CLR(current_socket, &all_sockets);
              ::closesocket(current_socket);
            }
            else {
              print_err_wsa(TEXT("recv"));
            }
          }
        }
      }
    }
  }

private:
  SOCKET socket_handle = INVALID_SOCKET;
  SOCKADDR_IN sockaddr;
  WSADATA wsadata;

  fd_set all_sockets;
  fd_set available_sockets;
};

int _tmain(int argc, TCHAR *argv[]) {
  if (argc != 2) {
    ::_tprintf(TEXT("Usage: %s <PORT>"), argv[0]);
    return 1;
  }
  int __convert = StrToInt(argv[1]);
  if (__convert > MAXUINT16 || __convert <= 0) {
    ::_tprintf(TEXT("Invalid port number"));
    return 1;
  }

  UINT16 port = __convert;
  ChatServer server(port);
  if (!server.is_initialized()) {
    ::_tprintf(TEXT("Failed to initialize server on port %d"), port);
    return 1;
  }

  if (!server.accept()) {
    ::_tprintf(TEXT("Failed to accept connections on port %d"), port);
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
