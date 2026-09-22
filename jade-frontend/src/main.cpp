#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
extern "C" HRESULT WINAPI SetCurrentProcessExplicitAppUserModelID(PCWSTR appID);
#include <filesystem>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>
#include <winhttp.h>
#include "../sdk/JadeView.h"

extern "C" {
int32_t JADEVIEW_CALL JadeView_init(int32_t, const char*, const char*, const char*, const char*, int32_t);
int32_t JADEVIEW_CALL run_message_loop(void);
int32_t JADEVIEW_CALL jadeview_exit(void);
int32_t JADEVIEW_CALL JadeView_unload(void);
uint32_t JADEVIEW_CALL create_webview_window(const char*, uint32_t, const WebViewWindowOptions*, const WebViewSettings*);
int32_t JADEVIEW_CALL register_ipc_handler(const char*, IpcCallback);
int32_t JADEVIEW_CALL set_protocol_service_path(const char*, char*, size_t, int32_t);
int32_t JADEVIEW_CALL set_window_visible(uint32_t, int32_t);
int32_t JADEVIEW_CALL set_window_title(uint32_t, const char*);
int32_t JADEVIEW_CALL minimize_window(uint32_t);
int32_t JADEVIEW_CALL toggle_maximize_window(uint32_t);
int32_t JADEVIEW_CALL close_window(uint32_t);
uint32_t JADEVIEW_CALL jade_on(const char*, IpcCallback);
char* JADEVIEW_CALL jade_text_create(const char*);
}

namespace {
uint32_t g_window = 0;
std::string g_url;
std::filesystem::path g_models_file;
std::filesystem::path g_plugin_patch_file;
std::filesystem::path g_plugin_overrides_file;
std::filesystem::path g_root;
std::filesystem::path g_sessions_root;
std::filesystem::path g_session_archive_file;
std::atomic<uint64_t> g_rpc_id{0};
HANDLE g_backend_job = nullptr;
HANDLE g_backend_process = nullptr;
bool g_backend_owned = false;
std::atomic<bool> g_exit_requested{false};

std::string utf8(const std::filesystem::path& path) {
  auto value = path.u8string();
  return std::string(value.begin(), value.end());
}

std::wstring wide(const std::string& value) {
  if (value.empty()) return {};
  const int length = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  std::wstring result(length, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length);
  return result;
}

bool localPortOpen(unsigned short port) {
  SOCKET socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socketHandle == INVALID_SOCKET) return false;
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
  const bool connected = connect(socketHandle, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
  closesocket(socketHandle);
  return connected;
}

std::wstring quoteCommandArg(const std::filesystem::path& path) {
  return L"\"" + wide(utf8(path)) + L"\"";
}

bool launchBackend(const std::filesystem::path& root) {
  if (localPortOpen(3080)) return true;
  const auto script = root / "start-dsh.ps1";
  const auto repository = root / "deepseek-harness";
  if (!std::filesystem::exists(script) || !std::filesystem::exists(repository / "package.json")) return false;

  g_backend_job = CreateJobObjectW(nullptr, nullptr);
  if (!g_backend_job) return false;
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
  limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (!SetInformationJobObject(g_backend_job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
    CloseHandle(g_backend_job);
    g_backend_job = nullptr;
    return false;
  }

  wchar_t systemDirectory[MAX_PATH]{};
  GetSystemDirectoryW(systemDirectory, MAX_PATH);
  const std::wstring powershell = std::wstring(systemDirectory) + L"\\WindowsPowerShell\\v1.0\\powershell.exe";
  std::wstring command = powershell + L" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "
    + quoteCommandArg(script) + L" -Repository " + quoteCommandArg(repository);
  std::vector<wchar_t> commandLine(command.begin(), command.end());
  commandLine.push_back(L'\0');
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION processInfo{};
  const std::wstring workingDirectory = wide(utf8(root));
  if (!CreateProcessW(powershell.c_str(), commandLine.data(), nullptr, nullptr, FALSE,
      CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, nullptr, workingDirectory.c_str(), &startup, &processInfo)) {
    CloseHandle(g_backend_job);
    g_backend_job = nullptr;
    return false;
  }
  CloseHandle(processInfo.hThread);
  g_backend_process = processInfo.hProcess;
  if (!AssignProcessToJobObject(g_backend_job, g_backend_process)) {
    TerminateProcess(g_backend_process, 1);
    CloseHandle(g_backend_process);
    CloseHandle(g_backend_job);
    g_backend_process = nullptr;
    g_backend_job = nullptr;
    return false;
  }
  g_backend_owned = true;
  for (int attempt = 0; attempt < 180; ++attempt) {
    if (localPortOpen(3080)) return true;
    DWORD exitCode = STILL_ACTIVE;
    if (GetExitCodeProcess(g_backend_process, &exitCode) && exitCode != STILL_ACTIVE) break;
    Sleep(250);
  }
  return localPortOpen(3080);
}

void shutdownBackend() {
  if (!g_backend_owned) return;
  if (g_backend_job) {
    CloseHandle(g_backend_job);
    g_backend_job = nullptr;
  }
  if (g_backend_process) {
    CloseHandle(g_backend_process);
    g_backend_process = nullptr;
  }
  g_backend_owned = false;
}

std::string trim(std::string value);

std::string jsonField(const std::string& body, const char* field) {
  const std::string marker = std::string("\"") + field + "\":\"";
  const size_t start = body.find(marker);
  if (start == std::string::npos) return {};
  const size_t valueStart = start + marker.size();
  std::string value;
  bool escaped = false;
  for (size_t i = valueStart; i < body.size(); ++i) {
    const char ch = body[i];
    if (!escaped && ch == '"') return value;
    if (!escaped && ch == '\\') { escaped = true; continue; }
    if (escaped) {
      if (ch == 'n') value += '\n';
      else if (ch == 'r') value += '\r';
      else if (ch == 't') value += '\t';
      else value += ch;
      escaped = false;
    } else {
      value += ch;
    }
  }
  return {};
}

bool jsonBoolField(const std::string& body, const char* field, bool& value) {
  const std::string marker = std::string("\"") + field + "\"";
  const size_t fieldStart = body.find(marker);
  if (fieldStart == std::string::npos) return false;
  const size_t colon = body.find(':', fieldStart + marker.size());
  if (colon == std::string::npos) return false;
  const std::string tail = trim(body.substr(colon + 1));
  if (tail.rfind("true", 0) == 0) { value = true; return true; }
  if (tail.rfind("false", 0) == 0) { value = false; return true; }
  return false;
}

bool safePluginEntryId(const std::string& value) {
  if (value.empty() || value.size() > 160) return false;
  return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
    return std::isalnum(ch) || ch == ':' || ch == '_' || ch == '-' || ch == '.' || ch == '/';
  });
}

