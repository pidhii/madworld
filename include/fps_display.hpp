#ifndef FPS_DISPLAY_HPP
#define FPS_DISPLAY_HPP

#include "ui_layer.hpp"
#include "sdl_environment.hpp"
#include "gui/sdl_string.hpp"

#include <boost/format.hpp>


namespace mw {

class fps_display: public ui_float {
  public:
  fps_display(sdl_environment &sdl, const sdl_string_factory &strfac, int x,
      int y)
  : m_sdl {sdl}, make_string {strfac}, m_fmt {"%d"},  m_x {x}, m_y {y}
  { }

  void
  set_format(const boost::format &fmt) noexcept
  { m_fmt = fmt; }

  void
  set_format(const std::string &fmt)
  { set_format(boost::format {fmt}); }

  void
  draw() const override;

  private:
  sdl_environment &m_sdl;
  sdl_string_factory make_string;
  mutable std::optional<int> m_last_shown_fps;
  mutable std::optional<sdl_string> m_str;
  mutable boost::format m_fmt;
  int m_x, m_y;
}; // mw::fps_display

} // namespace mw

#endif
