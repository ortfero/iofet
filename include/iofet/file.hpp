/* This file is part of iofet library
 * Copyright 2020 Andrei Ilin <ortfero@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once


#include <cstdint>
#include <system_error>
#include <filesystem>
#include <optional>


#ifdef _WIN32

#if !defined(_X86_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_)
#if defined(_M_IX86)
#define _X86_
#elif defined(_M_AMD64)
#define _AMD64_
#elif defined(_M_ARM)
#define _ARM_
#elif defined(_M_ARM64)
#define _ARM64_
#endif
#endif

#include <minwindef.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <sysinfoapi.h>

#else
  
#error Unsupported system

#endif // WIN32


namespace iofet {
  
  
class file {
public:
  friend class mapped_file;

  using size_type = std::int64_t;
  using offset_type = std::int64_t;


#if defined(_WIN32)

  using handle_type = HANDLE;

  static file create(std::filesystem::path const& path) noexcept {
    HANDLE const handle =
        CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                    nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);  
    return file{handle};
  }


  static file open_to_append(std::filesystem::path const& path) noexcept {
    file result;
    HANDLE const handle =
        CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(handle == INVALID_HANDLE_VALUE)
      return result;
    SetFilePointer(handle, 0, nullptr, FILE_END);
    result.handle_ = handle;
    return result;
  }


  static file open_to_read(std::filesystem::path const& path) noexcept {
    HANDLE const handle =
        CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    return file{handle};
  }


  static file open_to_rw(std::filesystem::path const& path) noexcept {
    HANDLE const handle =
        CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    return file{handle};
  }


  static bool touch(std::filesystem::path const& path) noexcept {
    DWORD const attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
      HANDLE const handle =
        CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
          FILE_SHARE_READ | FILE_SHARE_WRITE,
          nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
      if (handle == INVALID_HANDLE_VALUE)
        return false;
      CloseHandle(handle);
      return true;
    }
    else {
      if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        return false;
      HANDLE const handle =
        CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
          FILE_SHARE_READ | FILE_SHARE_WRITE,
          nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
      if (handle == INVALID_HANDLE_VALUE)
        return false;
      FILETIME time;
      GetSystemTimeAsFileTime(&time);
      BOOL const updated = SetFileTime(handle, nullptr, &time, &time);
      CloseHandle(handle);
      return !!updated;
    }
  }


  static bool remove(std::filesystem::path const& path) noexcept {
    return !!DeleteFileW(path.c_str());
  }


  static bool used_by(std::filesystem::path const& path) noexcept {
    HANDLE const handle =
      CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
      return true;
    CloseHandle(handle);
    return false;
  }
  
  
  static std::error_code last_error() noexcept {
    return std::error_code(int(GetLastError()), std::system_category());
  }


  file() noexcept = default;
  ~file() noexcept { if(handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); }
  file(file const&) = delete; // only one file handle
  file& operator = (file const&) = delete; // only one file handle  
  explicit file(handle_type h) noexcept: handle_{h} { }
  explicit operator bool () const noexcept { return handle_ != INVALID_HANDLE_VALUE; }


  file(file&& source) noexcept: handle_(source.handle_) {
    source.handle_ = INVALID_HANDLE_VALUE;
  }


  file& operator = (file&& source) noexcept {
    if(handle_ != INVALID_HANDLE_VALUE)
      CloseHandle(handle_);
    handle_ = source.handle_; source.handle_ = INVALID_HANDLE_VALUE;
    return *this;
  }


  void close() noexcept {
    if(handle_ == INVALID_HANDLE_VALUE)
      return;
    CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
  }


  std::optional<size_type> size() const noexcept {
    std::optional<size_type> result;
    LARGE_INTEGER n;
    if(!GetFileSizeEx(handle_, &n))
       return result;
    return result = n.QuadPart;
  }


  bool resize(size_type new_size) noexcept {
    LARGE_INTEGER n;
    n.QuadPart = static_cast<LONGLONG>(new_size);
    if(!SetFilePointerEx(handle_, n, nullptr, 0))
      return false;
    if(!SetEndOfFile(handle_))
      return false;
    return true;
  }


  bool read(char* buffer, size_type size) noexcept {
    DWORD n;
    bool const ok = ReadFile(handle_, buffer, static_cast<DWORD>(size), &n, nullptr);
    if(!ok || n != static_cast<DWORD>(size))
      return false;
    return true;
  }


  bool write(char const* buffer, size_type size) noexcept {
    DWORD n;
    bool const ok = WriteFile(handle_, buffer, static_cast<DWORD>(size), &n, nullptr);
    if(!ok || n != static_cast<DWORD>(size))
      return false;
    return true;
  }


  template<typename T>
  bool binary_read(T& buffer) noexcept {
    return read(reinterpret_cast<char*>(&buffer), sizeof(T));
  }


  template<typename T>
  bool binary_write(T const& buffer) noexcept {
    return write(reinterpret_cast<char const*>(&buffer), sizeof(T));
  }
  
  
  bool seek(offset_type offset) noexcept {
    LARGE_INTEGER n;
    n.QuadPart = static_cast<LONGLONG>(offset);
    if(!SetFilePointerEx(handle_, n, nullptr, FILE_BEGIN))
      return false;
    return true;
  }

private:

  handle_type handle_{INVALID_HANDLE_VALUE};


#else
  
  using handle_type = int;

  static file create(std::string path) noexcept {
    int const handle = ::open(path.data(), O_CREAT | O_RDWR | O_TRUNC,
                              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return file{handle};
  }


  static file open_to_append(std::string path) noexcept {
    int const handle = ::open(path.data(), O_WRONLY);
    if(handle == -1)
      return file{handle}
    lseek(handle, 0, SEEK_END);
    return file{handle};
  }


  static file open_to_read(std::string path) noexcept {
    int const handle = ::open(path.data(), O_RDONLY);
    return file{handle};
  }


  static file open_to_rw(std::string path) noexcept {
    int const handle = ::open(path.data(), O_RDWR);
    return file{handle};
  }
  
  
  static std::error_code last_error() noexcept {
    return std::error_code(errno, std::system_category());
  }

  
  file() noexcept = default;
  ~file() noexcept { if(handle_ != -1) ::close(handle_); }
  file(file const&) noexcept = delete; // only one file handle
  file& operator = (file const&) noexcept = delete; // only one file handle
  file(file&& source) noexcept: handle_(source.handle_) { source.handle_ = -1; }
  explicit file(handle_type h) noexcept: handle_{h} { }
  explicit operator bool () const noexcept { return handle_ != -1; }


  file& operator = (file&& source) noexcept {
    if(handle_ != -1)
      ::close(handle_);
    handle_ = source.handle_; source.handle_ = -1;
    return *this;
  }


  void close() noexcept {
    if(handle_ == -1)
      return;
    ::close(handle_);
    handle_ = -1;
  }


  bool read(char* buffer, size_type size) noexcept {
    size_type const n = ::read(handle_, buffer, size);
    if(n != size)
      return false;
    return true;
  }
    

  bool write(char const* buffer, size_type size) noexcept {
    size_type const n = ::write(handle_, buffer, size);
    if(n != size)
      return false;
    return true;
  }
  

  bool resize(size_type size) noexcept {
    if(ftruncate(handle_, static_cast<off_t>(size)) == -1)
      return false;
    return true;
  }
  
private:

  handle_type handle_{-1};
  
#endif // _WIN32
}; // file

  
} // cellarium
