#ifndef GUI_COMPOSER_HPP
#define GUI_COMPOSER_HPP

#include "gui/components.hpp"


namespace mw {
inline namespace gui {


class composer {
  public:
  composer(sdl_environment &sdl, const ttf_font &font)
  : m_sdl {sdl},
    m_default_strfac {font, 0xFFFFFFFF},
    m_label_strfac {m_default_strfac},
    m_button_normal_strfac {m_default_strfac},
    m_button_hover_strfac {m_default_strfac}
  {
    m_button_hover_strfac.set_style(TTF_STYLE_BOLD);
  }

  /**
   * @name Label
   * @{
   */
  sdl_string_factory&
  get_label_factory() noexcept
  { return m_label_strfac; }

  label*
  make_label(const std::string &text);
  /** @} */

  /**
   * @name Button
   * @{
   */
  std::pair<sdl_string_factory&, sdl_string_factory&>
  get_text_button_factory() noexcept
  { return {m_button_normal_strfac, m_button_hover_strfac}; }

  button*
  make_button(const std::string &text);
  /** @} */

  private:
  sdl_environment &m_sdl;

  sdl_string_factory m_default_strfac;

  sdl_string_factory m_label_strfac;
  sdl_string_factory m_button_normal_strfac;
  sdl_string_factory m_button_hover_strfac;
};


} // namespace mw::gui
} // namespace mw

#endif
