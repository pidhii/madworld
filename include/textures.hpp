#ifndef TEXTURES_HPP
#define TEXTURES_HPP

#include "exceptions.hpp"

#include <SDL2/SDL.h>

#include <unordered_map>
#include <string>


namespace mw {

class texture_storage {
  public:
  static constexpr char class_name[] = "mw::texture_storage";
  using exception = scoped_exception<class_name>;

  struct entry {
    entry(SDL_Surface *surf, SDL_Texture *tex): surf {surf}, tex {tex} { }
    SDL_Surface *surf;
    SDL_Texture *tex;
  };

  texture_storage(SDL_Renderer *rend): m_rend {rend} { }

  ~texture_storage();

  const entry&
  load(const std::string &name, const std::string &path);

  const entry&
  operator [] (const std::string &name) const;

  texture_storage(const texture_storage&) = delete;
  texture_storage(texture_storage&&) = delete;
  texture_storage& operator = (const texture_storage&) = delete;
  texture_storage& operator = (texture_storage&&) = delete;

  private:
  SDL_Renderer *m_rend;
  std::unordered_map<std::string, entry> m_storage;
}; // class mw::texture_storage

SDL_Texture*
create_texture(SDL_Renderer *rend, int access, int w, int h);

SDL_Texture*
get_render_target(SDL_Renderer *rend) noexcept;

void
set_render_target(SDL_Renderer *rend, SDL_Texture *target);

SDL_Texture*
copy_texture_for_drawing(SDL_Renderer *rend, SDL_Texture *tex, uint8_t alpha,
    int w, int h);

SDL_Texture*
copy_texture_for_drawing(SDL_Renderer *rend, SDL_Texture *tex, uint8_t alpha);

} // namespace mw

#endif
