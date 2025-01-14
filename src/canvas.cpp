#include "canvas.hpp"
#include "vision.hpp"

#include <SDL2/SDL2_gfxPrimitives.h>

#include <assert.h>


void
mw::canvas::draw_circle(const circle &circ, color_t color)
{
  const pt2d_i pixpos {m_viewport(circ.center)};
  // XXX must draw eliptic arc if scales are not equal
  assert(m_viewport.get_x_scale() == m_viewport.get_y_scale());
  const double pixradius = circ.radius * m_viewport.get_x_scale();
  aacircleColor(m_rend, pixpos.x, pixpos.y, pixradius, color);
}

void
mw::canvas::draw_arc(const circle &circ, double phi_from, double phi_to,
                     color_t color)
{
  const pt2d_i pixcenter {m_viewport(circ.center)};
  // XXX must draw eliptic arc if scales are not equal
  assert(m_viewport.get_x_scale() == m_viewport.get_y_scale());
  const double pixradius = circ.radius * m_viewport.get_x_scale();
  arcColor(m_rend, pixcenter.x, pixcenter.y, pixradius, phi_from*180/M_PI,
           phi_to*180/M_PI, color);
}
