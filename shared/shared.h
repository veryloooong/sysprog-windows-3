#pragma once

#include <Windows.h>
#include <tchar.h>

void print_err(TCHAR const* where) {
  ::_tprintf(TEXT("%s failed with error %d"), where, ::GetLastError());
}

void print_err_wsa(TCHAR const* where) {
  ::_tprintf(TEXT("%s failed with error %d"), where, ::WSAGetLastError());
}
