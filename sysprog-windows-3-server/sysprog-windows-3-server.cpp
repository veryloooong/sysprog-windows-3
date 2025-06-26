#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <WinSock2.h>
#include <tchar.h>
#include <Shlwapi.h>

#define BUFFER_SIZE 2048

void print_err(TCHAR const* where) {
  ::_tprintf(TEXT("%s failed with error %d\n"), where, ::GetLastError());
}

void print_err_wsa(TCHAR const* where) {
  ::_tprintf(TEXT("%s failed with error %d\n"), where, ::WSAGetLastError());
}

struct ChatUser {
  SOCKET socket_handle = INVALID_SOCKET; // Socket handle for the user
  TCHAR username[BUFFER_SIZE]; // Username of the user
  BOOL is_connected = FALSE; // Connection status of the user
  ChatUser() {
    ZeroMemory(username, sizeof(username));
  }
  ~ChatUser() {
    if (socket_handle != INVALID_SOCKET) {
      if (::closesocket(socket_handle) == SOCKET_ERROR) {
        print_err_wsa(TEXT("closesocket"));
      }
      socket_handle = INVALID_SOCKET;
    }
  }
};

class ChatServer {
public:
  // Default constructor initializes the server on specified port and allows a specified number of users
  ChatServer(const UINT16 port, const UINT user_count = 10) {
    this->max_user_count = user_count;
    users = new ChatUser[user_count]; // Allocate memory for user array
    if (users == nullptr) {
      print_err(TEXT("new ChatUser[]"));
      return;
    }

    if (WSAStartup(WINSOCK_VERSION, &wsadata) != 0) {
      print_err_wsa(TEXT("WSAStartup"));
      return;
    }


    server_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
      print_err_wsa(TEXT("socket"));
      return;
    }

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(server_socket, reinterpret_cast<SOCKADDR*>(&sockaddr), sizeof(sockaddr)) == SOCKET_ERROR) {
      print_err_wsa(TEXT("bind"));
      ::closesocket(server_socket);
      return;
    }
  }

  ~ChatServer() {
    if (users != nullptr) {
      delete[] users; // Free allocated memory for users
      users = nullptr;
    }
    if (server_socket != INVALID_SOCKET) {
      if (::closesocket(server_socket) == SOCKET_ERROR) {
        print_err_wsa(TEXT("closesocket"));
      }
      server_socket = INVALID_SOCKET;
    }

    if (::WSACleanup() == SOCKET_ERROR) {
      print_err_wsa(TEXT("WSACleanup"));
    }
  }

  // Check if the server is initialized successfully
  BOOL is_initialized() const {
    return server_socket != INVALID_SOCKET;
  }

  // Accept incoming connections and handle messages
  BOOL accept() {
    // Listen for incoming connections
    int listen_result = ::listen(server_socket, SOMAXCONN);
    if (listen_result == SOCKET_ERROR) {
      print_err_wsa(TEXT("listen"));
      return FALSE;
    }
    FD_ZERO(&all_sockets);
    FD_SET(server_socket, &all_sockets);
    timeval timeout = { 2, 0 };
    
    // Main loop to accept connections and process messages
    while (true) {
      available_sockets = all_sockets;
      // Wait for activity on the sockets
      int socket_count = ::select(0, &available_sockets, nullptr, nullptr, &timeout);

      if (socket_count == SOCKET_ERROR) {
        print_err_wsa(TEXT("select"));
        return FALSE;
      }
      else if (socket_count == 0) {
        continue;
      }
      else {
        // check for incoming connections
        if (FD_ISSET(server_socket, &available_sockets)) {
          SOCKADDR_IN client_addr;
          int addr_len = sizeof(client_addr);
          SOCKET client_socket = ::accept(server_socket, reinterpret_cast<SOCKADDR*>(&client_addr), &addr_len);
          if (client_socket == INVALID_SOCKET) {
            print_err_wsa(TEXT("accept"));
            return FALSE;
          }
          
          // add new client to the user list
          if (user_count < max_user_count) {
            users[user_count].socket_handle = client_socket;
            users[user_count].is_connected = TRUE;
            ::_stprintf_s(users[user_count].username, TEXT("User%d"), user_count + 1);
            user_count++;
            ::_tprintf(TEXT("New client connected: %s\n"), users[user_count - 1].username);
          }
          else {
            ::_tprintf(TEXT("Maximum user limit reached. Closing new connection.\n"));
            ::closesocket(client_socket);
          }
          FD_SET(client_socket, &all_sockets);
        }

        // check for data from existing clients
        for (int i = 0; i < socket_count; i++) {
          SOCKET current_socket = available_sockets.fd_array[i];
          if (current_socket == server_socket) continue;

          // find the user associated with the current socket
          ChatUser* current_user = nullptr;
          for (UINT j = 0; j < user_count; j++) {
            if (users[j].socket_handle == current_socket) {
              current_user = &users[j];
              break;
            }
          }

          if (current_user == nullptr || !current_user->is_connected) {
            ::_tprintf(TEXT("Client disconnected or not found.\n"));
            FD_CLR(current_socket, &all_sockets);
            users[i].is_connected = FALSE; 
            users[i].socket_handle = INVALID_SOCKET; 
            ::_tprintf(TEXT("Closing socket for user %s.\n"), current_user ? current_user->username : TEXT("Unknown"));
            FD_CLR(current_socket, &all_sockets);
            user_count--; 
            ::closesocket(current_socket);
            continue;
          }

          // Receive data from the client
          TCHAR buffer[BUFFER_SIZE] = { 0 }; // Buffer to store received message  
          if (receive_message_from_client(current_socket, buffer, BUFFER_SIZE)) {
            // Print the received message
            ::_tprintf(TEXT("[%s] %s\n"), current_user->username, buffer);
            // Here you can add logic to broadcast the message to other users if needed
            if (!send_message_to_all_clients(current_user, buffer)) {
              ::_tprintf(TEXT("Failed to send message to all clients.\n"));
              print_err(TEXT("send_message_to_all_clients"));
            }
          }
          else {
            // If receiving failed, mark the user as disconnected
            current_user->is_connected = FALSE;
            ::_tprintf(TEXT("Client %s disconnected.\n"), current_user->username);
            FD_CLR(current_socket, &all_sockets);
            ::closesocket(current_socket);
          }
        }
      }
    }
  }

