#ifndef NPC_HPP
#define NPC_HPP

#include "object.hpp"
#include "area_map.hpp"
#include "vision.hpp"
#include "mind.hpp"
#include "body.hpp"
#include "utl/safe_access.hpp"

#include <optional>


namespace mw {

struct npc_builder {
  std::optional<pt2d_d> starting_position;
  std::optional<double> radius;
  std::optional<std::unique_ptr<mind>> npc_mind;
  std::optional<std::unique_ptr<body>> npc_body;
};


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

  const mind&
  get_mind() const noexcept
  { return *m_mind; }

  mind&
  get_mind() noexcept
  { return *m_mind; }

  template <typename Mind, typename ...Args>
  Mind&
  make_mind(Args&& ...args)
  {
    m_mind = std::make_unique<Mind>(std::forward<Args>(args)...);
    return *static_cast<Mind*>(m_mind.get());
  }

  template <typename Body, typename ...Args>
  Body&
  make_body(Args&& ...args)
  {
    m_body = std::make_unique<Body>(std::forward<Args>(args)...);
    return *static_cast<Body*>(m_body.get());
  }

  void
  draw(const area_map &map) const override;

  void
  update(area_map &map, int n_ticks_passed) override;

  void
  receive_hit(area_map &map, const hit &hit);

  bool
  is_gone() const override
  { return m_body->is_dead(); }

  void
  get_sights(vision_processor &visproc) const override;

  void
  draw(const area_map &map, const sight &s) const override;

  private:
  std::unique_ptr<mind> m_mind;
  std::unique_ptr<body> m_body;
  double m_vision_radius;
  double m_speed;
  color_t m_color;
  std::optional<std::string> m_nickname;
}; // class mw::npc


} // namespace mw

#endif
