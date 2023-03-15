#ifndef HIT_HPP
#define HIT_HPP

#include "geometry.hpp"

#include <optional>


namespace mw {

struct hit {
  enum aoe_flag: uint32_t {
    pointwise = 0,
  };

  uint32_t aoe_flags = pointwise;
  double strength = 0;

  std::optional<vec2d_d> surface_normal = std::nullopt;

  hit&
  with_surface_normal(const vec2d_d &n) noexcept
  { surface_normal = n; return *this; }
};


inline hit
make_pointwise_hit(double strength)
{
  return hit {
    .aoe_flags = hit::pointwise,
    .strength = strength,
  };
}


} // namespace mw

#endif
