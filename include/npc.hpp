#ifndef NPC_HPP
#define NPC_HPP

#include "object.hpp"
#include "area_map.hpp"
#include "vision.hpp"
#include "mind.hpp"
#include "body.hpp"
#include "utl/safe_access.hpp"

#include <unordered_set>


namespace mw {


class npc: public phys_object, public vis_obstacle, public safe_access<npc> {
  public:
  npc(double phys_radius, const pt2d_d &pos);

  ~npc();

  void set_vision_radius(double r) noexcept { m_vision_radius = r; }
  void set_move_speed(double v) noexcept { m_speed = v; }
  void set_color(color_t c) noexcept { m_color = c; }
  void set_nickname(const std::string &s) noexcept { m_nickname = s; }

  double get_vision_radius() const noexcept { return m_vision_radius; }
  double get_speed() const noexcept { return m_speed; }
  const std::string& get_nickname() const noexcept {
    static const std::string empty_name = "";
    return m_nickname ? m_nickname.value() : empty_name;
  }

  bool
  has_mind() const noexcept
  { return m_mind != nullptr; }

  template <typename Mind, typename ...Args>
  Mind&
  make_mind(Args&& ...args)
  {
    if (m_mind)
      delete m_mind;
    m_mind = new Mind {std::forward<Args>(args)...};
    return *static_cast<Mind*>(m_mind);
  }

  const mind*
  get_mind() const noexcept
  { return m_mind; }

  mind*
  get_mind() noexcept
  { return m_mind; }

  bool
  has_body() const noexcept
  { return m_body != nullptr; }

  template <typename Body, typename ...Args>
  Body&
  make_body(Args&& ...args)
  {
    if (m_body)
      delete m_body;
    m_body = new Body {std::forward<Args>(args)...};
    return *static_cast<Body*>(m_body);
  }

  void
  draw(const area_map &map) const override;

  void
  update(area_map &map, int n_ticks_passed) override;

  void
  receive_hit(area_map &map, const hit &hit);

  bool
  is_gone() const override
  { return has_body() ? m_body->is_dead() : false; }

  void
  get_sights(vision_processor &visproc) const override;

  void
  draw(const area_map &map, const sight &s) const override;

  private:
  mind *m_mind;
  body *m_body;
  double m_vision_radius;
  double m_speed;
  color_t m_color;
  std::optional<std::string> m_nickname;
}; // class mw::npc


} // namespace mw

#endif
