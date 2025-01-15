#include "npc.hpp"

#include <sstream>

#include <boost/format.hpp>


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
{ }

void
mw::npc::draw(const area_map &map) const
{ map.get_canvas().draw_circle({get_position(), get_radius()}, m_color); }

void
mw::npc::update(area_map &map, int n_ticks_passed)
{
  m_mind->update(map, n_ticks_passed);

  vec2d_d destination;
  if (m_mind->get_destination(map, destination))
    set_internal_acceleration(normalized(destination)*0.0001);
}

void
mw::npc::receive_hit(area_map &map, const hit &hit)
{
  const std::string what = m_body->receive_hit(map, hit);
  if (m_nickname.has_value())
  {
    std::ostringstream ss;
    ss << m_nickname.value() << " " << what;
    map.add_message(ss.str());
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
  //const int visr = m_vision_radius*map.get_scale();
  //aacircleRGBA(rend, pixpos.x, pixpos.y, visr, 0xFF, 0x00, 0x00, 0x55);

  map.get_canvas().draw_arc(
      {get_position(), get_radius()},
      s.sight_data.circle.cphi2,
      s.sight_data.circle.cphi1,
      m_color
  );
}

