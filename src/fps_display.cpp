#include "fps_display.hpp"
#include "video_manager.hpp"


void
mw::fps_display::draw() const
{
  const int fps = fps_guardian.get_fps();

  if (m_last_shown_fps.has_value() and m_str.has_value() and
      m_last_shown_fps.value() == fps)
  {
    m_str.value().draw(m_sdl.get_renderer(), {m_x, m_y});
  }
  else
  {
    m_str = make_string((m_fmt % fps).str());
    m_str.value().draw(m_sdl.get_renderer(), {m_x, m_y});
  }

  m_last_shown_fps = fps;
}

