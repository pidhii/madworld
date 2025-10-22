#ifndef GUI_TTF_FONT_HPP
#define GUI_TTF_FONT_HPP

#include <SDL2/SDL_ttf.h>

#include <string>
#include <map>
#include <memory>


namespace mw {
inline namespace gui {

struct ttf_font {
  ttf_font(const std::string &font_path)
  : m_font_path {font_path}
  { }

  std::string_view
  get_font_path() const noexcept
  { return m_font_path; }

  TTF_Font *
  operator () (int point_size) const;

  private:
  std::string m_font_path;
  mutable std::map<int, std::shared_ptr<TTF_Font>> m_handles;
}; // struct mw::ttf_font

} // namespace mw::gui
} // namespace mw

#endif