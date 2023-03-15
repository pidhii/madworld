#include "logging.h"
#include "map_generation.hpp"
#include "exceptions.hpp"
#include "gui/components.hpp"
#include "gui/composer.hpp"
#include "gui/menu.hpp"
#include "ui_manager.hpp"


class map_generator_gui: public mw::component {
  public:
  map_generator_gui(mw::sdl_environment &sdl, mw::map_generator &mapgen,
      mw::mapping &viewport)
  : mw::component(sdl),
    m_mapgen {mapgen},
    m_viewport {viewport}
  { }

  void
  start_room_overlay(const mw::carbon_grid &room) noexcept
  { m_ovl_room = room; }

  void
  remove_room_overlay() noexcept
  { m_ovl_room = std::nullopt; }

  void
  write_room()
  {
    if (m_ovl_room.has_value())
    {
      try {
        m_mapgen.write(m_ovl_room.value());
      } catch (const mw::exception &exn) {
        error("%s", exn.what());
      }
    }
    else
      warning("no overlay room to write");
  }

  int
  get_width() const override
  {
    const mw::carbon_grid &cg = m_mapgen.get_main_grid();
    int xbegin, xend;
    cg.get_x_range(xbegin, xend);
    return (xend - xbegin)*m_viewport.get_x_scale();
  }

  int
  get_height() const override
  {
    const mw::carbon_grid &cg = m_mapgen.get_main_grid();
    int ybegin, yend;
    cg.get_y_range(ybegin, yend);
    return (yend - ybegin)*m_viewport.get_y_scale();
  }

  void
  draw(const mw::pt2d_i &at) const override
  {
    const mw::mapping viewport {
      mw::vec2d_d(mw::to_vec(at)) + m_viewport.get_offset(),
      m_viewport.get_x_scale(),
      m_viewport.get_y_scale()
    };
    mw::draw_carbon_grid(m_sdl, viewport, m_mapgen.get_main_grid());
    if (m_ovl_room.has_value())
      mw::draw_carbon_grid(m_sdl, viewport, m_ovl_room.value());
  }

  int
  send(const std::string &what, mw::signal_data data) override
  {
    if (what == "hover-begin" or what == "hover")
      m_mousepos = mw::unpack_hover(data.u64);
    else if (what == "hover-end")
      m_mousepos = std::nullopt;
    else if (what == "keydown")
    {
      if (m_ovl_room.has_value())
      {
        switch (data.u64)
        {
          case SDLK_UP: m_ovl_room.value().shift({0, -1}); break;
          case SDLK_DOWN: m_ovl_room.value().shift({0, +1}); break;
          case SDLK_LEFT: m_ovl_room.value().shift({-1, 0}); break;
          case SDLK_RIGHT: m_ovl_room.value().shift({+1, 0}); break;
          case SDLK_r:
          {
            int xbegin, xend, ybegin, yend;
            m_ovl_room.value().get_x_range(xbegin, xend);
            m_ovl_room.value().get_y_range(ybegin, yend);
            const mw::vec2d_i fix {(xend + xbegin)/2, (yend + ybegin)/2};
            m_ovl_room.value().rotate_right(fix);
            break;
          }
          case SDLK_l:
          {
            int xbegin, xend, ybegin, yend;
            m_ovl_room.value().get_x_range(xbegin, xend);
            m_ovl_room.value().get_y_range(ybegin, yend);
            const mw::vec2d_i fix {(xend + xbegin)/2, (yend + ybegin)/2};
            m_ovl_room.value().rotate_left(fix);
            break;
          }
          case SDLK_RETURN:
            write_room();
            break;
        }
      }
    }
    return 0;
  }

  private:
  mw::map_generator &m_mapgen;
  mw::mapping &m_viewport;
  std::optional<mw::vec2d_i> m_mousepos;
  std::optional<mw::carbon_grid> m_ovl_room;
};


static int
the_main(int argc, char **argv)
{
  mw::video_manager& vman = mw::video_manager::instance();
  vman.init();

  mw::sdl_environment &sdl = vman.get_sdl();


  mw::map_generator mapgen {50, 50};

  const int n = 9;
  mw::carbon_grid room {n, n};
  for (int y = 1; y < n-1; ++y)
  {
    int8_t c;

    c = room.get({1, y});
    room.set({1, y}, c | mw::wall | mw::down);

    c = room.get({n-1-1, y});
    room.set({n-1-1, y}, c | mw::wall | mw::up);
  }
  for (int x = 1; x < n-1; ++x)
  {
    int8_t c;

    c = room.get({x, 1});
    room.set({x, 1}, c | mw::wall | mw::left);

    c = room.get({x, n-1-1});
    room.set({x, n-1-1}, c | mw::wall | mw::right);
  }

  room.set({0, 3}, mw::wall | mw::left);
  room.set({1, 3}, mw::wall | mw::down | mw::left);
  room.set({0, 4}, mw::doorway);
  room.set({1, 4}, mw::cell_value::nothing);
  room.set({0, 5}, mw::wall | mw::right);
  room.set({1, 5}, mw::wall | mw::down | mw::right);

  room.rotate_left({4, 4});
  room.set({0, 3}, mw::wall | mw::left);
  room.set({1, 3}, mw::wall | mw::down | mw::left);
  room.set({0, 4}, mw::doorway);
  room.set({1, 4}, mw::cell_value::nothing);
  room.set({0, 5}, mw::wall | mw::right);
  room.set({1, 5}, mw::wall | mw::down | mw::right);

  room.rotate_left({4, 4});
  room.set({0, 3}, mw::wall | mw::left);
  room.set({1, 3}, mw::wall | mw::down | mw::left);
  room.set({0, 4}, mw::doorway);
  room.set({1, 4}, mw::cell_value::nothing);
  room.set({0, 5}, mw::wall | mw::right);
  room.set({1, 5}, mw::wall | mw::down | mw::right);

  mw::mapping viewport {{0, 0}, 20, 20};
  mw::composer ctlcomp {sdl, vman.get_font()};

  map_generator_gui *mapgengui = new map_generator_gui {sdl, mapgen, viewport};

  mw::linear_layout *ctllayout = new mw::vertical_layout {sdl};
  mw::button *addroombutton = ctlcomp.make_button("[add room]");
  ctllayout->add_component(addroombutton);
  mw::button *quitbutton = ctlcomp.make_button("[quit]");
  ctllayout->add_component(quitbutton);

  mw::linear_layout *toplayout = new mw::horisontal_layout {sdl};
  toplayout->add_component(ctllayout);
  toplayout->add_component(mapgengui);

  mw::basic_menu *mainmenu = new mw::basic_menu {sdl, toplayout};
  mainmenu->align_top();
  mainmenu->align_left();
  mainmenu->attach_keyboard_receiver(mapgengui);

  addroombutton->on("clicked", [&] (MWGUI_CALLBACK_ARGS) -> int {
      mapgengui->start_room_overlay(room);
      return 0;
  });
  quitbutton->on("clicked", [&] (MWGUI_CALLBACK_ARGS) -> int {
      mainmenu->close();
      return 0;
  });

  mw::ui_manager uiman {sdl};
  uiman.add_layer(mainmenu);
  uiman.run();

  return 0;
}

int
main(int argc, char **argv)
{
  eth::init(&argc);
  int ret = the_main(argc, argv);
  eth::cleanup();
  return ret;
}
