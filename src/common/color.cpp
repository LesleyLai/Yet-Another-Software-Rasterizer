#include "color.hpp"

#include <ostream>

auto operator<<(std::ostream& os, const RGB& color) -> std::ostream&
{
  os << "RGB(" << color.r << "," << color.g << "," << color.b << ")";
  return os;
}
