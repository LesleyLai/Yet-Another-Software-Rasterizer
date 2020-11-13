#include "yasr.hpp"

#include <cmath>

#include <beyond/math/transform.hpp>
#include <beyond/math/vector.hpp>

#include <stb_image.h>

namespace {

[[maybe_unused]] auto line(beyond::IPoint2 p0, beyond::IPoint2 p1, Image& image,
                           const RGB& color) -> void
{
  bool steep = false;
  if (std::abs(p0.x - p1.x) <
      std::abs(p0.y - p1.y)) { // if the line is steep, we transpose the image
    p0.xy = p0.yx;
    p1.xy = p1.yx;
    steep = true;
  }
  if (p0.x > p1.x) { // make it left−to−right
    std::swap(p0, p1);
  }
  for (int x = p0.x; x <= p1.x; x++) {
    const auto t =
        static_cast<float>(x - p0.x) / static_cast<float>(p1.x - p0.x);
    const auto y = static_cast<int>(std::round(p0.y * (1.f - t) + p1.y * t));
    if (steep) {
      if (y >= 0 && y < image.width() && x >= 0 && x < image.height()) {
        image.unsafe_at(y, x) = color; // if transposed, de−transpose
      }
    } else {
      if (x >= 0 && x < image.width() && y >= 0 && y < image.height()) {
        image.unsafe_at(x, y) = color;
      }
    }
  }
}

constexpr auto normalized_to_screen(const beyond::Vec3& pt)
{
  return beyond::Point3{(pt.x + 1.f) * width / 2,
                        height - (pt.y + 1.f) * height / 2, -pt.z};
}

[[nodiscard]] auto barycentric(beyond::IPoint2 a, beyond::IPoint2 b,
                               beyond::IPoint2 c, beyond::IPoint2 p)
    -> beyond::Vec3
{
  const auto u = cross(beyond::Vec3(c.x - a.x, b.x - a.x, a.x - p.x),
                       beyond::Vec3(c.y - a.y, b.y - a.y, a.y - p.y));
  // Returns negative values for degenerated triangle
  if (std::abs(u.z) < 1e-2) {
    return beyond::Vec3(-1, 1, 1);
  }
  return beyond::Vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

[[nodiscard]] auto barycentric_interpolate(beyond::Vec3 coord, float a, float b,
                                           float c)
{
  return coord.x * a + coord.y * b + coord.z * c;
}

void triangle(const std::array<beyond::Point3, 3>& pts,
              const std::array<beyond::Vec2, 3>& uvs,
              std::vector<float>& depth_buffer, Image& image,
              const float* diffuse_texture, int diffuse_texture_width,
              int diffuse_texture_height, int diffuse_texture_channels,
              const RGB color)
{
  const beyond::IVec2 bbox_upper_bound{image.width() - 1, image.height() - 1};
  beyond::IVec2 bboxmin{bbox_upper_bound};
  beyond::IVec2 bboxmax{0, 0};
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 2; ++j) {
      bboxmin[j] = std::clamp(bboxmin[j], 0, static_cast<int>(pts[i][j]));
      bboxmax[j] = std::clamp(bboxmax[j], static_cast<int>(pts[i][j]),
                              bbox_upper_bound[j]);
    }
  }

  const auto to_int_xy = [](beyond::Point3 pt) {
    return beyond::IPoint2{static_cast<int>(pt.x), static_cast<int>(pt.y)};
  };

  for (int x = bboxmin.x; x < bboxmax.x; ++x) {
    for (int y = bboxmin.y; y < bboxmax.y; ++y) {
      if (x >= image.width() || x < 0 || y >= image.height() || y < 0) {
        continue;
      }

      const auto index = y * image.width() + x;

      const beyond::Vec3 bc_screen = barycentric(
          to_int_xy(pts[0]), to_int_xy(pts[1]), to_int_xy(pts[2]), {x, y});
      if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
        continue;

      const auto u =
          barycentric_interpolate(bc_screen, uvs[0].x, uvs[1].x, uvs[2].x);
      const auto v =
          barycentric_interpolate(bc_screen, uvs[0].y, uvs[1].y, uvs[2].y);
      const auto z =
          barycentric_interpolate(bc_screen, pts[0].z, pts[1].z, pts[2].z);

      if (depth_buffer[index] < z) {
        depth_buffer[index] = z;
        const int texture_x = std::max(u * diffuse_texture_width, 0.f);
        const int texture_y =
            diffuse_texture_height - std::max(v * diffuse_texture_height, 0.f);
        const float* target_pixels =
            diffuse_texture + (texture_y * diffuse_texture_width + texture_x) *
                                  diffuse_texture_channels;
        RGB final_color{
            color.r * target_pixels[0],
            color.g * target_pixels[1],
            color.b * target_pixels[2],
        };

        // Gamma correction
        final_color.r = std::pow(final_color.r, 1 / 2.2);
        final_color.g = std::pow(final_color.g, 1 / 2.2);
        final_color.b = std::pow(final_color.b, 1 / 2.2);
        image.unsafe_at(x, y) = final_color;
      }
    }
  }
}
} // anonymous namespace

namespace yasr {

void draw_indexed(Image& image, std::vector<float> depth_buffer,
                  std::span<Vertex> vertices, std::span<uint32_t> indices)
{
  using beyond::float_constants::pi;

  const auto view =
      beyond::look_at(beyond::Vec3{1.f, 0.8f, 3.f}, beyond::Vec3{0.f, 0.f, 0.f},
                      beyond::Vec3{0.f, 1.f, 0.f});

  const auto proj = beyond::perspective(
      beyond::Radian(pi / 3.f),
      static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.f);

  const auto light_dir = beyond::normalize(beyond::Vec3{0, 1, 5});

  constexpr const char* diffuse_texture_filename =
      "assets/textures/african_head_diffuse.tga";

  float* diffuse_texture_ = nullptr;
  int diffuse_texture_width_ = 0;
  int diffuse_texture_height_ = 0;
  int diffuse_texture_channels_ = 0;
  diffuse_texture_ =
      stbi_loadf(diffuse_texture_filename, &diffuse_texture_width_,
                 &diffuse_texture_height_, &diffuse_texture_channels_, 0);

  for (std::size_t i = 0; i < indices.size(); i += 3) {
    const std::array triangle_indices{indices[i], indices[i + 1],
                                      indices[i + 2]};

    std::array<beyond::Point3, 3> world_coords;
    std::array<beyond::Point3, 3> screen_coords;
    std::array<beyond::Vec2, 3> uvs;
    for (std::size_t j = 0; j < 3; ++j) {
      world_coords[j] = *beyond::bit_cast<beyond::Point3*>(
          &vertices[triangle_indices[j]].pos);

      const beyond::Vec4 normalized_coord =
          proj * view * beyond::Vec4{world_coords[j], 1};
      screen_coords[j] =
          normalized_to_screen(normalized_coord.xyz / normalized_coord.w);
      uvs[j] = *beyond::bit_cast<beyond::Vec2*>(
          &vertices[triangle_indices[j]].texcoord);
    }

    const auto normal = beyond::normalize(beyond::cross(
        world_coords[1] - world_coords[0], world_coords[2] - world_coords[1]));
    float intensity = beyond::dot(normal, light_dir);
    intensity = std::min(intensity, 1.f);
    triangle(screen_coords, uvs, depth_buffer, image, diffuse_texture_,
             diffuse_texture_width_, diffuse_texture_height_,
             diffuse_texture_channels_, RGB(intensity, intensity, intensity));
  }
}
} // namespace yasr