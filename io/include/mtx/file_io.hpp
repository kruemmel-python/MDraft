#pragma once
#include <string>
namespace mtx {
std::string read_file(const std::string& path);
void write_file_atomic(const std::string& path, const std::string& data);
}
