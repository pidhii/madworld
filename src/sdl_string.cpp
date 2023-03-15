#include "gui/sdl_string.hpp"
#include "logging.h"

#include <SDL2/SDL_ttf.h>

#include <stdexcept>


void
mw::gui::sdl_string_data::clear_cache() const
{
  m_cache.rend = nullptr;
  if (m_cache.tex)
  {
    SDL_DestroyTexture(m_cache.tex);
    m_cache.tex = nullptr;
  }
}


SDL_Texture*
mw::gui::sdl_string_data::get_texture(SDL_Renderer *rend, texture_info &texinfo) const
{
  if (m_cache.rend != rend)
  {
    if (SDL_Texture *new_tex = SDL_CreateTextureFromSurface(rend, m_surf))
    {
      if (0 == SDL_QueryTexture(new_tex, &m_cache.texinfo.format,
            &m_cache.texinfo.access, &m_cache.texinfo.w, &m_cache.texinfo.h))
      {
        SDL_DestroyTexture(m_cache.tex);
        m_cache.rend = rend;
        m_cache.tex = new_tex;
      }
      else
      {
        const std::string &err = SDL_GetError();
        error("failed to query texture (%s)", err.c_str());
        SDL_DestroyTexture(new_tex);
        clear_cache();
        throw std::runtime_error {err};
      }
    }
    else
    {
      const std::string &err = SDL_GetError();
      error("failed to crate texture (%s)", err.c_str());
      throw std::runtime_error {err};
    }
  }

  texinfo = m_cache.texinfo;
  return m_cache.tex;
}

mw::gui::sdl_string
mw::gui::sdl_string::blended_wrapped(TTF_Font *font, const std::string &text,
    color_t fg, uint32_t wrap_len)
{
  SDL_Color fgc;
  fgc.r = (fg >>  0) & 0xFF;
  fgc.g = (fg >>  8) & 0xFF;
  fgc.b = (fg >> 16) & 0xFF;
  fgc.a = (fg >> 24) & 0xFF;

  SDL_Surface *surf;
  surf = TTF_RenderText_Blended_Wrapped(font, text.c_str(), fgc, wrap_len);

  if (surf == nullptr)
  {
    error("failed to render text (%s)", SDL_GetError());
    throw std::runtime_error {"failed to render text"};
  }
  return sdl_string {make_sdl_string_data(text, surf)};
}

mw::gui::sdl_string
mw::gui::sdl_string::blended(TTF_Font *font, const std::string &text, color_t fg)
{
  SDL_Color fgc;
  fgc.r = (fg >>  0) & 0xFF;
  fgc.g = (fg >>  8) & 0xFF;
  fgc.b = (fg >> 16) & 0xFF;
  fgc.a = (fg >> 24) & 0xFF;

  SDL_Surface *surf;
  surf = TTF_RenderText_Blended(font, text.c_str(), fgc);

  if (surf == nullptr)
  {
    error("failed to render text (%s)", SDL_GetError());
    throw std::runtime_error {"failed to render text"};
  }
  return sdl_string {make_sdl_string_data(text, surf)};
}


void
mw::gui::sdl_string::draw(SDL_Renderer *rend, pt2d_i at, texture_info &texinfo) const
{
  SDL_Texture *tex = m_data->get_texture(rend, texinfo);

  SDL_Rect rect;
  rect.x = at.x;
  rect.y = at.y;
  rect.w = texinfo.w;
  rect.h = texinfo.h;
  SDL_RenderCopy(rend, tex, NULL, &rect);
}


mw::gui::sdl_string
mw::gui::sdl_string_factory::operator () (const std::string &str) const
{
  SDL_Color fg;
  fg.r = (m_fg >>  0) & 0xFF;
  fg.g = (m_fg >>  8) & 0xFF;
  fg.b = (m_fg >> 16) & 0xFF;
  fg.a = (m_fg >> 24) & 0xFF;

  int cursty;
  if (m_sty.has_value())
  {
    cursty = TTF_GetFontStyle(m_font);
    TTF_SetFontStyle(m_font, m_sty.value());
  }

  SDL_Surface *surf;
  if (m_bg.has_value())
  {
    SDL_Color bg;
    bg.r = (m_bg.value() >>  0) & 0xFF;
    bg.g = (m_bg.value() >>  8) & 0xFF;
    bg.b = (m_bg.value() >> 16) & 0xFF;
    bg.a = (m_bg.value() >> 24) & 0xFF;
    if (m_wrap_len.has_value())
      surf = TTF_RenderText_Shaded_Wrapped(m_font, str.c_str(), fg, bg, m_wrap_len.value());
    else
      surf = TTF_RenderText_Shaded(m_font, str.c_str(), fg, bg);
  }
  else
  {
    if (m_wrap_len.has_value())
      surf = TTF_RenderText_Blended_Wrapped(m_font, str.c_str(), fg, m_wrap_len.value());
    else
      surf = TTF_RenderText_Blended(m_font, str.c_str(), fg);
  }

  if (m_sty.has_value())
    TTF_SetFontStyle(m_font, cursty);

  if (surf == nullptr)
  {
    error("failed to render text (%s)", SDL_GetError());
    throw std::runtime_error {"failed to render text"};
  }

  return sdl_string {make_sdl_string_data(str, surf)};
}

void
mw::gui::sdl_string_factory::set_style(int sty) noexcept
{
  if (sty == 0)
    m_sty == std::nullopt;
  else
    m_sty = sty;
}

