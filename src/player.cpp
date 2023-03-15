#include "player.hpp"

#include <SDL2/SDL2_gfxPrimitives.h>


mw::player::player(const pt2d_d &pos, double movespeed)
: phys_object(0.5, pos),
  m_move_dir {0, 0},
  m_speed {movespeed},
  m_glowtex {nullptr}
{ }

mw::player::~player()
{
  for (auto [_, ab] : m_abilities)
    delete ab;
}

void
mw::player::draw(const area_map &map) const
{
  SDL_Renderer *rend = map.get_sdl().get_renderer();
  const pt2d_i pixpos = map.point_to_pixels(get_position());

  if (m_glowtex)
  {
    const rectangle dstbox = {
      get_position() - vec2d_d(m_glowradius, m_glowradius),
      m_glowradius*2, m_glowradius*2
    };
    vision_processor localvision;
    localvision.set_ignore(this);
    map.blit_glow_with_shadowcast(m_glowtex, dstbox, 0x77,
        blit_flags::draw_sights, localvision);
  }

  const double r = 0.5 * map.get_scale();
  aacircleRGBA(rend, pixpos.x, pixpos.y, r, 0x00, 0xFF, 0x00, 0xFF);
}

void
mw::player::update(area_map &map, int n_ticks_passed)
{ set_internal_acceleration(m_move_dir*0.0001); }

void
mw::player::get_sights(vision_processor &visproc) const
{
  sight s;
  if (cast_sight(visproc.get_source(), circle {get_position(), 0.5}, s))
    visproc.add_sight(s, 0);
}

void
mw::player::draw(const area_map &map, const sight &s) const
{
  SDL_Renderer *rend = map.get_sdl().get_renderer();
  const pt2d_i pixpos = map.point_to_pixels(get_position());
  const double r = 0.5 * map.get_scale();
  double cphi1 = s.sight_data.circle.cphi1;
  double cphi2 = s.sight_data.circle.cphi2;
  if (interval_size({cphi1, cphi2}) > M_PI)
    std::swap(cphi1, cphi2);
  arcRGBA(rend, pixpos.x, pixpos.y, r, cphi1*180/M_PI, cphi2*180/M_PI,
      0x00, 0xFF, 0x00, 0xFF);
}

void
mw::player::set_ability(ability_slot slot, ability *ab) noexcept
{
  if (m_abilities[slot])
    delete m_abilities[slot];
  m_abilities[slot] = ab;
}

