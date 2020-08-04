#pragma once

#include <doctest/doctest.h>

#include <iofet/mapped_file.hpp>


TEST_CASE("mapped_file::mapped_file") {
  iofet::mapped_file target;
  REQUIRE(!target);
}


TEST_CASE("mapped_file::open") {
  auto file = iofet::file::create("test.file");
  file.close();
  auto target = iofet::mapped_file::open("test.file");
  REQUIRE(target);
}


TEST_CASE("mapped_file::map/1") {
  auto file = iofet::file::open_to_rw("test.file");
  using iofet::mapped_file;
  file.resize(2 * mapped_file::granularity());
  file.close();  
  auto target = mapped_file::open("test.file");
  auto region = target.map(0, mapped_file::granularity());
  REQUIRE(region);
  region = mapped_file::region{};
  REQUIRE(!region);
}


TEST_CASE("mapped_file::map/2") {
  using iofet::mapped_file;
  auto target = mapped_file::open("test.file");
  auto region1 = target.map(0, mapped_file::granularity());
  REQUIRE(region1);
  auto region2 = target.map(mapped_file::granularity(), mapped_file::granularity());
  REQUIRE(region2);
}
