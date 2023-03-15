#include "game_manager.hpp"
#include "projectiles.hpp"
#include "ui_manager.hpp"
#include "effects.hpp"
#include "video_manager.hpp"
#include "vision.hpp"
#include "physics.hpp"
#include "utl/grid.hpp"

#include "walls.hpp"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include "gui/sdl_string.hpp"
#include <boost/format.hpp>
#include <map>
#include <sstream>
#include <iomanip>
#include <deque>


mw::input_proxy::input_proxy(const sdl_environment &sdl)
: m_sdl {sdl}
{ }

void
mw::input_proxy::init_key(SDL_Scancode key)
{ m_handles[key]; }

void
mw::input_proxy::update(pt2d_i &mousepos)
{
  const uint8_t *kbrdstate = m_sdl.get_keyboard_state();
  const uint32_t mousestate = SDL_GetMouseState(&mousepos.x, &mousepos.y);

  for (auto &key_handle : m_handles)
  {
    const SDL_Scancode key = key_handle.first;
    key_handle_data &handle = key_handle.second;

    handle.was_pressed = false;
    handle.was_released = false;
    if (kbrdstate[key])
    { // now button is down
      if (not handle.is_down)
        // it used to be up => was just pressed
        handle.was_pressed = true;
      handle.is_down = true;
    }
    else
    { // now button is up
      if (handle.is_down)
        // it used to be down => was just released
        handle.was_released = true;
      handle.is_down = false;
    }
  }

  // LMB
  m_lmb_handle.was_pressed = false;
  m_lmb_handle.was_released = false;
  if (mousestate & SDL_BUTTON_LMASK)
  { // now button is down
    if (not m_lmb_handle.is_down)
      // it used to be up => was just pressed
      m_lmb_handle.was_pressed = true;
    m_lmb_handle.is_down = true;
  }
  else
  { // now button is up
    if (m_lmb_handle.is_down)
      // it used to be down => was just released
      m_lmb_handle.was_released = true;
    m_lmb_handle.is_down = false;
  }

  // RMB
  m_rmb_handle.was_pressed = false;
  m_rmb_handle.was_released = false;
  if (mousestate & SDL_BUTTON_RMASK)
  { // now button is down
    if (not m_rmb_handle.is_down)
      // it used to be up => was just pressed
      m_rmb_handle.was_pressed = true;
    m_rmb_handle.is_down = true;
  }
  else
  { // now button is up
    if (m_rmb_handle.is_down)
      // it used to be down => was just released
      m_rmb_handle.was_released = true;
    m_rmb_handle.is_down = false;
  }
}

bool
mw::input_proxy::is_down(SDL_Scancode key) const
{
  const auto it = m_handles.find(key);
  if (it == m_handles.end())
    throw mw::exception {"request for unregistered key"};
  return it->second.is_down;
}

bool
mw::input_proxy::was_pressed(SDL_Scancode key) const
{
  const auto it = m_handles.find(key);
  if (it == m_handles.end())
    throw mw::exception {"request for unregistered key"};
  return it->second.was_pressed;
}

bool
mw::input_proxy::was_released(SDL_Scancode key) const
{
  const auto it = m_handles.find(key);
  if (it == m_handles.end())
    throw mw::exception {"request for unregistered key"};
  return it->second.was_released;
}


void
mw::game_manager::_handle_zoom_keys(const pt2d_i &at, time_t dt)
{
  const uint8_t *kbrd = m_sdl.get_keyboard_state();
  double newscale = m_map.get_scale();
  if (m_input.is_down(SDL_SCANCODE_EQUALS) and m_input.is_shift_down())
  {
    int w, h;
    SDL_GetWindowSize(m_sdl.get_window(), &w, &h);
    newscale = m_map.get_scale()*(1 + 1e-3*dt);
  }
  else if (m_input.is_down(SDL_SCANCODE_EQUALS))
  {
    int w, h;
    SDL_GetWindowSize(m_sdl.get_window(), &w, &h);
    newscale = 10;
  }
  else if (m_input.is_down(SDL_SCANCODE_MINUS))
  {
    int w, h;
    SDL_GetWindowSize(m_sdl.get_window(), &w, &h);
    newscale = m_map.get_scale()*(1 - 1e-3*dt);
  }

  if (5 <= newscale and newscale <= 50)
    m_map.zoom(at, newscale);
}

void
mw::game_manager::_handle_map_movement_keys(time_t dt)
{
  vec2d_i dir = {0, 0};
  if (m_input.is_down(SDL_SCANCODE_LEFT))  dir.x += 1;
  if (m_input.is_down(SDL_SCANCODE_RIGHT)) dir.x -= 1;
  if (m_input.is_down(SDL_SCANCODE_UP))    dir.y += 1;
  if (m_input.is_down(SDL_SCANCODE_DOWN))  dir.y -= 1;

  if (dir != vec2d_i {0, 0})
  {
    const vec2d_d shift = vec2d_d(dir)*1e-1*m_map.get_scale()*dt;
    const vec2d_d newoffs = m_map.get_offset() + shift;
    m_map.set_offset(newoffs);
    m_center_on_player = false;
  }
}

