#include "./wall_builder.hpp"

#include "gui/sdl_string.hpp"
#include "video_manager.hpp"
#include "color_manager.hpp"
#include "walls.hpp"


wall_builder::wall_builder(mw::sdl_environment &sdl, mw::area_map &map)
: m_sdl {sdl},
  m_map {map},
  m_gui {nullptr}, m_gui_vertices {nullptr},
  make_normal_string {mw::video_config::instance().font.path},
  make_button_string {mw::video_config::instance().font.path},
  make_hlbutton_string {mw::video_config::instance().font.path},
  m_shift {false}, m_lbutton {false}
{
  const mw::color_manager &cman = mw::color_manager::instance();
  const mw::video_config &vcfg = mw::video_config::instance();

  const int point_size = vcfg.font.point_size;
  const int tab_size = point_size/2*2;

  make_normal_string.set_fg_color(cman["Normal"]);
  make_button_string.set_fg_color(cman["InteractiveText"]);
  make_hlbutton_string.set_fg_color(cman["HighlightedInteractiveText"]);
  make_hlbutton_string.set_style(TTF_STYLE_BOLD);

  auto on_hover_begin = [=] (MWGUI_CALLBACK_ARGS) {
    self->set_string(make_hlbutton_string(self->get_string().get_text()));
    return 0;
  };
  auto on_hover_end = [=] (MWGUI_CALLBACK_ARGS) {
    self->set_string(make_button_string(self->get_string().get_text()));
    return 0;
  };


  m_gui = new mw::vertical_layout {sdl};
  m_gui
    ->set_left_margin(10)
    ->set_right_margin(10)
    ->set_top_margin(10)
    ->set_bottom_margin(10)
    ->set_fill_color(0xCC000000);

  mw::label *vertices_label =
    new mw::label {sdl, make_normal_string("vertices:")};
  m_gui->add_component(vertices_label);

  m_gui_vertices = new mw::vertical_layout {sdl};
  m_gui->add_component(m_gui_vertices);

  mw::button *build_button = new mw::button {
    sdl,
    new mw::label {sdl, make_button_string("Build")},
    new mw::label {sdl, make_hlbutton_string("Build")},
  };
  build_button->set_hover(false);
  build_button->ignore_signals(true);
  build_button->on("clicked", [=] (MWGUI_CALLBACK_ARGS) {
    // create walls
    mw::basic_wall<>* wall = new mw::basic_wall<> {m_vertices};
    const mw::object_id wallid = m_map.add_static_object(wall);
    m_map.register_phys_obstacle(wallid);
    m_map.register_vis_obstacle(wallid);
    // clear vertices and reset button state
    m_vertices.clear();
    m_gui_vertices->clear();
    self->set_hover(false);
    self->ignore_signals(true);
    return 0;
  });
  m_gui->add_component(build_button);
  // build_button is only enabled when there are vertices in the list
  m_gui_vertices->on("<wall_builder>:add-vertex", [=] (MWGUI_CALLBACK_ARGS) {
    info("N vertices: %zu", m_vertices.size());
    if (m_vertices.size() >= 2)
    {
      info("enable 'Build'-button");
      build_button->ignore_signals(false);
    }
    return 0;
  });
  m_gui_vertices->on("<wall_builder>:remove-vertex", [=] (MWGUI_CALLBACK_ARGS) {
    if (m_vertices.size() < 2)
    {
      build_button->set_hover(false);
      build_button->ignore_signals(true);
    }
    return 0;
  });

  m_gui->add_component(new mw::padding {sdl, 0, point_size});
  mw::label *back_button = new mw::label {sdl, make_button_string("Back")};
  back_button
    ->on("hover-begin", on_hover_begin)
    ->on("hover-end", on_hover_end)
    ->on("clicked", [this] (MWGUI_CALLBACK_ARGS) {
      m_gui->send("<wall_builder>:finish", 0);
      return 0;
    });
  m_gui->add_component(back_button);
}

