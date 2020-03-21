#ifndef YASR_COLOR_HPP
#define YASR_COLOR_HPP

#include <iosfwd>

/**
 * \brief 24 bit float RGB color
 */
struct RGB {
  /// Red component of the color.
  float r = 0;

  /// Green component of the color.
  float g = 0;

  /// Blue component of the color.
  float b = 0;

  /**
   * \brief Constructs a new black 24 bit RGB RGB.
   */
  constexpr RGB() noexcept = default;

  /**
   * \brief Constructs a new 24 bit float RGB RGB with given r,g,b components.
   * \param red, green, blue
   */
  constexpr RGB(float red, float green, float blue) noexcept
      : r{red}, g{green}, b{blue}
  {
  }

  /**
   * @brief Operator << of RGB.
   * @param os The output stream
   * @param color The color to output.
   */
  friend auto operator<<(std::ostream& os, const RGB& color) -> std::ostream&;

  //  /**
  //   * @brief Clamps the RGB values of color to [0, 1)
  //   */
  //  constexpr auto clamp() noexcept -> void
  //  {
  //    r = std::clamp(r, 0.0f, 1.0f);
  //    g = std::clamp(g, 0.0f, 1.0f);
  //    b = std::clamp(b, 0.0f, 1.0f);
  //  }
};

#endif // YASR_COLOR_HPP
