#ifndef CANVAS_HPP
#define CANVAS_HPP

#include "common.hpp"
#include "geometry.hpp"

#include <SDL2/SDL.h>


namespace mw {

class canvas {
  public:
  canvas(SDL_Renderer *rend, const mapping &viewport)
  : m_rend {rend},
    m_viewport {viewport}
  { }

  void
  draw_circle(const circle &circ, color_t color);

  void
  draw_arc(const circle &circ, double phi_from, double phi_to, color_t color);

  private:
  SDL_Renderer *m_rend;
  mapping m_viewport;
}; // class mw::canvas

} // namespace mw

#endif