void
mw::game_manager::run_game(boost::optional<ui_manager&> uiman)
{
  if (not m_prev_time.has_value())
    m_prev_time = SDL_GetTicks();

  SDL_PumpEvents();

  int winw, winh;
  SDL_GetWindowSize(m_sdl.get_window(), &winw, &winh);

  const uint32_t now = SDL_GetTicks();
  const time_t dt = now - m_prev_time.value();
  if (dt == 0)
    return;
  m_prev_time = now;

  // update button states
  mw::pt2d_i mousepos;
  m_input.update(mousepos);

  if (m_input.was_released(SDL_SCANCODE_ESCAPE))
  {
    info("leaving game_manager");
    if (uiman.has_value())
      uiman.value().remove_layer(get_id());
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    return;
  }

  safe_pointer<hud_component> hudptr;
  const double mouse_on_hud = m_hud_footprint.contains(mousepos, hudptr);
  if (mouse_on_hud and hudptr and m_input.was_lmb_pressed())
  {
    info("before click");
    hudptr.get()->click(mousepos);
    info("after click");
  }

  _handle_zoom_keys(mousepos, dt);
  _handle_map_movement_keys(dt);

  if (m_input.was_pressed(SDL_SCANCODE_GRAVE))
    m_fow_enabled = !m_fow_enabled;

  if (m_player.has_value())
  {
    player &player1 = m_player.value();
    vec2d_d movedir = {0, 0};
    if (m_input.is_down(SDL_SCANCODE_A)) movedir.x -= 1;
    if (m_input.is_down(SDL_SCANCODE_D)) movedir.x += 1;
    if (m_input.is_down(SDL_SCANCODE_W)) movedir.y -= 1;
    if (m_input.is_down(SDL_SCANCODE_S)) movedir.y += 1;
    player1.set_move_direction(movedir);

    if (m_input.is_down(SDL_SCANCODE_SPACE))
      m_center_on_player = true;

    if (not mouse_on_hud and m_input.is_lmb_down())
    {
      const pt2d_d mp = m_map.pixels_to_point(mousepos);
      player1.get_ability(ability_slot::lmb).activate(m_map, player1, mp);
    }
  }

  std::chrono::milliseconds nticks;
  if (m_tick_limiter(nticks))
  {
    md_physics physproc {double(nticks.count())};
    m_map.tick(physproc, nticks.count());
  }

  m_hud.update();

  if (fps_guardian(SDL_GetTicks()))
  {
    if (m_player.has_value() and m_center_on_player)
      m_map.adjust_offset(m_player.value().get_position(), {winw/2, winh/2});

    if (uiman.has_value())
      uiman.value().draw(get_id());
    else
    {
      SDL_SetRenderDrawColor(m_sdl.get_renderer(), 0x00, 0x00, 0x00, 0);
      SDL_RenderClear(m_sdl.get_renderer());
      draw();
    }

    SDL_RenderPresent(m_sdl.get_renderer());
    fps_guardian.notify();
  }
}

void
mw::game_manager::draw() const
{
  if (m_player.has_value())
  {
    m_player.value().draw(m_map);

    if (m_fow_enabled)
    {
      SDL_Renderer *rend = m_sdl.get_renderer();

      const pt2d_d playerpos = m_player.value().get_position();

      const double localvisradius = 20;
      vision_processor localvision {{playerpos, localvisradius}};
      localvision.set_ignore(&m_player.value());
      localvision.load_obstacles(m_map.get_vis_obstacles());
      localvision.process();

      const double globalvisradius = std::max(m_map.get_width(), m_map.get_height())/2;
      vision_processor globalvision {{playerpos, globalvisradius}};
      globalvision.set_ignore(&m_player.value());
      globalvision.load_obstacles(m_map.get_vis_obstacles());
      globalvision.process();

      m_map.draw_visible(localvision, globalvision);
    }
    else
      m_map.draw_all();
  }
  else
    m_map.draw_all();

  m_map.draw_messages();

  // minimap
  int winw, winh;
  SDL_GetWindowSize(m_sdl.get_window(), &winw, &winh);
  const double w2h_ratio = m_map.get_width()/m_map.get_height();
  const int miniw = winw*0.2;
  const int minih = miniw/w2h_ratio;
  const pt2d_i minipos {winw - miniw - 1, winh - minih -1};
  const mapping oldview = m_map.get_view();
  m_map.adjust_to_box_w(minipos, miniw);
  m_map.draw_obstacles();
  m_map.set_view(oldview);

  // GUI
  m_hud_footprint.clear();
  m_hud.draw(m_hud_footprint);
}