private:
  SOCKET server_socket = INVALID_SOCKET;
  SOCKADDR_IN sockaddr;
  WSADATA wsadata;
  UINT user_count = 0;
  UINT max_user_count = 10; // Default user count, can be adjusted as needed
  ChatUser* users = nullptr; // Array of connected users, can be dynamically allocated if needed

  fd_set all_sockets;
  fd_set available_sockets;

  BOOL receive_message_from_client(SOCKET client_socket, TCHAR* buffer, int buffer_size) {
    char recv_buffer[BUFFER_SIZE] = { 0 }; // Buffer to store received message
    int bytes_received = ::recv(client_socket, recv_buffer, sizeof(recv_buffer) - 1, 0);
    if (bytes_received == SOCKET_ERROR) {
      return FALSE;
    }
    else if (bytes_received == 0) {
      return FALSE;
    }
    recv_buffer[bytes_received] = '\0'; // Null-terminate the received string
    // Convert received message to TCHAR
    ::MultiByteToWideChar(CP_UTF8, 0, recv_buffer, -1, buffer, buffer_size);
    return TRUE;
  }

  BOOL send_message_to_all_clients(ChatUser* user, const TCHAR* message) {
    if (user->socket_handle == INVALID_SOCKET || !user->is_connected) {
      print_err(TEXT("send_message_to_all_clients"));
      return FALSE;
    }
    if (message == nullptr || *message == '\0') {
      print_err(TEXT("send_message_to_all_clients"));
      return FALSE;
    }
    // Prepare the message to send
    TCHAR message_with_username[BUFFER_SIZE] = { 0 };
    ::_stprintf_s(message_with_username, TEXT("[%s]: %s"), user->username, message);

    char send_buffer[BUFFER_SIZE] = { 0 };
    // message format is "[username]: message" so need to cat the username
    ::WideCharToMultiByte(CP_UTF8, 0, message_with_username, -1, send_buffer, sizeof(send_buffer) - 1, NULL, NULL);
    // Check if the message is too long
    if (strlen(send_buffer) >= BUFFER_SIZE) {
      ::_tprintf(TEXT("Message too long to send: %s\n"), message_with_username);
      return FALSE;
    }

    // Send the message to all connected users
    for (UINT i = 0; i < user_count; i++) {
      if (users[i].is_connected && users[i].socket_handle != INVALID_SOCKET && users[i].socket_handle != user->socket_handle) {
        int bytes_sent = ::send(users[i].socket_handle, send_buffer, static_cast<int>(strlen(send_buffer)), 0);
        if (bytes_sent == SOCKET_ERROR) {
          print_err_wsa(TEXT("send"));
          return FALSE;
        }
      }
    }
    return TRUE;
  }
};

int _tmain(int argc, TCHAR *argv[]) {
  if (argc < 2) {
    ::_tprintf(TEXT("Usage: %s <PORT> [<MAXCONNS>]"), argv[0]);
    return 1;
  }
  int __convert = StrToInt(argv[1]);
  if (__convert > MAXUINT16 || __convert <= 0) {
    ::_tprintf(TEXT("Invalid port number"));
    return 1;
  }
  int max_connections = 10; // Default max connections
  if (argc >= 3) {
    max_connections = StrToInt(argv[2]);
    if (max_connections <= 0 || max_connections > 100) { // Arbitrary limit for max connections
      ::_tprintf(TEXT("Invalid max connections, must be between 1 and 100"));
      return 1;
    }
  }

  UINT16 port = __convert;
  ChatServer server(port, max_connections);
  if (!server.is_initialized()) {
    ::_tprintf(TEXT("Failed to initialize server on port %d"), port);
    return 1;
  }

  ::_tprintf(TEXT("Server initialized on port %d\n"), port);

  if (!server.accept()) {
    ::_tprintf(TEXT("Failed to accept connections on port %d"), port);
    return 1;
  }

  return 0;
}
