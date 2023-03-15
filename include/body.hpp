#ifndef BODY_HPP
#define BODY_HPP

#include "hit.hpp"
#include "random.hpp"
#include "geometry.hpp"

#include <SDL2/SDL.h>


namespace mw {
class area_map;
class phys_object;


struct body {
  virtual ~body() = default;

  virtual std::string receive_hit(area_map &map, const hit &hit) = 0;
  virtual bool is_dead() const = 0;
};


class simple_body: public body {
  public:
  simple_body(phys_object &owner, double hp, double multavg = 1)
  : m_owner {owner},
    m_hp {hp},
    m_random_mult {multavg},
    m_enable_blood {true},
    m_blood_stain_tex {nullptr}
  { }

  void
  set_blood(bool enable) noexcept
  { m_enable_blood = enable; }

  void
  set_blood_stain(SDL_Texture *tex) noexcept
  { m_blood_stain_tex = tex; }

  std::string
  receive_hit(area_map &map, const hit &hit) override;

  bool
  is_dead() const override
  { return m_hp <= 0; }

  private:
  phys_object &m_owner;
  double m_hp;
  std::poisson_distribution<> m_random_mult;
  bool m_enable_blood;
  SDL_Texture *m_blood_stain_tex;
}; // class mw::simple_body


} // namespace mw

#endif
