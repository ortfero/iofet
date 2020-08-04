#pragma once

#include <string.h>
#include <filesystem>
#include <doctest/doctest.h>

#include <iofet/file.hpp>


TEST_CASE("file::file") {
  iofet::file target;
  REQUIRE(!target);
}


TEST_CASE("file::create") {
  auto target = iofet::file::create("test.file");
  REQUIRE(target);
  auto another = std::move(target);
  REQUIRE(!target);
  REQUIRE(another);
}


TEST_CASE("file::open_to_rw") {
  auto target = iofet::file::open_to_rw("test.file");
  REQUIRE(target);
  REQUIRE(target.write("hello", 5));
}


TEST_CASE("file::open_to_append") {
  auto target = iofet::file::open_to_append("test.file");
  REQUIRE(target);
  REQUIRE(target.write(" world", 6));
}


TEST_CASE("file::open_to_read") {
  auto target = iofet::file::open_to_read("test.file");
  REQUIRE(target);
  char buffer[11];
  REQUIRE(target.read(buffer, sizeof(buffer)));
  REQUIRE(memcmp(buffer, "hello world", 11) == 0);
}


TEST_CASE("file::resize") {
  auto target = iofet::file::open_to_rw("test.file");
  REQUIRE(target.resize(0));
  REQUIRE(std::filesystem::file_size("test.file") == 0);
}
