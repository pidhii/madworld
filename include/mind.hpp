#ifndef MIND_HPP
#define MIND_HPP

#include "geometry.hpp"
#include "vision.hpp"
#include "ai/exploration.hpp"

#include <optional>


namespace mw {

class npc;
class player;
class area_map;


struct mind {
  virtual
  ~mind() = default;

  virtual bool
  get_destination(const area_map &map, vec2d_d &destination) = 0;

  virtual void
  update(const area_map &map, int n_ticks_passed) = 0;
}; // class mw::mind


class simple_ai: public mind {
  public:
  simple_ai(npc &slave, const area_map &map);

  bool
  get_destination(const area_map &map, vec2d_d &destination) override;

  void
  update(const area_map &map, int n_ticks_passed) override;

  const explorer&
  get_explorer() const noexcept
  { return m_exploration.explr; }

  void
  set_chase_player(bool v)
  { m_do_chase_player = v; }

  bool
  get_chase_player()
  { return m_do_chase_player; }

  private:
  void
  sync_vision(const area_map &map);

  void
  sync_path(const area_map &map);

  bool
  see_player() const noexcept
  { return m_vision.visible_player.has_value(); }

  const player&
  get_player() const
  { return *m_vision.visible_player.value(); }

  private:
  npc &m_slave;

  time_t m_current_time;

  struct vision_data {
    vision_data(): timestamp {0} { }
    vision_processor visproc;
    std::optional<const player*> visible_player;
    time_t timestamp;
  } m_vision;

  struct path_data {
    path_data(): timestamp {0} { }
    std::optional<pt2d_d> destination;
    time_t timestamp;
  } m_path;

  struct exploration_data {
    exploration_data(const area_map &map, double r)
    : explr {map, r}, timestamp {0}
    { }
    explorer explr;
    std::optional<vec2d_d> destination;
    time_t timestamp;
  } m_exploration;

  bool m_do_chase_player;
}; // class mw::simple_ai


} // namespace mw

#endif
