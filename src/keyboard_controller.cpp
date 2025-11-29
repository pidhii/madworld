#include "controls/keyboard_controller.hpp"
#include "logging.h"

#include "exceptions.hpp"

mw::pt2d_i
mw::keyboard_controller::get_pointer() const
{ return m_pointer; }

void
mw::keyboard_controller::update()
{
  const uint8_t *kbrdstate = m_sdl.get_keyboard_state();

  m_old_pointer = m_pointer;
  const uint32_t mousestate = SDL_GetMouseState(&m_pointer.x, &m_pointer.y);

  // Keyboard buttons
  for (auto &[_, handle] : m_keyboard_handles)
  {
    const SDL_Scancode key = handle.scancode;
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

  // Simulate analog with keyboard buttons
  for (auto &[_, handle] : m_analog_handles)
  {
    handle.value = !!kbrdstate[handle.scancode_pos]
                 - !!kbrdstate[handle.scancode_neg];
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

const mw::keyboard_controller::key_handle_data&
mw::keyboard_controller::_get_key_handle(std::string_view _key) const
{
  const std::string key {_key}; // would not be needed in C++20
  // lmb
  if (m_lmb_keys.find(key) != m_lmb_keys.end())
    return m_lmb_handle;
  // rmb
  if (m_rmb_keys.find(key) != m_rmb_keys.end())
    return m_rmb_handle;
  // other keys
  const auto it = m_keyboard_handles.find(key);
  if (it == m_keyboard_handles.end())
    throw mw::exception {"request for unregistered key: " + key};
  return it->second;
}

double
mw::keyboard_controller::get_analog(std::string_view _key) const
{
  const std::string key {_key}; // would not be needed in C++20
  const auto it = m_analog_handles.find(key);
  if (it == m_analog_handles.end())
    throw mw::exception {"request for unregistered key: " + key};
  return it->second.value;
}

mw::keyboard_controller::key_modifier
mw::keyboard_controller::key_modifier_from_string(std::string_view s)
{
  if (s == "shift")
    return shift; 
  if (s == "lshift")
    return lshift;
  if (s == "rshift")
    return rshift;
  if (s == "")
    return none;
  throw std::runtime_error {"unknown key_modifier name"};
}

std::string
mw::keyboard_controller::key_modifier_to_string(key_modifier mods)
{
  switch (mods)
  {
    case none: return "";
    case lshift: return "lshift";
    case rshift: return "rshift";
    case shift: return "shift";
  }
  throw std::logic_error {"invalid key_modifier value"};
}