bool protectedPlugin(const std::string& entryId) {
  static const std::vector<std::string> protectedEntries = {
    "include", "include:plugin-inventory", "include:api-gateway", "include:cordis-host-runner",
    "include:web-startup", "include:webserver", "include:web-runtime",
  };
  return std::find(protectedEntries.begin(), protectedEntries.end(), entryId) != protectedEntries.end();
}

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return {};
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

bool replaceTextFile(const std::filesystem::path& path, const std::string& content) {
  const auto temporary = path.string() + ".dsh-gui.tmp";
  {
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!output) return false;
  }
  return MoveFileExW(wide(temporary).c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

std::map<std::string, bool> loadPluginOverrides() {
  std::map<std::string, bool> overrides;
  std::istringstream input(readTextFile(g_plugin_overrides_file));
  std::string line;
  while (std::getline(input, line)) {
    const std::string content = trim(line);
    if (content.empty() || content.front() == '#') continue;
    std::istringstream row(content);
    std::string entryId;
    std::string enabled;
    if (row >> entryId >> enabled && safePluginEntryId(entryId) && (enabled == "0" || enabled == "1")) {
      overrides[entryId] = enabled == "1";
    }
  }
  return overrides;
}

bool writePluginOverrides(const std::map<std::string, bool>& overrides) {
  std::ostringstream data;
  data << "# DSH-GUI managed plugin overrides. This file contains no secrets.\n";
  for (const auto& [entryId, enabled] : overrides) data << entryId << ' ' << (enabled ? '1' : '0') << '\n';
  return replaceTextFile(g_plugin_overrides_file, data.str());
}

bool writePluginPatch(const std::map<std::string, bool>& overrides) {
  constexpr const char* begin = "# >>> DSH-GUI plugin overrides >>>";
  constexpr const char* end = "# <<< DSH-GUI plugin overrides <<<";
  std::string patch = readTextFile(g_plugin_patch_file);
  const size_t beginAt = patch.find(begin);
  if (beginAt != std::string::npos) {
    const size_t endAt = patch.find(end, beginAt);
    if (endAt == std::string::npos) return false;
    size_t eraseEnd = endAt + std::char_traits<char>::length(end);
    if (eraseEnd < patch.size() && patch[eraseEnd] == '\r') ++eraseEnd;
    if (eraseEnd < patch.size() && patch[eraseEnd] == '\n') ++eraseEnd;
    patch.erase(beginAt, eraseEnd - beginAt);
  } else {
    const size_t emptyList = patch.rfind("\n[]");
    if (emptyList != std::string::npos && trim(patch.substr(emptyList + 1)) == "[]") patch.erase(emptyList + 1);
  }
  if (!overrides.empty()) {
    if (!patch.empty() && patch.back() != '\n') patch += '\n';
    patch += begin;
    patch += '\n';
    for (const auto& [entryId, enabled] : overrides) {
      patch += "- id: " + entryId + "\n  disabled: ";
      patch += enabled ? "false\n" : "true\n";
    }
    patch += end;
    patch += '\n';
  }
  bool hasYamlValue = false;
  std::istringstream lines(patch);
  std::string line;
  while (std::getline(lines, line)) {
    const std::string value = trim(line);
    if (!value.empty() && value[0] != '#') { hasYamlValue = true; break; }
  }
  if (!hasYamlValue) patch += "[]\n";
  return replaceTextFile(g_plugin_patch_file, patch);
}

std::string jsonPayload(const std::string& body) {
  const size_t marker = body.find("\"payload\"");
  if (marker == std::string::npos) return "{}";
  const size_t colon = body.find(':', marker);
  if (colon == std::string::npos) return "{}";
  std::string payload = trim(body.substr(colon + 1));
  if (!payload.empty() && payload.back() == '}') payload.pop_back();
  return trim(payload);
}

const char* JADEVIEW_CALL apiRequest(uint32_t, const char* payload) {
  const std::string body = payload ? payload : "";
  const std::string method = jsonField(body, "method");
  static const std::vector<std::string> allowed = {
    "session.list", "session.create", "session.history", "session.models",
    "session.selectModel", "session.prompt", "commands/execute", "pluginInventory/list",
  };
  if (std::find(allowed.begin(), allowed.end(), method) == allowed.end()) {
    return jade_text_create("{\"error\":\"unsupported API method\"}");
  }
  const std::string requestBody = "{\"type\":\"client-request\",\"rpcId\":\"jade-native-" + std::to_string(++g_rpc_id) + "\",\"method\":\"" + method + "\",\"payload\":" + jsonPayload(body) + "}";
  HINTERNET session = WinHttpOpen(L"DSHStudio/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0);
  if (!session) return jade_text_create("{\"error\":\"unable to open local API session\"}");
  HINTERNET connection = WinHttpConnect(session, L"127.0.0.1", 3080, 0);
  HINTERNET request = connection ? WinHttpOpenRequest(connection, L"POST", wide("/api/" + method).c_str(), nullptr, nullptr, nullptr, WINHTTP_FLAG_REFRESH) : nullptr;
  const wchar_t* headers = L"Content-Type: application/json\r\n";
  BOOL sent = request && WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L), const_cast<char*>(requestBody.data()), static_cast<DWORD>(requestBody.size()), static_cast<DWORD>(requestBody.size()), 0);
  BOOL received = sent && WinHttpReceiveResponse(request, nullptr);
  std::string response;
  if (received) {
    DWORD available = 0;
    while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
      std::string chunk(available, '\0');
      DWORD read = 0;
      if (!WinHttpReadData(request, chunk.data(), available, &read) || read == 0) break;
      chunk.resize(read);
      response += chunk;
    }
  }
  if (request) WinHttpCloseHandle(request);
  if (connection) WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);
  if (response.empty()) return jade_text_create("{\"error\":\"local API request failed\"}");
  return jade_text_create(response.c_str());
}

