#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include "area_map.hpp"
#include "sdl_environment.hpp"
#include "player.hpp"
#include "ui_layer.hpp"
#include "hud.hpp"
#include "utl/frequency_limiter.hpp"
#include "controls/controller.hpp"

#include <SDL2/SDL.h>

#include <boost/optional.hpp>


namespace mw {

class game_manager: public ui_layer {
  public:
  game_manager(sdl_environment &sdl, area_map &map, controller &input)
  : ui_layer(ui_layer::size::whole_screen),
    m_sdl {sdl},
    m_map {map},
    m_tick_limiter {std::chrono::milliseconds {10}},
    m_input {input}
  { }

  void
  set_player(player &p, double vision_radius) noexcept
  {
    m_player = p;
    m_player_vision_radius = vision_radius;
  }

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

  private:
  void
  _handle_zoom_keys(const pt2d_i &at, time_t dt);

  void
  _handle_map_movement_keys(time_t dt);

  private:
  sdl_environment &m_sdl;
  area_map &m_map;
  boost::optional<player&> m_player;
  double m_player_vision_radius;
  std::optional<time_t> m_prev_time;
  frequency_limiter m_tick_limiter;

  bool m_center_on_player = true;
  bool m_fow_enabled = true;
  controller &m_input;


  heads_up_display m_hud;
  mutable hud_footprint m_hud_footprint;
}; // class game_manager

} // namespace mw

#endif
