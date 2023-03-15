#include "object.hpp"
#include "physics.hpp"

#include <cmath>

static 
double
_overlap_area(const mw::circle &A, const mw::circle &B) {

  const double d = hypot(B.center.x - A.center.x, B.center.y - A.center.y);

  if (d < A.radius + B.radius) {

    const double a = A.radius * A.radius;
    const double b = B.radius * B.radius;

    const double x = (a - b + d * d) / (2 * d);
    const double z = x * x;
    const double y = sqrt(a - z);

    if (d <= abs(B.radius - A.radius)) {
      return M_PI * std::min(a, b);
    }
    return a * asin(y / A.radius) + b * asin(y / B.radius) - y * (x + sqrt(z + b - a));
  }
  return 0;
}

mw::vec2d_d
mw::phys_object::act_on_object(area_map &map, phys_object *subj)
{
  const double d =
    hypot(subj->get_position().x - m_position.x, subj->get_position().y - m_position.y);
  if (d < m_radius + subj->get_radius())
    on_collision(map, subj);

  const double overlap_area =
    _overlap_area({m_position, m_radius}, {subj->get_position(), subj->get_radius()});
  return normalized(subj->get_position() - m_position) * overlap_area * 1;
}