const char* JADEVIEW_CALL windowControl(uint32_t, const char* payload) {
  std::string body = payload ? payload : "";
  if (body.find("minimize") != std::string::npos) minimize_window(g_window);
  else if (body.find("maximize") != std::string::npos) toggle_maximize_window(g_window);
  else if (body.find("close") != std::string::npos) close_window(g_window);
  return jade_text_create("{\"ok\":true}");
}

std::string trim(std::string value) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) return "";
  const auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(begin, end - begin + 1);
}

std::string jsonEscape(const std::string& value) {
  std::string escaped;
  for (const char c : value) {
    if (c == '"' || c == '\\') escaped += '\\';
    if (c == '\n') escaped += "\\n";
    else if (c == '\t') escaped += "\\t";
    else if (c != '\r') escaped += c;
  }
  return escaped;
}

const char* JADEVIEW_CALL clipboardRead(uint32_t, const char*) {
  if (!OpenClipboard(nullptr)) return jade_text_create("{\"ok\":false,\"error\":\"无法读取系统剪贴板\"}");
  HANDLE handle = GetClipboardData(CF_UNICODETEXT);
  if (!handle) {
    CloseClipboard();
    return jade_text_create("{\"ok\":false,\"error\":\"剪贴板中没有文本\"}");
  }
  const auto* source = static_cast<const wchar_t*>(GlobalLock(handle));
  if (!source) {
    CloseClipboard();
    return jade_text_create("{\"ok\":false,\"error\":\"无法读取剪贴板文本\"}");
  }
  const int length = WideCharToMultiByte(CP_UTF8, 0, source, -1, nullptr, 0, nullptr, nullptr);
  std::string value(length > 0 ? length : 0, '\0');
  if (length > 0) WideCharToMultiByte(CP_UTF8, 0, source, -1, value.data(), length, nullptr, nullptr);
  if (!value.empty()) value.pop_back();
  GlobalUnlock(handle);
  CloseClipboard();
  const std::string response = "{\"ok\":true,\"text\":\"" + jsonEscape(value) + "\"}";
  return jade_text_create(response.c_str());
}

