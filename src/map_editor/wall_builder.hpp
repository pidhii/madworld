#ifndef WALL_BUILDER_HPP
#define WALL_BUILDER_HPP

#include "map_editor.hpp"

#include <boost/format.hpp>


class wall_builder: public mw::map_editor::processor {
  public:
  wall_builder(mw::sdl_environment &sdl, mw::area_map &map);

  mw::vertical_layout*
  get_gui() const noexcept
  { return m_gui; }

  void
  mouse_button_down(int button) override
  { if (button == 1) m_lbutton = true; }

  void
  mouse_button_up(int button) override;

  void
  mouse_move_within(const mw::pt2d_d &at) override;

  void
  mouse_move_outside() override
  { m_lbutton = false; }

  void
  key_down(int sym) override;

  void
  key_up(int sym) override;

  void
  visualise() const override;

  private:
  size_t
  _find_closest_vertex(const mw::pt2d_d &p) const;

  private:
  mw::sdl_environment &m_sdl;
  mw::area_map &m_map;
  mw::linear_layout *m_main_layout;

  mw::vertical_layout *m_gui;
  mw::linear_layout *m_gui_vertices;

  mw::sdl_string_factory make_normal_string;
  mw::sdl_string_factory make_button_string;
  mw::sdl_string_factory make_hlbutton_string;

  std::vector<mw::pt2d_d> m_vertices;
  std::optional<mw::pt2d_d> m_last_mouse_pos;
  bool m_shift;
  bool m_lbutton;
};



#endif
