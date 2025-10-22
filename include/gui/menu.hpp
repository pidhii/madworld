#ifndef GUI_MENU_HPP
#define GUI_MENU_HPP

#include "ui_layer.hpp"
#include "gui/components.hpp"
#include "utl/frequency_limiter.hpp"
#include "video_manager.hpp"

#include <optional>


namespace mw {
inline namespace gui {

class basic_menu: public mw::ui_layer {
  enum class alignment {start, center, end};

  public:
  basic_menu(sdl_environment &sdl, component *c);
  basic_menu(component *c): basic_menu {video_manager::instance().get_sdl(), c} { }

  virtual ~basic_menu() { delete m_c; }

  void set_box_color(color_t color) noexcept { m_box_color = color; }

  void set_left_padding(int pad) noexcept { m_leftpad = pad; }
  void set_right_padding(int pad) noexcept { m_rightpad = pad; }
  void set_top_padding(int pad) noexcept { m_toppad = pad; }
  void set_bottom_padding(int pad) noexcept { m_botpad = pad; }

  void align_left() noexcept { m_hor_align = alignment::start; }
  void align_right() noexcept { m_hor_align = alignment::end; }
  void align_top() noexcept { m_vert_align = alignment::start; }
  void align_bottom() noexcept { m_vert_align = alignment::end; }

  int get_width() const noexcept;
  int get_height() const noexcept;

  component*
  get_contents() const noexcept
  { return m_c; }

  void
  attach_keyboard_receiver(component *c) noexcept
  { m_kbrd_receiver = c; }

  void
  detach_keyboard_receiver() noexcept
  { m_kbrd_receiver = std::nullopt; }

  void
  close() noexcept
  { m_pending_close = true; }

  virtual void
  draw() const override;

  virtual void
  run_layer(ui_manager &uiman) override;

  private:
  void
  _get_box(int16_t xs[4], int16_t ys[4]) const noexcept;

  void
  _update_box() const noexcept
  { _get_box(m_box.xs, m_box.ys); }

  private:
  sdl_environment &m_sdl;
  int m_leftpad, m_rightpad, m_toppad, m_botpad;
  color_t m_box_color, m_border_color;
  mutable struct { int16_t xs[4], ys[4]; } m_box;
  component *m_c;
  bool m_is_hovering;
  std::optional<component*> m_kbrd_receiver;

  std::optional<alignment> m_hor_align;
  std::optional<alignment> m_vert_align;

  bool m_pending_close;
}; // class mw::gui::basic_menu

} // namespace mw::gui
} // namespace mw


#endif
