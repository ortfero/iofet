# iofet
C++ 17 header-only library for dealing with files, memory mapped files, directories

## Snippets

### Dealing with files

```cpp
#include <cassert>
#include <cstring>
#include <iostream>
#include <iofet/file.hpp>

int main(int, char**) {
  using namespace iofet;
  auto f = file::create("test.file");
  if(!f)
    std::cout << file::last_error().message() << std::endl;
  assert(!!f);
  f.close();
  
  f = file::open_to_rw("test.file");  
  assert(!!f);
  bool const hello_written = f.write("hello", 5);
  assert(hello_written);  
  f.close();
  
  f = file::open_to_append("test.file");
  assert(!!f);
  bool const world_written = f.write(" world", 6);
  assert(world_written);
  f.close();
  
  f = file::open_to_read("test.file");
  assert(!!f);
  char buffer[11];
  bool const read = f.read(buffer, sizeof(buffer));
  assert(read);
  assert(std::memcmp(buffer, "hello world", 11) == 0);
  f.close();
  
  f = file::open_to_rw("test.file");
  assert(!!f);
  bool const resized = f.resize(0);
  assert(resized);
  auto const maybe_size = f.size();
  assert(maybe_size);
  assert(*maybe_size == 0);
  f.close();
  
  auto const touched = file::touch("test.file");
  assert(touched);
  auto const used_by = file::used_by("test.file");
  assert(!used_by);
  auto const removed = file::remove("test.file");
  assert(removed);
  
  return 0;
}
```

### Dealing with memory-mapped files

```cpp
#include <cassert>
#include <iostream>
#include <iofet/file.hpp>
#include <iofet/mapped_file.hpp>


int main(int, char**) {
  using namespace iofet;
  
  auto file = file::open_to_rw("test.file");
  file.resize(2 * mapped_file::granularity());
  file.close();
  auto target = mapped_file::open("test.file");
  auto region = target.map(0, mapped_file::granularity());
  // auto region = target.map();
  assert(region);
  
  std::cout << "mapped address: " << region.address << std::endl;
  std::cout << "mapped size: " region.size << std::endl;
  
  return 0;
}
```

### Iterating over directory by mask

```cpp
#include <cassert>
#include <iostream>
#include <iofet/file.hpp>
#include <iofet/directory_mask_iterator.hpp>

int main(int, char**) {
  using namespace iofet;
  
  auto const files = {"a.log", "b.data", "c.log"};
  
  for(it: files)
    file::touch(it);
  
  std::error_code ec;
  for(it: directory_mask_iterator(".", "*.log", ec))
    std::cout << it->path().filename() << std::endl;
    
  if(!!ec)
    std::cout << ec.message() << std::endl;
  
  for(it: files)
    file::remove(it);
    
  return 0;
}
```