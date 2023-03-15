#include "npc.hpp"
#include "player.hpp"

#include <SDL2/SDL2_gfxPrimitives.h>

#include <sstream>


mw::npc::npc(double phys_radius, const pt2d_d &pos)
: phys_object(phys_radius, pos),
  safe_access(this),
  m_mind {nullptr},
  m_body {nullptr},
  m_vision_radius {5},
  m_speed {0.01},
  m_color {0xFFFFFFFF}
{ }

mw::npc::~npc()
{
  if (m_mind)
    delete m_mind;
  if (m_body)
    delete m_body;
}

void
mw::npc::draw(const area_map &map) const
{
  SDL_Renderer *rend = map.get_sdl().get_renderer();
  const pt2d_i pixpos = map.point_to_pixels(get_position());
  const double r = get_radius() * map.get_scale();
  aacircleColor(rend, pixpos.x, pixpos.y, r, m_color);
}

void
mw::npc::update(area_map &map, int n_ticks_passed)
{
  if (not has_mind())
    return;

  m_mind->update(map, n_ticks_passed);

  vec2d_d destination;
  if (m_mind->get_destination(map, destination))
    set_internal_acceleration(normalized(destination)*0.0001);
}

void
mw::npc::receive_hit(area_map &map, const hit &hit)
{
  if (has_body())
  {
    const std::string what = m_body->receive_hit(map, hit);
    if (m_nickname.has_value())
    {
      std::ostringstream ss;
      ss << m_nickname.value() << " " << what;
      map.add_message(ss.str());
    }
  }
}

void
mw::npc::get_sights(vision_processor &visproc) const
{
  sight s;
  if (cast_sight(visproc.get_source(), circle {get_position(), get_radius()-0.01}, s))
    visproc.add_sight(s, 0);
}

void
mw::npc::draw(const area_map &map, const sight &s) const
{
  SDL_Renderer *rend = map.get_sdl().get_renderer();
  const pt2d_i pixpos = map.point_to_pixels(get_position());

  //const int visr = m_vision_radius*map.get_scale();
  //aacircleRGBA(rend, pixpos.x, pixpos.y, visr, 0xFF, 0x00, 0x00, 0x55);

  const double r = get_radius() * map.get_scale();
  double cphi1 = s.sight_data.circle.cphi1;
  double cphi2 = s.sight_data.circle.cphi2;
  if (interval_size({cphi1, cphi2}) > M_PI)
    std::swap(cphi1, cphi2);
  arcColor(rend, pixpos.x, pixpos.y, r, cphi1*180/M_PI, cphi2*180/M_PI, m_color);
}

