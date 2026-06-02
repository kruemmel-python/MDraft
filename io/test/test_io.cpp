#include "mtx/file_io.hpp"
#include <cassert>
#include <iostream>
#include <cstdio>
int main() {
  const std::string p = "io_test.tmp";
  mtx::write_file_atomic(p, "abc");
  assert(mtx::read_file(p) == "abc");
  std::remove(p.c_str());
  std::cout << "io ok\n";
}
