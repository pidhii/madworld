#ifndef SDL_ENVIRONMENT_HPP
#define SDL_ENVIRONMENT_HPP

#include <SDL2/SDL.h>

namespace mw {


// template <typename T, typename Getter, typename Setter>
// class parameter {
//   public:
//   parameter(Getter getter, Setter setter)
//   : m_getter {getter},
//     m_setter {setter}
//   { }

//   operator T () const
//   { return m_getter(); }

//   parameter &
//   operator = (const T &value)
//   { m_setter(value); }

//   private:
//   Getter m_getter;
//   Setter m_setter;
// };


class sdl_environment {
  public:
  SDL_Renderer*
  get_renderer() const noexcept
  { return m_rend; }

  SDL_Window*
  get_window() const noexcept
  { return m_win; }

  const uint8_t*
  get_keyboard_state() const noexcept
  {
    if (m_kbrdstate == nullptr)
      m_kbrdstate = SDL_GetKeyboardState(NULL);
    return m_kbrdstate;
  }

  private:
  explicit
  sdl_environment(SDL_Window *win, SDL_Renderer *rend)
  : m_win {win},
    m_rend {rend},
    m_kbrdstate {SDL_GetKeyboardState(NULL)}
  { }

  sdl_environment()
  : m_win {nullptr}, m_rend {nullptr}, m_kbrdstate {nullptr}
  { }

  private:
  SDL_Window *m_win;
  SDL_Renderer *m_rend;
  mutable const uint8_t *m_kbrdstate;

  friend class video_manager;
};

} // namespace mw

#endif