bool safeSessionId(const std::string& sessionId);

bool safeApprovalId(const std::string& approvalId) {
  if (approvalId.size() != 36) return false;
  for (size_t i = 0; i < approvalId.size(); ++i) {
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (approvalId[i] != '-') return false;
    } else if (!std::isxdigit(static_cast<unsigned char>(approvalId[i]))) {
      return false;
    }
  }
  return true;
}

std::string pendingApprovalRpcId(const std::string& sessionId, const std::string& approvalId) {
  HINTERNET session = WinHttpOpen(L"DSHStudio/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0);
  if (!session) return {};
  WinHttpSetTimeouts(session, 3000, 3000, 3000, 5000);
  HINTERNET connection = WinHttpConnect(session, L"127.0.0.1", 3080, 0);
  HINTERNET request = connection ? WinHttpOpenRequest(connection, L"GET", L"/api/events.mux", nullptr, nullptr, nullptr, 0) : nullptr;
  const bool opened = request && WinHttpSendRequest(request, nullptr, 0, nullptr, 0, 0, 0)
    && WinHttpReceiveResponse(request, nullptr);
  std::string stream;
  std::string rpcId;
  if (opened) {
    for (int chunkIndex = 0; chunkIndex < 32 && rpcId.empty(); ++chunkIndex) {
      DWORD available = 0;
      if (!WinHttpQueryDataAvailable(request, &available) || available == 0) break;
      std::string chunk(available, '\0');
      DWORD read = 0;
      if (!WinHttpReadData(request, chunk.data(), available, &read) || read == 0) break;
      stream.append(chunk.data(), read);
      size_t cursor = 0;
      while (cursor < stream.size()) {
        const size_t type = stream.find("\"type\":\"approval/requested\"", cursor);
        if (type == std::string::npos) break;
        const size_t begin = stream.rfind("data:", type);
        const size_t end = stream.find("\n\n", type);
        if (begin == std::string::npos || end == std::string::npos) break;
        const std::string frame = stream.substr(begin, end - begin);
        if (jsonField(frame, "sessionId") == sessionId && jsonField(frame, "approvalId") == approvalId) {
          rpcId = jsonField(frame, "rpcId");
          break;
        }
        cursor = end + 2;
      }
      if (stream.size() > 1024 * 1024) break;
    }
  }
  if (request) WinHttpCloseHandle(request);
  if (connection) WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);
  return rpcId;
}

