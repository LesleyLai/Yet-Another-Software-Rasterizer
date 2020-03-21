#ifndef YASR_IMAGE_HPP
#define YASR_IMAGE_HPP

#include <cstdio>
#include <vector>

#include "color.hpp"

class Image {
public:
  Image(size_t width, size_t height) noexcept
      : width_(width), height_(height), data_(width * height)
  {
  }

  auto width() const noexcept -> size_t
  {
    return width_;
  }

  auto height() const noexcept -> size_t
  {
    return height_;
  }

  auto unsafe_at(size_t x, size_t y) const noexcept -> RGB
  {
    // No bound checking
    return data_[y * width_ + x];
  }

  auto unsafe_at(size_t x, size_t y) noexcept -> RGB&
  {
    // No bound checking
    return data_[y * width_ + x];
  }

  [[nodiscard]] auto data() noexcept -> RGB*
  {
    return data_.data();
  }

  [[nodiscard]] auto data() const noexcept -> const RGB*
  {
    return data_.data();
  }

private:
  //  void bound_checking(size_t x, size_t y) const
  //  {
  //    if (x >= width_ || y >= height_) {
  //      std::stringstream ss;
  //      ss << "Access image out of index:\n";
  //      ss << "Input x:" << x << " y:" << y << "\n";
  //      ss << "width:" << width_ << " height:" << height_ << "\n";
  //      throw std::out_of_range{ss.str().c_str()};
  //    }
  //  }

  size_t width_;
  size_t height_;
  std::vector<RGB> data_;
};

#endif // YASR_IMAGE_HPP
