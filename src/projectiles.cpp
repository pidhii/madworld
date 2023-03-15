#include "projectiles.hpp"
#include "effects.hpp"


mw::simple_projectile::simple_projectile(const pt2d_d &origin, const vec2d_d &dir,
    double length, double maxdist)
: projectile(0, origin),
  m_origin {origin},
  m_dir {normalized(dir)},
  m_speed {mag(dir)},
  m_lencoef {length/sqrt(maxdist)},
  m_maxdist {maxdist},
  m_flags {none},
  m_gone {false},
  m_line_color {0xFF00FFFF},
  m_glowtex {nullptr}
{
  set_velocity(m_dir*m_speed);
  set_friction_coeff(0);
}

void
mw::simple_projectile::set_glow(SDL_Texture *tex, double radius, uint8_t alpha)
  noexcept
{ m_glowtex = tex; m_glowradius = radius; m_glowalpha = alpha; }


void
mw::simple_projectile::draw(const area_map &map) const
{
  if (m_gone)
    return;

  SDL_Renderer *rend = map.get_sdl().get_renderer();
  const double curpathlen = mag(get_position() - m_origin);
  const double linelen = sqrt(fabs(m_maxdist - curpathlen))*m_lencoef;

  if (m_glowtex)
  {
    const rectangle dstbox = {
      get_position() - vec2d_d(m_glowradius, m_glowradius),
      m_glowradius*2, m_glowradius*2
    };
    map.blit_glow_with_shadowcast(m_glowtex, dstbox, m_glowalpha,
        blit_flags::apply_global_vision | blit_flags::draw_sights);
  }

  pt2d_d start;
  if (curpathlen < linelen)
    start = m_origin;
  else
    start = get_position() - linelen*m_dir;

  const auto [xstart, ystart] = map.point_to_pixels(start);
  const auto [xend, yend] = map.point_to_pixels(get_position());

  const color_rgba rgba {m_line_color};
  SDL_SetRenderDrawColor(rend, rgba.r, rgba.g, rgba.b, rgba.a);
  SDL_RenderDrawLine(rend, xstart, ystart, xend, yend);
}

void
mw::simple_projectile::update(area_map &map, int n_ticks_passed)
{
  if (m_gone)
    return;

  if (mag2(get_position() - m_origin) > m_maxdist*m_maxdist)
    on_falloff(map);
}


mw::vec2d_d
mw::simple_projectile::act_on_object(area_map &map, phys_object *subj)
{
  const pt2d_d mypos = get_position();
  const pt2d_d subjpos = subj->get_position();
  const double d = mag(subjpos - mypos);
  if (d < subj->get_radius())
  {
    on_collision(map, subj);
    return normalized(subjpos - mypos)*get_mass()*mag(get_velocity())*50;
  }
  else
    return {0, 0};
}
void
mw::simple_bullet::on_collision(area_map &map, phys_obstacle *obs)
{
  if (phys_object *obj = dynamic_cast<phys_object*>(obs))
  {
    const vec2d_d n = get_position() - obj->get_position();
    obj->receive_hit(map,
        make_pointwise_hit(m_hit_strength).with_surface_normal(n));
  }
  else
    obs->receive_hit(map, make_pointwise_hit(m_hit_strength));

  map.add_object(new simple_explosion {get_position(), 200, 0.2, 0xAA00AACC});
  set_gone();
}

void
mw::simple_bullet::on_falloff(area_map &map)
{
  map.add_object(new simple_explosion {get_position(), 100, 0.1, 0xAA0044CC});
  set_gone();
}

