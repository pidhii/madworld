#include "gui/ttf_font.hpp"
#include "logging.h"

#include <stdexcept>


TTF_Font *
mw::ttf_font::operator () (int point_size) const
{
  const auto it = m_handles.find(point_size);

  // Return existing handle if present
  if (it != m_handles.end())
    return it->second.get();

  // Otherwize, create new TTF_Font handle for the given `point_size`
  TTF_Font *font = TTF_OpenFont(m_font_path.c_str(), point_size);
  if (font == nullptr)
  {
    error("failed to open font (%s)", SDL_GetError());
    throw std::runtime_error {"failed to open font"};
  }

  std::shared_ptr<TTF_Font> shfont {font, [] (TTF_Font *p) { TTF_CloseFont(p); }};
  m_handles.emplace(point_size, shfont);
  return font;
}