void
wall_builder::mouse_button_up(int button)
{
  if (button == 1)
  {
    if (m_lbutton and m_last_mouse_pos.has_value())
    {
      boost::format fmt {"%2d - (%7.3f, %7.3f)"};
      const int idx = m_vertices.size();
      const double x = m_last_mouse_pos.value().x;
      const double y = m_last_mouse_pos.value().y;
      m_gui_vertices->add_component(new mw::label {m_sdl,
        make_normal_string((fmt % idx % x % y).str())
      });
      m_vertices.push_back(m_last_mouse_pos.value());
      m_gui_vertices->send("<wall_builder>:add-vertex", 0);
    }
    m_lbutton = false;
  }
}

void
wall_builder::mouse_move_within(const mw::pt2d_d &at)
{
  if (not m_vertices.empty() and m_lbutton and m_shift)
  {
    const mw::pt2d_i pixat = m_map.point_to_pixels(at);
    const mw::pt2d_d vtx = m_vertices[_find_closest_vertex(at)];
    const mw::pt2d_i pixvtx = m_map.point_to_pixels(vtx);
    if (mag(pixat - pixvtx) <= 7)
    {
      SDL_WarpMouseInWindow(m_sdl.get_window(), pixvtx.x, pixvtx.y);
      m_last_mouse_pos = vtx;
      return;
    }
  }

  m_last_mouse_pos = at;
}

void
wall_builder::key_down(int sym)
{
  switch (sym)
  {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      m_shift = true;
      break;

    case SDLK_BACKSPACE:
      if (not m_vertices.empty())
      {
        m_gui_vertices->remove_entry(m_gui_vertices->get_last_entry());
        m_gui_vertices->send("<wall_builder>:remove-vertex", 0);
        m_vertices.pop_back();
      }
      break;
  }
}

void
wall_builder::key_up(int sym)
{
  switch (sym)
  {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      m_shift = false;
      break;
  }
}

void
wall_builder::visualise() const
{
  for (size_t j = 1; j < m_vertices.size(); ++j)
  {
    const size_t i = j - 1;
    const mw::pt2d_i &a = m_map.point_to_pixels(m_vertices[i]);
    const mw::pt2d_i &b = m_map.point_to_pixels(m_vertices[j]);
    aalineColor(m_sdl.get_renderer(), a.x, a.y, b.x, b.y, 0xFFFFFFFF);
  }

  // highlight latest vertex
  if (not m_vertices.empty())
  {
    const mw::pt2d_i &a = m_map.point_to_pixels(m_vertices.back());
    aacircleColor(m_sdl.get_renderer(), a.x, a.y, 5, 0xFFF987AF);
  }

  if (m_lbutton and m_last_mouse_pos.has_value())
  {
    // show future wall
    if (not m_vertices.empty())
    {
      const mw::pt2d_i &a = m_map.point_to_pixels(m_vertices.back());
      const mw::pt2d_i &b = m_map.point_to_pixels(m_last_mouse_pos.value());
      aalineColor(m_sdl.get_renderer(), a.x, a.y, b.x, b.y, 0xFF888888);
    }

    // show auto-join-raidus
    if (m_shift and not m_vertices.empty())
    {
      const mw::pt2d_i a = m_map.point_to_pixels(m_last_mouse_pos.value());
      aacircleColor(m_sdl.get_renderer(), a.x, a.y, 7, 0xFF9763FF);

      const mw::pt2d_d &p =
        m_vertices[_find_closest_vertex(m_last_mouse_pos.value())];
      const mw::pt2d_i b = m_map.point_to_pixels(p);
      aacircleColor(m_sdl.get_renderer(), b.x, b.y, 7, 0xFF9763FF);
    }
  }
}

size_t
wall_builder::_find_closest_vertex(const mw::pt2d_d &p) const
{
  double minr2 = DBL_MAX;
  int minidx = -1;
  for (size_t i = 0; i < m_vertices.size(); ++i)
  {
    const double r2 = mag2(p - m_vertices[i]);
    if (r2 < minr2)
    {
      minr2 = r2;
      minidx = i;
    }
  }
  if (minidx < 0)
    throw std::logic_error {"m_vertices is empty"};
  return minidx;
}

