#include "map_editor.hpp"
#include "color_manager.hpp"
#include "video_manager.hpp"
#include "ui_manager.hpp"

#include "./wall_builder.hpp"

mw::map_editor::map_editor(sdl_environment &sdl, area_map &map)
: ui_layer(size::whole_screen),
  m_sdl {sdl},
  m_map {map},
  m_gui {_build_gui()},
  m_processor {nullptr},
  m_pending_exit {false}
{ }

mw::component*
mw::map_editor::_build_gui()
{
  TTF_Font *font = video_manager::instance().get_font();
  const color_manager &cman = color_manager::instance();

  sdl_string_factory make_label_string {font, cman["InteractiveText"]};
  sdl_string_factory make_hl_label_string {font, cman["HighlightedInteractiveText"]};
  make_hl_label_string.set_style(TTF_STYLE_BOLD);

  sdl_string_factory make_normal_string {font, cman["Normal"]};
  sdl_string_factory make_boolean_string {font, cman["Boolean"]};
  sdl_string_factory make_string_string {font, cman["String"]};
  make_string_string.set_bg_color(0x77101010);

  auto on_hover_begin = [=] (MWGUI_CALLBACK_ARGS) {
    self->set_string(make_hl_label_string(self->get_string().get_text()));
    return 0;
  };
  auto on_hover_end = [=] (MWGUI_CALLBACK_ARGS) {
    self->set_string(make_label_string(self->get_string().get_text()));
    return 0;
  };

  const int point_size =
    video_config::instance().get_config().font.point_size;

  typedef std::vector<label*> buttons_type;
  std::shared_ptr<buttons_type> buttons = std::make_shared<buttons_type>();

  vertical_layout *gui = new vertical_layout {m_sdl};
  gui
    ->set_fill_color(cman["BoxBackground"])
    ->set_border_color(cman["BoxBorder"])
    ->set_left_margin(10)
    ->set_right_margin(10)
    ->set_top_margin(10)
    ->set_bottom_margin(10)
    ->set_min_width(point_size/2 * 30)
    ->set_min_height(point_size * 40);

  label *basic_wall_label = new label {m_sdl, make_label_string("Build walls")};
  basic_wall_label
    ->on("hover-begin", on_hover_begin)
    ->on("hover-end", on_hover_end)
    ->on("clicked", [this, gui, font, buttons] (MWGUI_CALLBACK_ARGS) {
      // launch wall-builder
      info("launching a wall-builder");
      wall_builder *wb = new wall_builder {m_sdl, m_map};
      vertical_layout *wbgui = wb->get_gui();
      const auto my_id = gui->find_component(self);
      const auto wbgui_id = gui->add_component_after(my_id, wbgui);
      auto basic_wall_label = self;
      wbgui->on("<wall_builder>:finish", [=] (MWGUI_CALLBACK_ARGS) {
        gui->remove_entry(wbgui_id);
        delete m_processor;
        m_processor = nullptr;
        // unlock state of all the buttons (enable signals)
        for (label *button : *buttons)
          button->ignore_signals(false);
        basic_wall_label->send("hover-end", 0);
        return 0;
      });
      m_processor = wb;
      // lock state of all the buttons (disable signals)
      for (label *button : *buttons)
        button->ignore_signals(true);
      return 0;
    });
  gui->add_component(basic_wall_label);
  buttons->push_back(basic_wall_label);

  label *blablabla_label =
    new label {m_sdl, make_label_string("Bla-bla-bla")};
  blablabla_label
    ->on("hover-begin", on_hover_begin)
    ->on("hover-end", on_hover_end);
  gui->add_component(blablabla_label);
  buttons->push_back(blablabla_label);

  gui->add_component(new mw::padding {m_sdl, 0, point_size});

  label *exit_label = new label {m_sdl, make_label_string("Exit")};
  exit_label
    ->on("hover-begin", on_hover_begin)
    ->on("hover-end", on_hover_end)
    ->on("clicked", [this] (MWGUI_CALLBACK_ARGS) {
      m_pending_exit = true;
      return 0;
    });
  gui->add_component(exit_label);
  buttons->push_back(exit_label);

  return gui;
}

