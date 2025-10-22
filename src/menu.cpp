#include "gui/menu.hpp"
#include "color_manager.hpp"
#include "ui_manager.hpp"
#include "video_manager.hpp"

#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_render.h>



mw::gui::basic_menu::basic_menu(mw::sdl_environment &sdl, component *c)
: ui_layer(ui_layer::size::screen_portion),
  m_sdl {sdl},
  m_leftpad {10},
  m_rightpad {10},
  m_toppad {10},
  m_botpad {10},
  m_box_color {color_manager::instance()["BoxBackground"]},
  m_border_color {color_manager::instance()["BoxBorder"]},
  m_c {c},
  m_is_hovering {false},
  m_pending_close {false}
{ }

int
mw::gui::basic_menu::get_width() const noexcept
{ return m_c->get_width() + m_leftpad + m_rightpad; }

int
mw::gui::basic_menu::get_height() const noexcept
{ return m_c->get_height() + m_toppad + m_botpad; }

inline void
mw::gui::basic_menu::_get_box(int16_t xs[4], int16_t ys[4]) const noexcept
{
  const int16_t w = get_width();
  const int16_t h = get_height();

  int winw, winh;
  SDL_GetWindowSize(m_sdl.get_window(), &winw, &winh);

  int16_t startx = (winw - w)/2;
  int16_t starty = (winh - h)/2;
  if (m_hor_align.has_value())
  {
    switch (m_hor_align.value())
    {
      case alignment::start: startx = 0; break;
      case alignment::center: startx = (winw - w)/2; break;
      case alignment::end: startx = winw - w; break;
    }
    switch (m_vert_align.value())
    {
      case alignment::start: starty = 0; break;
      case alignment::center: starty = (winh - h)/2; break;
      case alignment::end: starty = winh - h; break;
    }
  }

  xs[0] = startx;
  xs[1] = startx + w;
  xs[2] = startx + w;
  xs[3] = startx;
  ys[0] = starty;
  ys[1] = starty;
  ys[2] = starty + h;
  ys[3] = starty + h;
}

void
mw::gui::basic_menu::draw() const
{
  SDL_Renderer *rend = m_sdl.get_renderer();
  SDL_Window *win = m_sdl.get_window();

  _update_box();

  SDL_Rect clip;
  clip.x = m_box.xs[0];
  clip.y = m_box.ys[0];
  clip.w = get_width();
  clip.h = get_height();
  const uint8_t
    r = m_box_color >>  0 & 0xFF,
    g = m_box_color >>  8 & 0xFF,
    b = m_box_color >> 16 & 0xFF,
    a = m_box_color >> 24 & 0xFF;
  SDL_BlendMode oldblend;
  if (SDL_GetRenderDrawBlendMode(rend, &oldblend))
    warning("%s", SDL_GetError());
  if (SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE))
    warning("%s", SDL_GetError());
  if (SDL_SetRenderDrawColor(rend, r, g, b, a))
    warning("%s", SDL_GetError());
  if (SDL_RenderFillRect(rend, &clip))
    warning("%s", SDL_GetError());
  if (SDL_SetRenderDrawBlendMode(rend, oldblend))
    warning("%s", SDL_GetError());
  aapolygonColor(rend, m_box.xs, m_box.ys, 4, m_border_color);
  m_c->draw({m_box.xs[0]+m_leftpad, m_box.ys[0]+m_toppad});
}

void
mw::gui::basic_menu::run_layer(mw::ui_manager &uiman)
{
  SDL_Renderer *rend = m_sdl.get_renderer();
  SDL_Window *win = m_sdl.get_window();
  const uint8_t *kbrd = m_sdl.get_keyboard_state();

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      case SDL_WINDOWEVENT:
        {
          // refresh whole stack of layers
          uiman.draw(get_id());
          break;
        }

      case SDL_MOUSEMOTION:
        {
          int x, y;
          SDL_GetMouseState(&x, &y);
          const pt2d_i cpos {m_box.xs[0] + m_leftpad, m_box.ys[0] + m_toppad};
          if (m_box.xs[0] + m_leftpad < x and x < m_box.xs[1] + m_rightpad and
              m_box.ys[0] + m_toppad  < y and y < m_box.ys[3] + m_botpad)
          {
            if (m_is_hovering)
            {
              m_c->send("hover", std::any(pt2d_i(x, y) - cpos));
            }
            else
            {
              m_is_hovering = true;
              m_c->send("hover-begin", std::any(pt2d_i(x, y) - cpos));
            }
          }
          else
          {
            if (m_is_hovering)
            {
              m_is_hovering = false;
              m_c->send("hover-end", 0);
            }
          }
          break;
        }

      case SDL_MOUSEBUTTONUP:
        if (event.button.button == 1 and m_is_hovering)
        {
          m_c->send("clicked", 0);
        }
        break;

      case SDL_KEYDOWN:
        if (m_kbrd_receiver.has_value())
        {
          m_kbrd_receiver.value()->send("keydown", event.key.keysym.sym);
        }
        break;
    }
  }

  if (m_pending_close)
  {
    uiman.remove_layer(get_id());
    m_pending_close = false;
    return;
  }

  if (fps_guardian(SDL_GetTicks()))
  {
    uiman.draw(get_id());
    SDL_RenderPresent(rend);
    fps_guardian.notify();
  }
}

