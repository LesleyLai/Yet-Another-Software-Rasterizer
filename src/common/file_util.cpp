#include "file_util.hpp"

auto read_file(std::string_view path) -> std::string
{
  std::ifstream file{path.data()};

  if (!file.is_open()) {
    spdlog::error("Cannot open file {}\n", path);
    std::fflush(stdout);
  }

  std::stringstream ss;
  // read file's buffer contents into streams
  ss << file.rdbuf();

  return ss.str();
}
