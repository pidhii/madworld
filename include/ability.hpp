#ifndef ABILITY_HPP
#define ABILITY_HPP

#include "geometry.hpp"
#include "area_map.hpp"
#include "utl/frequency_limiter.hpp"

#include <random>


namespace mw {

class player;

struct ability {
  virtual
  ~ability() = default;

  virtual void
  activate(area_map &map, player &user, const pt2d_d &pt) = 0;
}; // struct mw::ability


class simple_gun: public ability {
  public:
  simple_gun(int amo = -1, double rate = 20, double spread = 1e-2)
  : m_amo {amo},
    m_delay {uint32_t(1e3/rate)},
    m_prevshot {0},
    m_gen {std::random_device{}()},
    m_cauchy {0, spread},
    m_glowtex {nullptr},
    m_glowalpha {0x55},
    m_antispam {std::chrono::seconds{1}}
  { }

  void
  activate(area_map &map, player &user, const pt2d_d &pt) override;

  void
  set_bullet_glow(SDL_Texture *tex, uint8_t alpha) noexcept
  { m_glowtex = tex, m_glowalpha = alpha; }

  private:
  int m_amo;
  uint32_t m_delay;
  time_t m_prevshot;
  std::mt19937 m_gen;
  std::cauchy_distribution<double> m_cauchy;
  SDL_Texture *m_glowtex;
  double m_glowalpha;
  frequency_limiter m_antispam;
}; // class mw::simple_gun


class open_door: public ability {
  public:
  void
  activate(area_map &map, player &user, const pt2d_d &pt) override;
};


class close_door: public ability {
  public:
  void
  activate(area_map &map, player &user, const pt2d_d &pt) override;
};

} // namespace mw

#endif
