#pragma once

#include <doctest/doctest.h>

#include <iofet/directory_mask_iterator.hpp>
#include <iofet/file.hpp>


TEST_CASE("directory_mask_iterator::end") {
  iofet::directory_mask_iterator it;
  REQUIRE(it == end(it));
}


TEST_CASE("directory_mask_iterator::begin") {
  using namespace iofet;
  std::error_code ec;
  file::touch("file.test");
  directory_mask_iterator it{ ".", "*", ec};
  REQUIRE(it == begin(it));
  file::remove("file.test");
}

TEST_CASE("directory_mask_iterator::increment") {
  using namespace iofet;
  file::touch("a.1");  
  file::touch("c.2");
  file::touch("b.1");
  std::error_code ec;
  directory_mask_iterator it{ ".", "*.1", ec};
  REQUIRE(it != end(it));
  REQUIRE(it->path().filename() == "a.1");
  ++it;
  REQUIRE(it != end(it));
  REQUIRE(it->path().filename() == "b.1");
  ++it;
  REQUIRE(it == end(it));
  file::remove("a.1");
  file::remove("b.1");
  file::remove("c.2");
}
