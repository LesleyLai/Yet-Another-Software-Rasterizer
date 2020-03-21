#pragma once

#ifndef YASR_FILE_UTIL_HPP
#define YASR_FILE_UTIL_HPP

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

[[nodiscard]] auto read_file(std::string_view path) -> std::string;

#endif // YASR_FILE_UTIL_HPP