bool postApprovalResponse(const std::string& body) {
  HINTERNET session = WinHttpOpen(L"DSHStudio/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0);
  if (!session) return false;
  WinHttpSetTimeouts(session, 3000, 3000, 3000, 5000);
  HINTERNET connection = WinHttpConnect(session, L"127.0.0.1", 3080, 0);
  HINTERNET request = connection ? WinHttpOpenRequest(connection, L"POST", L"/api/respond", nullptr, nullptr, nullptr, WINHTTP_FLAG_REFRESH) : nullptr;
  const wchar_t* headers = L"Content-Type: application/json\r\n";
  const bool received = request && WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L), const_cast<char*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0)
    && WinHttpReceiveResponse(request, nullptr);
  std::string response;
  if (received) {
    DWORD available = 0;
    while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
      std::string chunk(available, '\0');
      DWORD read = 0;
      if (!WinHttpReadData(request, chunk.data(), available, &read) || read == 0) break;
      response.append(chunk.data(), read);
    }
  }
  if (request) WinHttpCloseHandle(request);
  if (connection) WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);
  return response.find("\"accepted\":true") != std::string::npos;
}

const char* JADEVIEW_CALL approvalAction(uint32_t, const char* payload) {
  const std::string body = payload ? payload : "";
  const std::string sessionId = jsonField(body, "sessionId");
  const std::string approvalId = jsonField(body, "approvalId");
  const std::string outcome = jsonField(body, "outcome");
  if (!safeSessionId(sessionId) || !safeApprovalId(approvalId) || (outcome != "allowed-once" && outcome != "rejected")) {
    return jade_text_create("{\"ok\":false,\"error\":\"无效的授权请求\"}");
  }
  const std::string rpcId = pendingApprovalRpcId(sessionId, approvalId);
  if (rpcId.empty()) return jade_text_create("{\"ok\":false,\"error\":\"授权请求已失效或无法连接 DSH 实时通道\"}");
  const std::string response = "{\"type\":\"client-response\",\"rpcId\":\"" + jsonEscape(rpcId)
    + "\",\"result\":{\"ok\":true,\"value\":{\"sessionId\":\"" + jsonEscape(sessionId)
    + "\",\"approvalId\":\"" + jsonEscape(approvalId) + "\",\"outcome\":\"" + outcome + "\"}}}";
  return jade_text_create(postApprovalResponse(response) ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"授权请求未被 DSH 接受\"}");
}

bool safeSessionId(const std::string& sessionId) {
  if (sessionId.size() != 44 || sessionId.rfind("session-", 0) != 0) return false;
  for (size_t i = 8; i < sessionId.size(); ++i) {
    if (i == 16 || i == 21 || i == 26 || i == 31) {
      if (sessionId[i] != '-') return false;
    } else if (!std::isxdigit(static_cast<unsigned char>(sessionId[i]))) {
      return false;
    }
  }
  return true;
}

std::filesystem::path findSessionDirectory(const std::string& sessionId) {
  if (!safeSessionId(sessionId)) return {};
  std::error_code error;
  if (!std::filesystem::is_directory(g_sessions_root, error) || error) return {};
  std::filesystem::path found;
  for (std::filesystem::directory_iterator it(g_sessions_root, error), end; !error && it != end; it.increment(error)) {
    const auto projectStatus = it->symlink_status(error);
    if (error || !std::filesystem::is_directory(projectStatus) || std::filesystem::is_symlink(projectStatus)) continue;
    const auto candidate = it->path() / sessionId;
    const auto candidateStatus = std::filesystem::symlink_status(candidate, error);
    if (error || !std::filesystem::is_directory(candidateStatus) || std::filesystem::is_symlink(candidateStatus)) { error.clear(); continue; }
    const bool hasTranscript = std::filesystem::is_regular_file(candidate / "session.jsonl", error)
      || std::filesystem::is_regular_file(candidate / "session.jsonl.zstd", error);
    if (error || !hasTranscript) { error.clear(); continue; }
    if (!found.empty()) return {}; // A duplicated session ID is unsafe to operate on.
    found = candidate;
  }
  return error ? std::filesystem::path{} : found;
}

