#include "mtx/file_io.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace mtx {
namespace {

#ifdef _WIN32
std::wstring widen_utf8(const std::string& s) {
  if (s.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
  if (n <= 0) {
    std::wstring out;
    out.reserve(s.size());
    for (unsigned char c : s) out.push_back(static_cast<wchar_t>(c));
    return out;
  }
  std::wstring out(static_cast<std::size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
  return out;
}

std::string win_error_message(const char* operation, const std::string& path, DWORD code) {
  return std::string(operation) + " '" + path + "' failed, win32_error=" +
         std::to_string(static_cast<unsigned long>(code));
}

[[noreturn]] void throw_last_win_error(const char* operation, const std::string& path) {
  throw std::runtime_error(win_error_message(operation, path, GetLastError()));
}
#endif

} // namespace

std::string read_file(const std::string& path) {
  if (path.empty()) return {};
#ifdef _WIN32
  const std::wstring wpath = widen_utf8(path);
  HANDLE h = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return {};
  LARGE_INTEGER size{};
  if (!GetFileSizeEx(h, &size) || size.QuadPart <= 0) {
    CloseHandle(h);
    return {};
  }
  std::string out(static_cast<std::size_t>(size.QuadPart), '\0');
  std::size_t done = 0;
  while (done < out.size()) {
    const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(out.size() - done, 1u << 20));
    DWORD got = 0;
    if (!ReadFile(h, out.data() + done, chunk, &got, nullptr)) {
      const DWORD err = GetLastError();
      CloseHandle(h);
      throw std::runtime_error(win_error_message("read", path, err));
    }
    if (got == 0) break;
    done += got;
  }
  CloseHandle(h);
  out.resize(done);
  return out;
#else
  std::ifstream f(path, std::ios::binary);
  if (!f) return {};
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
#endif
}

void write_file_atomic(const std::string& path, const std::string& data) {
  if (path.empty()) throw std::runtime_error("path is empty");
  const std::string tmp = path + ".tmp";
#ifdef _WIN32
  const std::wstring wpath = widen_utf8(path);
  const std::wstring wtmp = widen_utf8(tmp);
  HANDLE h = CreateFileW(wtmp.c_str(), GENERIC_WRITE, 0, nullptr,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) throw_last_win_error("cannot open tmp file", tmp);
  std::size_t done = 0;
  while (done < data.size()) {
    const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(data.size() - done, 1u << 20));
    DWORD written = 0;
    if (!WriteFile(h, data.data() + done, chunk, &written, nullptr) || written == 0) {
      const DWORD err = GetLastError();
      CloseHandle(h);
      DeleteFileW(wtmp.c_str());
      throw std::runtime_error(win_error_message("cannot write tmp file", tmp, err));
    }
    done += written;
  }
  if (!FlushFileBuffers(h)) {
    const DWORD err = GetLastError();
    CloseHandle(h);
    DeleteFileW(wtmp.c_str());
    throw std::runtime_error(win_error_message("cannot flush tmp file", tmp, err));
  }
  if (!CloseHandle(h)) {
    DeleteFileW(wtmp.c_str());
    throw_last_win_error("cannot close tmp file", tmp);
  }
  if (!MoveFileExW(wtmp.c_str(), wpath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    const DWORD err = GetLastError();
    DeleteFileW(wtmp.c_str());
    throw std::runtime_error(win_error_message("rename tmp file", path, err));
  }
#else
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("cannot open tmp file '" + tmp + "'");
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!f) {
      std::remove(tmp.c_str());
      throw std::runtime_error("cannot write tmp file '" + tmp + "'");
    }
  }
  std::remove(path.c_str());
  if (std::rename(tmp.c_str(), path.c_str()) != 0) {
    std::remove(tmp.c_str());
    throw std::runtime_error("rename tmp file failed for '" + path + "'");
  }
#endif
}

} // namespace mtx
