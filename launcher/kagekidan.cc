#include "webview/webview.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::wstring exe_dir() {
  wchar_t path[MAX_PATH];
  DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
  return fs::path(std::wstring(path, n)).parent_path().wstring();
}

static bool port_open(int port) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) return false;
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<u_short>(port));
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
  bool ok = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
  closesocket(sock);
  return ok;
}

static int read_port(const std::wstring& dir) {
  std::ifstream file(fs::path(dir) / "launcher.port");
  int port = 0;
  file >> port;
  return port;
}

static int wait_port(const std::wstring& dir) {
  for (int i = 0; i < 80; ++i) {
    int port = read_port(dir);
    if (port > 0 && port_open(port)) return port;
    Sleep(100);
  }
  return 0;
}

static HANDLE start_host(const std::wstring& dir) {
  std::wstring python = dir + L"\\python\\pythonw.exe";
  if (!fs::exists(python)) python = dir + L"\\python\\python.exe";
  std::wstring script = dir + L"\\scripts\\launcher_host.py";
  std::wstring command = L"\"" + python + L"\" \"" + script + L"\" --no-browser";
  std::vector<wchar_t> mutable_command(command.begin(), command.end());
  mutable_command.push_back(L'\0');
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION process{};
  if (!CreateProcessW(nullptr, mutable_command.data(), nullptr, nullptr, FALSE,
                      CREATE_NO_WINDOW, nullptr, dir.c_str(), &startup, &process)) {
    return nullptr;
  }
  CloseHandle(process.hThread);
  return process.hProcess;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  WSADATA wsa{};
  WSAStartup(MAKEWORD(2, 2), &wsa);
  std::wstring dir = exe_dir();
  int port = read_port(dir);
  HANDLE host = nullptr;
  if (!(port > 0 && port_open(port))) {
    host = start_host(dir);
    if (!host) {
      MessageBoxW(nullptr, L"The bundled Python was not found next to Kagekidan.exe.", L"Kagekidan", MB_ICONERROR);
      return 1;
    }
    port = wait_port(dir);
  }
  if (port <= 0) {
    MessageBoxW(nullptr, L"The launcher did not start. See launcher-error.txt.", L"Kagekidan", MB_ICONERROR);
    if (host) TerminateProcess(host, 1);
    return 1;
  }
  std::string url = "http://127.0.0.1:" + std::to_string(port) + "/";
  int code = 0;
  try {
    webview::webview window(false, nullptr);
    window.set_title("Kagekidan");
    window.set_size(1280, 800, WEBVIEW_HINT_NONE);
    window.navigate(url);
    window.run();
  } catch (const std::exception&) {
    MessageBoxW(nullptr, L"WebView2 was not found. Install the Microsoft Edge WebView2 runtime, then start Kagekidan.exe again.", L"Kagekidan", MB_ICONERROR);
    code = 1;
  }
  if (host) TerminateProcess(host, 0);
  WSACleanup();
  return code;
}
