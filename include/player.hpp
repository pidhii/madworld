#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "object.hpp"
#include "area_map.hpp"
#include "vision.hpp"
#include "ability.hpp"

#include <unordered_set>


namespace mw {

enum class ability_slot {
  lmb,
};

} // namespace mw

template <>
struct std::hash<mw::ability_slot> {
  size_t
  operator () (mw::ability_slot slot) const noexcept
  { return std::hash<size_t>()(static_cast<size_t>(slot)); }
};


namespace mw {

class player: public phys_object, public vis_obstacle {
  public:
  player(const pt2d_d &pos, double movespeed);

  ~player();

  void
  set_glow(SDL_Texture *tex, double radius) noexcept
  { m_glowtex = tex; m_glowradius = radius; }

  void
  set_move_direction(const vec2d_d &dir) noexcept
  { m_move_dir = normalized(dir); }

  void
  draw(const area_map &map) const override;

  void
  update(area_map &map, int n_ticks_passed) override;

  bool
  is_gone() const override
  { return false; }

  void
  get_sights(vision_processor &visproc) const override;

  void
  draw(const area_map &map, const sight &s) const override;

  void
  set_ability(ability_slot slot, ability *ab) noexcept;

  ability&
  get_ability(ability_slot slot)
  { return *m_abilities[slot]; }

  private:
  vec2d_d m_move_dir;
  double m_speed;
  SDL_Texture *m_glowtex;
  double m_glowradius;
  std::unordered_map<ability_slot, ability*> m_abilities;
}; // class mw::player

} // namespace mw


#endif
