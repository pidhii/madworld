#ifndef PROJECTILES_HPP
#define PROJECTILES_HPP

#include <SDL2/SDL.h>

#include "area_map.hpp"
#include "geometry.hpp"
#include "object.hpp"


namespace mw {


struct projectile: public phys_object {
  using phys_object::phys_object;
}; // struct mw::projectile


class simple_projectile: public projectile {
  public:
  enum flags : uint8_t {
    none = 0,
    remove_on_collision = 1 << 0,
  };

  simple_projectile(const pt2d_d &origin, const vec2d_d &dir,
      double length = 0.2, double maxdist = 100);

  void
  set_flags(uint8_t flags)
  { m_flags = flags; }

  void
  enable_flag(uint8_t flag)
  { m_flags &= flag; }

  void
  disable_flag(uint8_t flag)
  { m_flags &= ~flag; }

  void
  set_line_color(color_t color) noexcept { m_line_color = color; }

  void
  set_glow(SDL_Texture *tex, double radius, uint8_t alpha) noexcept;

  void
  draw(const area_map &map) const override;

  void
  update(area_map &map, int n_ticks_passed) override;

  bool
  is_gone() const override
  { return m_gone; }

  virtual vec2d_d
  act_on_object(area_map &map, phys_object *subj) override;

  virtual void
  on_collision(area_map &map, phys_obstacle *obs) override
  {
    if (m_flags & remove_on_collision)
      m_gone = true;
  }

  virtual void
  on_falloff(area_map &map)
  { m_gone = true; }

  protected:
  void
  set_gone() noexcept
  { m_gone = true; }

  private:
  pt2d_d m_origin;
  vec2d_d m_dir;
  double m_speed;
  double m_lencoef;
  double m_maxdist;
  uint8_t m_flags;
  bool m_gone;

  color_t m_line_color;

  SDL_Texture *m_glowtex;
  double m_glowradius;
  uint8_t m_glowalpha;
}; // class mw::simple_projectile


class simple_bullet: public simple_projectile {
  public:
  template <typename ...Args>
  simple_bullet(Args&& ...args)
  : simple_projectile(std::forward<Args>(args)...),
    m_hit_strength {1}
  { }

  void
  set_damage(double dmg) noexcept
  { m_hit_strength = dmg; }

  void
  on_collision(area_map &map, phys_obstacle *obs) override;

  void
  on_falloff(area_map &map) override;

  private:
  double m_hit_strength;
}; // class mw::simple_bullet

} // namespace mw


#endif
