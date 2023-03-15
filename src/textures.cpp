#include "textures.hpp"
#include "logging.h"

#include <SDL2/SDL_image.h>

#include <boost/format.hpp>


mw::texture_storage::~texture_storage()
{
  for (const auto &ent : m_storage)
  {
    if (ent.second.surf)
      SDL_FreeSurface(ent.second.surf);
    if (ent.second.tex)
      SDL_DestroyTexture(ent.second.tex);
  }
}

const mw::texture_storage::entry&
mw::texture_storage::load(const std::string &name, const std::string &path)
{
  SDL_Surface *surf = IMG_Load(path.c_str());
  if (surf == nullptr)
    throw exception {"failed to load surface"}.in(__func__);

  SDL_Texture *tex = IMG_LoadTexture(m_rend, path.c_str());
  if (tex == nullptr)
  {
    SDL_FreeSurface(surf);
    throw exception {"failed to load texture"}.in(__func__);
  }

  const auto res = m_storage.emplace(name, entry {surf, tex});
  if (not res.second)
  {
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
    throw exception {"entry with identical name already exists"}.in(__func__);
  }

  return res.first->second;
}

const mw::texture_storage::entry&
mw::texture_storage::operator [] (const std::string &name) const
{
  const auto it = m_storage.find(name);
  if (it == m_storage.end())
    throw exception {"no such texture"}.in(__func__);
  return it->second;
}

SDL_Texture*
mw::create_texture(SDL_Renderer *rend, int access, int w, int h)
{
  SDL_Texture *tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA32,
      access, w, h);
  if (tex == nullptr)
  {
    throw exception {
      (boost::format("failed to create targetable texture: (%s)")
       % SDL_GetError()).str()};
  }
  return tex;
}

SDL_Texture*
mw::get_render_target(SDL_Renderer *rend) noexcept
{ return SDL_GetRenderTarget(rend); }

void
mw::set_render_target(SDL_Renderer *rend, SDL_Texture *target)
{
  if (SDL_SetRenderTarget(rend, target))
  {
    throw exception {
      (boost::format("failed to set texture as a rendering target (%s)")
       % SDL_GetError()).str()};
  }
}

SDL_Texture*
mw::copy_texture_for_drawing(SDL_Renderer *rend, SDL_Texture *tex,
    uint8_t alpha, int w, int h)
{
  // create new texture
  SDL_Texture *texcopy = create_texture(rend, SDL_TEXTUREACCESS_TARGET, w, h);

  // copy original texture to the new one
  SDL_Texture *oldtarget = get_render_target(rend);
  set_render_target(rend, texcopy);

  SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0x00, 0x00);
  SDL_RenderClear(rend);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_SetTextureAlphaMod(tex, alpha);
  if (SDL_RenderCopy(rend, tex, nullptr, nullptr) < 0)
  {
    error("failed to copy texture (%s)", SDL_GetError());
    abort();
  }
  set_render_target(rend, oldtarget);

  return texcopy;
}

SDL_Texture*
mw::copy_texture_for_drawing(SDL_Renderer *rend, SDL_Texture *tex, uint8_t alpha)
{
  uint32_t format;
  int access, w, h;
  SDL_QueryTexture(tex, &format, &access, &w, &h);
  return copy_texture_for_drawing(rend, tex, alpha, w, h);
}

