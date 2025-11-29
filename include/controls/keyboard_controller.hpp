#ifndef CONTROLS_KEYBOARD_CONTROLLER_HPP
#define CONTROLS_KEYBOARD_CONTROLLER_HPP

#include "controls/controller.hpp"

#include "sdl_environment.hpp"

#include <unordered_map>
#include <set>
#include <optional>

#include <SDL2/SDL.h>


namespace mw {

/** Controller interface for keyboard and mouse. */
class keyboard_controller: public controller {
  public:
  keyboard_controller(const sdl_environment &sdl)
  : m_sdl {sdl}
  { }

  enum key_modifier: uint8_t {
    none = 0,
    lshift = 1,
    rshift = 2,
    shift = 3,
  };

  static key_modifier
  key_modifier_from_string(std::string_view s);

  static std::string
  key_modifier_to_string(key_modifier mods);

  void
  make_button(std::string_view key, SDL_Scancode sc, key_modifier mods = none)
  { m_keyboard_handles.emplace(key, keyboard_key_handle_data {sc, mods}); }

  void
  make_left_mouse_button(std::string_view key)
  { m_lmb_keys.emplace(key); }

  void
  make_right_mouse_button(std::string_view key)
  { m_rmb_keys.emplace(key); }

  void
  make_analog(std::string_view key, SDL_Scancode scpos, SDL_Scancode scneg)
  {
    m_analog_handles.emplace(std::piecewise_construct,
                             std::forward_as_tuple(key),
                             std::forward_as_tuple(scpos, scneg));
  }

  void
  update() override;

  pt2d_i
  get_pointer() const override;

  vec2d_i
  get_pointer_shift() const
  { return m_old_pointer - m_pointer; }

  bool
  button_is_down(std::string_view key) const override
  { return _get_key_handle(key).is_down; }

  bool
  button_was_pressed(std::string_view key) const override
  { return _get_key_handle(key).was_pressed; }

  bool
  button_was_released(std::string_view key) const override
  { return _get_key_handle(key).was_released; }

  double
  get_analog(std::string_view key) const override;

  private:
  struct key_handle_data {
    key_handle_data()
    : is_down {false},
      was_pressed {false},
      was_released {false}
    { }

    bool is_down;
    bool was_pressed;
    bool was_released;
  };

  struct keyboard_key_handle_data: public key_handle_data {
    keyboard_key_handle_data(SDL_Scancode scancode, uint8_t mod)
    : key_handle_data(),
      scancode {scancode},
      mod {0}
    { }

    SDL_Scancode scancode;
    uint8_t mod;
  };

  struct simulated_analog_handle_data {
    simulated_analog_handle_data(SDL_Scancode scancode_pos,
                                 SDL_Scancode scancode_neg)
    : scancode_pos {scancode_pos},
      scancode_neg {scancode_neg},
      value {0}
    { }

    SDL_Scancode scancode_pos;
    SDL_Scancode scancode_neg;
    double value;
  };

  const key_handle_data&
  _get_key_handle(std::string_view key) const;

  const sdl_environment &m_sdl;
  pt2d_i m_pointer, m_old_pointer;
  std::unordered_map<std::string, keyboard_key_handle_data> m_keyboard_handles;
  std::unordered_map<std::string, simulated_analog_handle_data> m_analog_handles;
  std::set<std::string> m_lmb_keys, m_rmb_keys;
  key_handle_data m_lmb_handle, m_rmb_handle;
}; // class mw::keyboard_controller

} // namespace mw

#endif
