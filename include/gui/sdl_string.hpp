#ifndef UTL_SDL_STRING_HPP
#define UTL_SDL_STRING_HPP

#include "common.hpp"
#include "geometry.hpp"
#include "color_manager.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <memory>
#include <string>
#include <optional>

namespace mw {
inline namespace gui {

struct texture_info {
  uint32_t format;
  int access, w, h;
};

class sdl_string_data {
  public:
  enum tag_type { blended };

  sdl_string_data(const std::string &text, SDL_Surface *surf)
  : m_text {text}, m_surf {surf}
  { }

  ~sdl_string_data() { SDL_FreeSurface(m_surf); }

  const std::string&
  get_text() const noexcept
  { return m_text; }

  SDL_Texture*
  get_texture(SDL_Renderer *rend, texture_info &texinfo) const;

  SDL_Texture*
  get_texture(SDL_Renderer *rend) const
  { texture_info texinfo; return get_texture(rend, texinfo); }

  void
  clear_cache() const;

  private:
  std::string m_text;
  SDL_Surface *m_surf;

  struct cache_type {
    SDL_Renderer *rend;
    SDL_Texture *tex;
    texture_info texinfo;

    cache_type(): rend {nullptr}, tex {nullptr} { }
    ~cache_type() { if (tex) SDL_DestroyTexture(tex); }
  };
  mutable cache_type m_cache;
};

typedef std::shared_ptr<sdl_string_data> sdl_string_data_ptr;

template <typename ...Args>
sdl_string_data_ptr
make_sdl_string_data(Args&& ...args)
{ return std::make_shared<sdl_string_data>(std::forward<Args>(args)...); }


class sdl_string {
  public:
  sdl_string(const sdl_string_data_ptr &data): m_data {data} { }
  sdl_string(const sdl_string&) = default;
  sdl_string(sdl_string&&) = default;
  sdl_string& operator = (const sdl_string&) = default;
  sdl_string& operator = (sdl_string&&) = default;

  static sdl_string
  blended_wrapped(TTF_Font *font, const std::string &text, color_t fg,
      uint32_t wrap_len);

  static sdl_string
  blended(TTF_Font *font, const std::string &text, color_t fg);

  void
  get_texture_info(SDL_Renderer *rend, texture_info &texinfo) const
  { m_data->get_texture(rend, texinfo); }

  const std::string&
  get_text() const noexcept
  { return m_data->get_text(); }

  void
  draw(SDL_Renderer *rend, pt2d_i at, texture_info &texinfo) const;

  void
  draw(SDL_Renderer *rend, pt2d_i at) const
  { texture_info texinfo; draw(rend, at, texinfo); }

  private:
  sdl_string_data_ptr m_data;
}; // class mw::gui::sdl_string


class sdl_string_factory {
  public:
  sdl_string_factory(TTF_Font *font, color_t fg = color_manager::instance()["Normal"])
  : m_font {font}, m_fg {fg}
  { }

  sdl_string_factory(const sdl_string_factory &other) = default;


  void set_fg_color(color_t fg) noexcept { m_fg = fg; }
  void set_bg_color(color_t bg) noexcept { m_bg = bg; }
  void set_belnded() noexcept { m_bg = std::nullopt; }
  void set_style(int sty) noexcept;
  void set_wrap(uint32_t wrap_len) noexcept { m_wrap_len = wrap_len; }
  void set_nowrap() noexcept { m_wrap_len = std::nullopt; }
  void set_font_size(int point_size) noexcept { m_font_size = point_size; }

  sdl_string
  operator () (const std::string &str) const;

  private:
  TTF_Font *m_font;
  color_t m_fg;
  std::optional<color_t> m_bg;
  std::optional<int> m_sty;
  std::optional<uint32_t> m_wrap_len;
  std::optional<int> m_font_size;

}; // class mw::gui::sdl_string_factory


} // namespace mw::gui
} // namespace mw

#endif