void
mw::map_editor::_fit_map_to_screen(pt2d_i &at, int &w, int &h) const noexcept
{
  int winw, winh;
  SDL_GetWindowSize(m_sdl.get_window(), &winw, &winh);

  const int guiw = m_gui->get_width();

  const double w2h_ratio = m_map.get_width()/m_map.get_height();

  const int w1 = winw - guiw - 2;
  const int h1 = w1/w2h_ratio;
  const pt2d_i pos1 = {guiw + 1, (winh - h1)/2};

  const int h2 = winh - 1;
  const int w2 = h2*w2h_ratio;
  const pt2d_i pos2 = {(guiw + winw - w2)/2, 0};

  const int maxw = winw - guiw;
  const int maxh = winh;

  if (w1 < maxw and h1 < maxh)
  {
    at = pos1;
    w = w1;
    h = h1;
  }
  else
  {
    at = pos2;
    w = w2;
    h = h2;
  }
}

void
mw::map_editor::draw() const
{
  SDL_SetRenderDrawColor(m_sdl.get_renderer(), 0x00, 0x00, 0x00, 0);
  SDL_RenderClear(m_sdl.get_renderer());

  pt2d_i mappos;
  int mapw, maph;
  _fit_map_to_screen(mappos, mapw, maph);
  m_map.adjust_to_box_w(mappos, mapw);
  m_map.draw_all();

  if (m_processor != nullptr)
    m_processor->visualise();

  m_gui->draw({0, 0});
}

void
mw::map_editor::run_layer(ui_manager &uiman)
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    pt2d_i mousepos;
    SDL_GetMouseState(&mousepos.x, &mousepos.y);

    switch (event.type)
    {
      case SDL_WINDOWEVENT:
        draw();
        break;

      case SDL_MOUSEMOTION:
      {
        const int guiw = m_gui->get_width();
        const int guih = m_gui->get_height();
        const pt2d_d mappos = m_map.pixels_to_point(mousepos);

        if (0 < mousepos.x and mousepos.x < guiw and
            0 < mousepos.y and mousepos.y < guih)
        {
          m_gui->send("hover", std::any(mousepos));
        }
        else if (0 <= mappos.x and mappos.x <= m_map.get_width() and
                 0 <= mappos.y and mappos.y <= m_map.get_height())
        {
          if (m_processor)
            m_processor->mouse_move_within(mappos);
        }
        else
        {
          if (m_processor)
            m_processor->mouse_move_outside();
        }
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
      {
        const int guiw = m_gui->get_width();
        const int guih = m_gui->get_height();
        const pt2d_d mappos = m_map.pixels_to_point(mousepos);
        if (0 <= mappos.x and mappos.x <= m_map.get_width() and
            0 <= mappos.y and mappos.y <= m_map.get_height())
        {
          if (m_processor)
            m_processor->mouse_button_down(event.button.button);
        }
        break;
      }

      case SDL_MOUSEBUTTONUP:
        if (event.button.button == 1)
        {
          const int guiw = m_gui->get_width();
          const int guih = m_gui->get_height();
          const pt2d_d mappos = m_map.pixels_to_point(mousepos);
          if (0 < mousepos.x and mousepos.x < guiw and
              0 < mousepos.y and mousepos.y < guih)
          {
            m_gui->send("clicked", std::any(mousepos));
          }
          else if (0 <= mappos.x and mappos.x <= m_map.get_width() and
                   0 <= mappos.y and mappos.y <= m_map.get_height())
          {
            if (m_processor)
              m_processor->mouse_button_up(event.button.button);
          }
        }
        break;

      case SDL_KEYDOWN:
        if (m_processor)
          m_processor->key_down(event.key.keysym.sym);
        break;

      case SDL_KEYUP:
        if (m_processor)
          m_processor->key_up(event.key.keysym.sym);
        break;
    }
  }

  if (fps_guardian(SDL_GetTicks()))
  {
    uiman.draw(get_id());
    SDL_RenderPresent(m_sdl.get_renderer());
    fps_guardian.notify();
  }

  if (m_pending_exit)
    // XXX: "this" will be deleted on the next line
    uiman.remove_layer(get_id());
}

