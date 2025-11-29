#ifndef CONTROLS_CONTROLLER_HPP
#define CONTROLS_CONTROLLER_HPP

#include "geometry.hpp"

#include <string_view>


namespace mw {

struct controller {
  virtual void
  update() = 0;

  /** Get pointer position. */
  virtual pt2d_i
  get_pointer() const = 0;

  /** Check if a key is in a DOWN-state. */
  virtual bool
  button_is_down(std::string_view key) const = 0;

  /** Check if a key was pressed sinsce the last update. */
  virtual bool
  button_was_pressed(std::string_view key) const = 0;

  /** Check if a key was released sisnce the last update. */
  virtual bool
  button_was_released(std::string_view key) const = 0;

  /** Get value of an analog input ranging from [-1, 1]. */
  virtual double
  get_analog(std::string_view key) const = 0;
}; // class mw::controller

} // namespace mw

#endif
