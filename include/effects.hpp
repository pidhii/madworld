#ifndef EFFECTS_HPP
#define EFFECTS_HPP

#include "object.hpp"
#include "geometry.hpp"
#include "area_map.hpp"
#include "vision.hpp"

#include <SDL2/SDL2_gfxPrimitives.h>

#include <cmath>
#include <assert.h>
#include <array>


namespace mw {


class effect: public object { }; // class mw::effect


class terrain_overlay: public effect {
  public:
  terrain_overlay(const rectangle &dstbox, SDL_Texture *tex, double duration)
  : m_duration {duration},
    m_acctime {0},
    m_dstbox {dstbox},
    m_tex {tex}
  { }

  virtual void
  draw(const area_map &map) const override
  { map.add_terrain_overlay(m_tex, m_dstbox, 0xFF); }

  void
  update(area_map &map, int n_ticks_passed) override
  { m_acctime += n_ticks_passed; }

  bool
  is_gone() const override
  { return m_acctime > m_duration; }

  protected:
  double m_duration;
  double m_acctime;
  rectangle m_dstbox;
  SDL_Texture *m_tex;
}; // class mw::terrain_overlay


class simple_explosion: public effect {
  public:
  simple_explosion(const pt2d_d &pos, double duration, double radius,
      uint32_t color)
  : m_pos {pos},
    m_duration {duration},
    m_acctime {0},
    m_radius {radius},
    m_color {color}
  { }

  virtual void
  draw(const area_map &map) const override
  {
    SDL_Renderer *rend = map.get_sdl().get_renderer();
    const double r = m_radius/(1. + exp(-m_acctime));

    const pt2d_i pixpos = map.point_to_pixels(m_pos);
    const double pixr = r * map.get_scale();

    filledCircleColor(rend, pixpos.x, pixpos.y, pixr, m_color);
    if (pixr > 20)
      aacircleColor(rend, pixpos.x, pixpos.y, pixr, m_color);
  }

  void
  update(area_map &map, int n_ticks_passed) override
  { m_acctime += n_ticks_passed; }

  bool
  is_gone() const override
  { return m_acctime > m_duration; }

  protected:
  pt2d_d m_pos;
  double m_duration;
  double m_acctime;
  double m_radius;
  uint32_t m_color;
}; // class mw::simple_explosion


} // namespace mw

#endif
