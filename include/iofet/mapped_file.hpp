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


#include <system_error>

#include "file.hpp"


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
#include <memoryapi.h>
#include <handleapi.h>

#else
  
#error Unsupported system

#endif // WIN32


namespace iofet {
  
  
class mapped_file {
public:
  using size_type = long long;
  using offset_type = long long;


  struct region {
    friend class mapped_file;

    char* address{nullptr};
    size_type size{0};

    region() noexcept = default;
    ~region() noexcept { if(!!address) dispose(); }
    region(region const&) = delete;    
    region& operator = (region const&) = delete;


    explicit operator bool () const noexcept {
      return address != nullptr;
    }
    
    
    region(region&& other) noexcept:
      address(other.address), size(other.size) {
      other.address = nullptr;
    }


    region& operator = (region&& other) noexcept {
      if(address != nullptr)
        dispose();
      address = other.address; other.address = nullptr;
      size = other.size;
      return *this;
    }


  private:
    
    region(char* address, size_type size) noexcept:
      address(address), size(size)
    { }

#ifdef _WIN32
    void dispose() noexcept { UnmapViewOfFile(address); }
#else
    void dispose() noexcept { munmap(address, size); }
#endif // _WIN32
  }; // region


  static mapped_file open(std::string path) noexcept {
    return mapped_file{file::open_to_rw(std::move(path))};
  }

  
  static std::error_code last_error() noexcept {
    return file::last_error();
  }


  mapped_file() noexcept = default;
  mapped_file(mapped_file const&) noexcept = delete;
  mapped_file& operator = (mapped_file const&) noexcept = delete;
  mapped_file(mapped_file&&) noexcept = default;
  mapped_file& operator = (mapped_file&&) noexcept = default;
  
  
  region map(offset_type offset, size_type size) noexcept {    
    return region{mmap(offset, size), size};
  }
  
  
  region map() noexcept {
    region result;
    auto const size = file_.size();
    if (!size)
      return std::move(result);
    return std::move(result = region{ mmap(0, *size), *size });
  }


#ifdef _WIN32

  explicit operator bool() const noexcept {
    return mapping_ != INVALID_HANDLE_VALUE;
  }

  static size_type granularity() {
    return 65536;
  }
  
#else
  
  explicit operator bool() const noexcept {
    return !!file_;
  }

  static size_type granularity() {
    return 65536;
  }

#endif // _WIN32

private:

  file file_;
  
#ifdef _WIN32
  
  HANDLE mapping_{INVALID_HANDLE_VALUE};

  explicit mapped_file(file&& f) noexcept:
    file_{std::move(f)} {
    mapping_ = CreateFileMappingW(file_.handle_, nullptr, PAGE_READWRITE,
                                  0, 0, nullptr);
  }
  
  
  char* mmap(offset_type offset, size_type size) noexcept {
    return reinterpret_cast<char*>(
            MapViewOfFile(mapping_, FILE_MAP_READ | FILE_MAP_WRITE,
                          DWORD(offset >> 32), DWORD(offset), size));
  }

    
#else

  explicit mapped_file(cellarium::file&& f) noexcept:
    file_{std::move(f)} {
  }
  
  
  char* mmap(offset_type offset, size_type size) noexcept {
    char* address = mmap(nullptr, size, PROT_READ|PROT_WRITE,
                         MAP_PRIVATE|MAP_NORESERVE, file_.handle_, offset);
    if(address == MAP_FAILED)
      return nullptr;
    return address;
  }

 
#endif  

}; // mapped_file
  
  
} // cellarium