bool waitForPortClosed(unsigned short port) {
  for (int attempt = 0; attempt < 40; ++attempt) {
    if (!localPortOpen(port)) return true;
    Sleep(100);
  }
  return !localPortOpen(port);
}

std::vector<std::string> loadArchivedSessions() {
  std::vector<std::string> ids;
  std::ifstream input(g_session_archive_file);
  std::string line;
  while (std::getline(input, line)) {
    const auto id = trim(line);
    if (safeSessionId(id) && std::find(ids.begin(), ids.end(), id) == ids.end()) ids.push_back(id);
  }
  return ids;
}

bool writeArchivedSessions(const std::vector<std::string>& ids) {
  std::ostringstream output;
  for (const auto& id : ids) output << id << '\n';
  return replaceTextFile(g_session_archive_file, output.str());
}

const char* JADEVIEW_CALL sessionArchive(uint32_t, const char* payload) {
  const std::string body = payload ? payload : "";
  const std::string action = jsonField(body, "action");
  auto archived = loadArchivedSessions();
  if (action == "list") {
    std::ostringstream response;
    response << "{\"ok\":true,\"sessionIds\":[";
    for (size_t i = 0; i < archived.size(); ++i) {
      if (i) response << ',';
      response << "\"" << jsonEscape(archived[i]) << "\"";
    }
    response << "]}";
    return jade_text_create(response.str().c_str());
  }
  const std::string sessionId = jsonField(body, "sessionId");
  bool shouldArchive = false;
  if (action != "set" || !safeSessionId(sessionId) || !jsonBoolField(body, "archived", shouldArchive)) {
    return jade_text_create("{\"ok\":false,\"error\":\"无效的归档请求\"}");
  }
  const auto found = std::find(archived.begin(), archived.end(), sessionId);
  if (shouldArchive && found == archived.end()) archived.push_back(sessionId);
  if (!shouldArchive && found != archived.end()) archived.erase(found);
  return jade_text_create(writeArchivedSessions(archived) ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"无法保存归档状态\"}");
}

