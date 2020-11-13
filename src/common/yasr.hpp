#ifndef YASR
#define YASR

#include "image.hpp"

#include <cstdint>
#include <span>

#include <beyond/math/point.hpp>

constexpr int width = 1200;
constexpr int height = 800;

struct Vertex {
  beyond::Point3 pos;
  beyond::Vec3 normal;
  beyond::Vec2 texcoord;
};

namespace yasr {

void draw_indexed(Image& image, std::vector<float> depth_buffer,
                  std::span<Vertex> vertices, std::span<uint32_t> indices);

} // namespace yasr

#endif // YASR
