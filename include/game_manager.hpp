#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include "area_map.hpp"
#include "sdl_environment.hpp"
#include "player.hpp"
#include "ui_layer.hpp"
#include "textures.hpp"
#include "hud.hpp"
#include "utl/frequency_limiter.hpp"

#include <SDL2/SDL.h>

#include <random>
#include <boost/optional.hpp>


namespace mw {

class input_proxy {
  public:
  input_proxy(const sdl_environment &sdl);

  void
  init_key(SDL_Scancode key);

  void
  update(pt2d_i &mousepos);

  void
  update()
  { pt2d_i _; update(_); }

  bool
  is_down(SDL_Scancode key) const;

  bool
  was_pressed(SDL_Scancode key) const;

  bool
  was_released(SDL_Scancode key) const;

  bool
  is_shift_down() const
  { return is_down(SDL_SCANCODE_LSHIFT) or is_down(SDL_SCANCODE_RSHIFT); }

  bool
  is_lmb_down() const noexcept
  { return m_lmb_handle.is_down; }

  bool
  was_lmb_pressed() const noexcept
  { return m_lmb_handle.was_pressed; }

  bool
  was_lmb_released() const noexcept
  { return m_lmb_handle.was_released; }

  bool
  is_rmb_down() const noexcept
  { return m_rmb_handle.is_down; }

  bool
  was_rmb_pressed() const noexcept
  { return m_rmb_handle.was_pressed; }

  bool
  was_rmb_released() const noexcept
  { return m_rmb_handle.was_released; }

  private:
  struct key_handle_data {
    key_handle_data()
    : is_down {false}, was_pressed {false}, was_released {false}
    { }
    bool is_down;
    bool was_pressed;
    bool was_released;
  };

  const sdl_environment &m_sdl;
  std::unordered_map<SDL_Scancode, key_handle_data> m_handles;
  key_handle_data m_lmb_handle, m_rmb_handle;
};


class game_manager: public ui_layer {
  public:
  game_manager(sdl_environment &sdl, area_map &map)
  : ui_layer(ui_layer::size::whole_screen),
    m_sdl {sdl},
    m_map {map},
    m_tick_limiter {std::chrono::milliseconds {10}},
    m_input {sdl}
  {
    m_input.init_key(SDL_SCANCODE_LSHIFT);
    m_input.init_key(SDL_SCANCODE_RSHIFT);
    m_input.init_key(SDL_SCANCODE_EQUALS);
    m_input.init_key(SDL_SCANCODE_MINUS);
    m_input.init_key(SDL_SCANCODE_LEFT);
    m_input.init_key(SDL_SCANCODE_RIGHT);
    m_input.init_key(SDL_SCANCODE_UP);
    m_input.init_key(SDL_SCANCODE_DOWN);
    m_input.init_key(SDL_SCANCODE_ESCAPE);
    m_input.init_key(SDL_SCANCODE_A);
    m_input.init_key(SDL_SCANCODE_D);
    m_input.init_key(SDL_SCANCODE_W);
    m_input.init_key(SDL_SCANCODE_S);
    m_input.init_key(SDL_SCANCODE_SPACE);
    m_input.init_key(SDL_SCANCODE_GRAVE);
  }

  void
  set_player(player &p) noexcept
  { m_player = p; }

  heads_up_display&
  hud() noexcept
  { return m_hud; }

  void
  run_game(boost::optional<ui_manager&> uiman = boost::none);

  bool
  is_fow_enabled() const noexcept
  { return m_fow_enabled; }

  void
  set_fow(bool enable) noexcept
  { m_fow_enabled = enable; }

  void
  draw() const override;

  void
  run_layer(ui_manager &uiman) override
  { return run_game(uiman); }

  void
  destroy_layer() override
  { delete this; }

  private:
  void
  _handle_zoom_keys(const pt2d_i &at, time_t dt);

  void
  _handle_map_movement_keys(time_t dt);

  private:
  sdl_environment &m_sdl;
  area_map &m_map;
  boost::optional<player&> m_player;
  std::optional<time_t> m_prev_time;
  frequency_limiter m_tick_limiter;

  bool m_center_on_player = true;
  bool m_fow_enabled = true;
  input_proxy m_input;

  heads_up_display m_hud;
  mutable hud_footprint m_hud_footprint;
}; // class game_manager

} // namespace mw

#endif
