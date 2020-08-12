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


#include <string>
#include <filesystem>
#include <system_error>


namespace iofet {
  
  namespace detail {  
  
  
    template<typename C> bool matched(C const* pattern, C const* text) {
    
      if(pattern == text)
        return true;
      
      if(pattern == nullptr)
        return *text == '\0';
      
      if(text == nullptr)
        return *pattern == '\0';
      
      C const* last_text = nullptr;
      C const* last_pattern = nullptr;
      
      while(*text != '\0')
        switch(*pattern) {
          case '*':
            // new star-loop: backup positions in pattern and text
            last_text = text;
            last_pattern = ++pattern;
            continue;
          case '?':
            // ? matched any character or we matched the current non-NUL character
            text++;
            pattern++;
            continue;
          default:
            if(*pattern == *text) {
              // we matched the current non-NUL character
              text++;
              pattern++;
            } else {
              // if no stars we fail to match
              if (last_pattern == nullptr)
                return false;
              // star-loop: backtrack to the last * by restoring the backup positions 
              // in the pattern and text
              text = ++last_text;
              pattern = last_pattern;
            }
        }
      
      // ignore trailing stars
      while(*pattern == '*')
        ++pattern;
      
      // at end of text means success if nothing else is left to match
      if(*pattern != '\0')
        return false;
      
      return true;
    }

  } // detail

  
  
  class directory_mask_iterator {
  public:
  
    using value_type = std::filesystem::directory_entry;
    using pointer = std::filesystem::directory_entry const*;
    using reference = std::filesystem::directory_entry const&;
    using iterator_category = std::input_iterator_tag;
    
    directory_mask_iterator() noexcept = default;
    directory_mask_iterator(directory_mask_iterator const&) = default;
    directory_mask_iterator& operator = (directory_mask_iterator const&) = default;
    directory_mask_iterator(directory_mask_iterator&&) = default;
    directory_mask_iterator& operator = (directory_mask_iterator&&) = default;
    
    
    directory_mask_iterator(std::filesystem::path const& path,
                            std::string mask,
                            std::error_code& ec):
      it_{path, ec}, mask_{std::move(mask)}
    {
      if (it_ == end(it_))
        return;
      if (detail::matched(mask_.data(), it_->path().filename().string().data()))
        return;
      operator ++ ();
    }
    
    
    directory_mask_iterator(std::filesystem::path path,
                            std::filesystem::directory_options options,
                            std::string mask,
                            std::error_code& ec):
      it_{path, options, ec}, mask_{std::move(mask)}
    {
      if (it_ == end(it_))
        return;
      if (detail::matched(mask_.data(), it_->path().filename().string().data()))
        return;
      operator ++ ();
    }
    
    
    bool operator == (directory_mask_iterator const& other) const noexcept {
      return it_ == other.it_;
    }
    
    
    bool operator != (directory_mask_iterator const& other) const noexcept {
      return it_ != other.it_;
    }
    
    
    directory_mask_iterator& operator ++ () {
      std::error_code ec;
      for(;;) {
        it_.increment(ec);
        if(it_ == end(it_))
          return *this;
        if(detail::matched(mask_.data(), it_->path().filename().string().data()))
          return *this;
      }
    }
    
    
    std::filesystem::directory_entry const& operator * () const {
      return it_.operator * ();
    }
    
    
    std::filesystem::directory_entry const* operator -> () const {
      return it_.operator -> ();
    }
    
  private:
    std::filesystem::directory_iterator it_;
    std::string mask_;
  };
  
  
  directory_mask_iterator begin(directory_mask_iterator const& iter) noexcept {
    return iter;
  }
  
  
  directory_mask_iterator end(directory_mask_iterator const&) noexcept {
    return directory_mask_iterator{};
  }
  
}
