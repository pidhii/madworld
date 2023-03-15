#include "body.hpp"
#include "area_map.hpp"
#include "projectiles.hpp"
#include "effects.hpp"
#include <sstream>


namespace mw {

std::string
mw::simple_body::receive_hit(area_map &map, const hit &hit)
{
  const double dmg = hit.strength * m_random_mult(random_generator);
  m_hp -= dmg;

  // norification about received damage
  std::ostringstream ss;
  ss << "received " << dmg << " damage (" << std::max(m_hp, 0.) << " HP remaining)";
  const std::string report = ss.str();

  // add blood-stain
  if (m_blood_stain_tex)
  {
    const double sz = 0.7;
    const rectangle dstbox {m_owner.get_position() - vec2d_d {sz/2, sz/2}, sz, sz};
    map.add_terrain_overlay(m_blood_stain_tex, dstbox, 0xFF);
  }

  // blood droplets
  if (m_enable_blood and hit.surface_normal.has_value())
  {
    const vec2d_d &n = normalized(hit.surface_normal.value());
    std::cauchy_distribution<double> cauchy {0, 1e-2};
    for (int i = 0; i < 10; ++i)
    {
      const double phi = fmod(cauchy(random_generator), M_PI);
      const vec2d_d dir = rotated(n, phi);
      const pt2d_d origin = m_owner.get_position() + dir*m_owner.get_radius()*2;
      simple_projectile *blood = new simple_projectile {origin, dir*0.01, 0.2, 2};
      blood->enable_flag(simple_projectile::remove_on_collision);
      blood->set_line_color(0xFF000077);

      const object_id id = map.add_object(blood);
      map.register_phys_object(id);
    }
  }

  return report;
}


} // namespace mw
