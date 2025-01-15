#include "mind.hpp"
#include "npc.hpp"
#include "player.hpp"


mw::simple_ai::simple_ai(npc &slave, const area_map &map)
: m_slave {slave},
  m_current_time {0},
  m_exploration {map, slave.get_vision_radius()},
  m_do_chase_player {true}
{ }

void
mw::simple_ai::update(const area_map &map, int n_ticks_passed)
{
  m_current_time += n_ticks_passed;

  sync_vision(map);
  sync_path(map);

  // TODO: calculate once but make a propper decay according to n_ticks_passed
  if (m_current_time - m_exploration.timestamp > 10)
  {
    // TODO: sync vision/mark raduis with NPC's vision radius
    m_exploration.destination = m_exploration.explr(m_vision.visproc);
    m_exploration.timestamp = m_current_time;
  }
}

void
mw::simple_ai::sync_vision(const area_map &map)
{
  if (m_vision.timestamp == m_current_time)
    return;
  // update timestamp (whatever will be done below is the final state for this
  // timepoint)
  m_vision.timestamp = m_current_time;

  // re-process sights
  m_vision.visproc.reset();
  m_vision.visproc.set_source(
      {m_slave.get_position(), m_slave.get_vision_radius()});
  m_vision.visproc.set_ignore(&m_slave);
  m_vision.visproc.load_obstacles(map.get_vis_obstacles());
  m_vision.visproc.process();

  // update vision on player
  m_vision.visible_player = std::nullopt;
  for (const sight &s : m_vision.visproc.get_sights())
  {
    //info("see %p", s.static_data->obs);
    const player *plyr = dynamic_cast<const player*>(s.static_data.obs);
    if (plyr)
    {
      //info("see player");
      m_vision.visible_player = plyr;
      break;
    }
  }
}

void
mw::simple_ai::sync_path(const area_map &map)
{
  if (m_path.timestamp == m_current_time)
    return;
  // update timestamp (whatever will be done below is the final decision)
  m_path.timestamp = m_current_time;

  // need to use vision to decide on path
  sync_vision(map);

  // if player is in sight => go for him
  // otherwize, if reached old destination => finish the path
  // otherwize => keep old path
  if (see_player())
  {
    if (m_do_chase_player)
      m_path.destination = get_player().get_position();
  }
  else if (m_path.destination.has_value())
  {
    const double dr = mag(m_slave.get_position() - m_path.destination.value());
    if (dr <= m_slave.get_radius())
      m_path.destination = std::nullopt;
  }
}

bool
mw::simple_ai::get_destination(const area_map &map, vec2d_d &destination)
{
  if (m_path.destination.has_value())
  {
    destination = normalized(m_path.destination.value() - m_slave.get_position());
    return true;
  }

  if (m_exploration.destination.has_value())
  {
    destination = normalized(m_exploration.destination.value());
    return true;
  }
  // branches below are effectively dead
  else if (m_slave.get_velocity() != vec2d_d {0, 0})
  {
    destination = normalized(m_slave.get_position() - m_slave.get_position());
    return true;
  }
  else
    return false;
}


