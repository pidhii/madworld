#include "gui/composer.hpp"


mw::label*
mw::gui::composer::make_label(const std::string &text)
{
  const sdl_string str = m_label_strfac(text);
  return new label {m_sdl, str};
}

mw::button*
mw::gui::composer::make_button(const std::string &text)
{
  const sdl_string nstr = m_button_normal_strfac(text);
  const sdl_string hstr = m_button_hover_strfac(text);
  return new button {m_sdl, new label {m_sdl, nstr}, new label {m_sdl, hstr}};
}

