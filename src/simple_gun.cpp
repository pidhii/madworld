#include "ability.hpp"
#include "player.hpp"
#include "projectiles.hpp"


void
mw::simple_gun::activate(area_map &map, player &user, const pt2d_d &pt)
{
  // Amo check
  if (m_amo == 0)
  {
    if (m_antispam())
      map.add_message("out of amo");
    return;
  }

  // Limit rate of fire
  const uint32_t now = SDL_GetTicks();
  if (now - m_prevshot <= m_delay)
    return;

  // Amo consumption
  if (m_amo > 0)
    m_amo -= 1;

  // Shoot it
  mw::vec2d_d dir = normalized(pt-user.get_position());
  double phi = fmod(m_cauchy(m_gen), M_PI);
  if (fabs(phi) > 10./180.*M_PI)
    phi = 0;
  dir = rotated(dir, phi);
  simple_bullet *sb = new simple_bullet {
    user.get_position()+dir*user.get_radius()*1.1, dir*0.1, 0.2
  };
  sb->set_mass(0.1);
  sb->set_damage(0.5);
  if (m_glowtex)
    sb->set_glow(m_glowtex, 3, 0x55);

  const object_id bullet_id =  map.add_object(sb);
  map.register_phys_object(bullet_id);
  m_prevshot = now;
}