const char* JADEVIEW_CALL sessionContextAction(uint32_t, const char* payload) {
  const std::string body = payload ? payload : "";
  const std::string action = jsonField(body, "action");
  const std::string sessionId = jsonField(body, "sessionId");
  const auto sessionDir = findSessionDirectory(sessionId);
  if (sessionDir.empty()) return jade_text_create("{\"ok\":false,\"error\":\"会话不存在或请求无效\"}");

  if (action == "open_folder") {
    const std::string cwd = jsonField(body, "cwd");
    const std::filesystem::path workspace = wide(cwd);
    std::error_code error;
    if (cwd.empty() || !workspace.is_absolute() || !std::filesystem::is_directory(workspace, error) || error) {
      return jade_text_create("{\"ok\":false,\"error\":\"此会话没有可打开的工作区文件夹\"}");
    }
    const HINSTANCE result = ShellExecuteW(nullptr, L"open", workspace.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return jade_text_create(reinterpret_cast<INT_PTR>(result) > 32 ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"无法打开工作区文件夹\"}");
  }

  if (action != "delete") return jade_text_create("{\"ok\":false,\"error\":\"不支持的会话操作\"}");
  if (!g_backend_owned) {
    return jade_text_create("{\"ok\":false,\"error\":\"当前 DSH 后台不是此 GUI 启动，无法安全彻底删除会话\"}");
  }
  shutdownBackend();
  if (!waitForPortClosed(3080)) return jade_text_create("{\"ok\":false,\"error\":\"后台服务未能停止，会话未删除\"}");
  std::error_code error;
  std::filesystem::remove_all(sessionDir, error);
  if (error || std::filesystem::exists(sessionDir)) {
    launchBackend(g_root);
    return jade_text_create("{\"ok\":false,\"error\":\"无法删除会话文件\"}");
  }
  auto archived = loadArchivedSessions();
  archived.erase(std::remove(archived.begin(), archived.end(), sessionId), archived.end());
  writeArchivedSessions(archived);
  if (!launchBackend(g_root)) return jade_text_create("{\"ok\":false,\"error\":\"会话已删除，但后台服务未能重新启动\"}");
  return jade_text_create("{\"ok\":true}");
}

const char* JADEVIEW_CALL pluginToggle(uint32_t, const char* payload) {
  const std::string body = payload ? payload : "";
  const std::string entryId = jsonField(body, "entryId");
  bool enabled = false;
  if (!safePluginEntryId(entryId) || !jsonBoolField(body, "enabled", enabled)) {
    return jade_text_create("{\"ok\":false,\"error\":\"invalid plugin toggle request\"}");
  }
  if (protectedPlugin(entryId)) {
    return jade_text_create("{\"ok\":false,\"error\":\"system core plugins are protected\"}");
  }
  const auto overrides = [&]() {
    auto next = loadPluginOverrides();
    next[entryId] = enabled;
    return next;
  }();
  if (!writePluginPatch(overrides) || !writePluginOverrides(overrides)) {
    return jade_text_create("{\"ok\":false,\"error\":\"unable to update the local DSH plugin profile\"}");
  }
  const std::string response = "{\"ok\":true,\"entryId\":\"" + jsonEscape(entryId)
    + "\",\"enabled\":" + (enabled ? "true" : "false") + "}";
  return jade_text_create(response.c_str());
}

const char* JADEVIEW_CALL modelGroups(uint32_t, const char*) {
  struct Model { std::string id; std::string name; };
  struct Group { std::string id; std::string name; std::vector<Model> models; };
  std::ifstream input(g_models_file);
  std::vector<Group> groups;
  bool models = false;
  std::string line;
  while (std::getline(input, line)) {
    const size_t indent = line.find_first_not_of(" \t");
    const std::string item = trim(line.substr(indent == std::string::npos ? 0 : indent));
    if (item.rfind("- id:", 0) == 0) {
      const std::string id = trim(item.substr(5));
      if (indent <= 2) { groups.push_back({ id, "", {} }); models = false; }
      else if (!groups.empty()) { groups.back().models.push_back({ id, "" }); models = true; }
    } else if (item == "models:") {
      models = true;
    } else if (item.rfind("name:", 0) == 0 && !groups.empty()) {
      const std::string name = trim(item.substr(5));
      if (models && !groups.back().models.empty()) groups.back().models.back().name = name;
      else groups.back().name = name;
    }
  }
  std::ostringstream json;
  json << "{\"groups\":[";
  for (size_t i = 0; i < groups.size(); ++i) {
    if (i) json << ',';
    const auto& group = groups[i];
    json << "{\"id\":\"" << jsonEscape(group.id) << "\",\"name\":\"" << jsonEscape(group.name.empty() ? group.id : group.name) << "\",\"models\":[";
    for (size_t j = 0; j < group.models.size(); ++j) {
      if (j) json << ',';
      const auto& model = group.models[j];
      json << "{\"id\":\"" << jsonEscape(model.id) << "\",\"name\":\"" << jsonEscape(model.name.empty() ? model.id : model.name) << "\"}";
    }
    json << "]}";
  }
  json << "]}";
  return jade_text_create(json.str().c_str());
}

const char* JADEVIEW_CALL openFullWorkspace(uint32_t, const char*) {
  WebViewWindowOptions options{};
  options.title = "DSH-GUI Full Workspace";
  options.width = 1360;
  options.height = 900;
  options.resizable = 1;
  options.frame_style = "normal";
  options.background_color = "#101522ff";
  options.theme = "Dark";
  options.maximizable = 1;
  options.minimizable = 1;
  options.x = -1;
  options.y = -1;
  options.min_width = 980;
  options.min_height = 680;
  options.focus = 1;
  WebViewSettings settings{};
  settings.background_throttling = 1;
  settings.allow_right_click = 0;
  settings.focused = 1;
  const uint32_t window = create_webview_window("http://127.0.0.1:3080/", 0, &options, &settings);
  return jade_text_create(window ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"Unable to open DSH workspace\"}");
}

const char* JADEVIEW_CALL onLoaded(uint32_t window, const char*) {
  if (window == g_window) {
    set_window_visible(g_window, 1);
  }
  return jade_text_create("{}");
}

const char* JADEVIEW_CALL onAllWindowsClosed(uint32_t, const char*) {
  if (!g_exit_requested.exchange(true)) {
    shutdownBackend();
    jadeview_exit();
  }
  return jade_text_create("{}");
}

const char* JADEVIEW_CALL onAppReady(uint32_t window, const char*) {
  if (window == 0 || g_window != 0) return jade_text_create("{}");
  WebViewWindowOptions options{};
  options.title = "DSH-GUI";
  options.width = 1280;
  options.height = 850;
  options.resizable = 1;
  options.frame_style = "no-titlebar";
  options.transparent = 1;
  options.background_color = "#00000000";
  options.theme = "Dark";
  options.maximizable = 1;
  options.minimizable = 1;
  options.x = -1;
  options.y = -1;
  options.min_width = 900;
  options.min_height = 620;
  options.focus = 1;
  options.hide_window = 1;
  WebViewSettings settings{};
  settings.background_throttling = 1;
  settings.allow_right_click = 0;
  settings.focused = 1;
  settings.cors_whitelist = "JADE://jadeview";
  g_window = create_webview_window(g_url.c_str(), 0, &options, &settings);
  if (g_window) set_window_title(g_window, "DSH-GUI");
  return jade_text_create(g_window ? "{\"ok\":true}" : "{\"ok\":false}");
}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  SetCurrentProcessExplicitAppUserModelID(L"DeepSeekHarness.DSHGUI");
  WSADATA winsockData{};
  const bool winsockReady = WSAStartup(MAKEWORD(2, 2), &winsockData) == 0;
  wchar_t modulePath[MAX_PATH]{};
  GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
  const auto root = std::filesystem::path(modulePath).parent_path();
  g_root = root;
  g_sessions_root = root / "sessions";
  g_session_archive_file = root / "session-archive.conf";
  const auto contentRoot = std::filesystem::exists(root / "web") ? root : root / "jade-frontend";
  g_models_file = root / "models.yaml";
  if (!std::filesystem::exists(g_models_file)) g_models_file = contentRoot.parent_path() / "models.yaml";
  g_plugin_patch_file = root / "profiles" / "web" / "cordis.patch.yml";
  g_plugin_overrides_file = root / "plugin-overrides.conf";
  const auto data = std::filesystem::path(std::getenv("USERPROFILE") ? std::getenv("USERPROFILE") : ".") / ".dsh" / "jade-data";
  std::filesystem::create_directories(data);
  const auto log = data / "jade-frontend.log";
  const std::string logPath = utf8(log);
  const std::string dataPath = utf8(data);
  if (winsockReady) launchBackend(root);
  if (!JadeView_init(1, logPath.c_str(), dataPath.c_str(), "DSH-GUI", "dsh-studio", 0)) {
    shutdownBackend();
    if (winsockReady) WSACleanup();
    return 2;
  }
  register_ipc_handler("window_control", windowControl);
  register_ipc_handler("model_groups", modelGroups);
  register_ipc_handler("api_request", apiRequest);
  register_ipc_handler("plugin_toggle", pluginToggle);
  register_ipc_handler("open_full_workspace", openFullWorkspace);
  register_ipc_handler("session_context_action", sessionContextAction);
  register_ipc_handler("session_archive", sessionArchive);
  register_ipc_handler("approval_action", approvalAction);
  register_ipc_handler("clipboard_read", clipboardRead);
  jade_on(JADEVIEW_EVENT_APP_READY, onAppReady);
  jade_on(JADEVIEW_EVENT_WEBVIEW_DID_FINISH_LOAD, onLoaded);
  jade_on(JADEVIEW_EVENT_WINDOW_ALL_CLOSED, onAllWindowsClosed);
  char url[2048]{};
  const auto web = contentRoot / "web";
  if (!set_protocol_service_path(utf8(web).c_str(), url, sizeof(url), 0)) return 3;
  g_url = std::string(url) + "?v=36";
  const int result = run_message_loop();
  shutdownBackend();
  jadeview_exit();
  JadeView_unload();
  if (winsockReady) WSACleanup();
  return result;
}
