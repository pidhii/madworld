#include "video_manager.hpp"
#include "ui_manager.hpp"
#include "logging.h"
#include "gui/menu.hpp"
#include "color_manager.hpp"

#include <ether/sandbox.hpp>

#include <sstream>
#include <vector>

#include <boost/format.hpp>

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>


mw::fps_guardian_type &mw::fps_guardian = fps_guardian_type::instance();



mw::video_config&
mw::video_config::instance()
{
  static bool is_first_time = true;
  static video_config self;
  if (is_first_time)
  {
    load_config("/home/pidhii/sandbox/create/madworld/config.eth", self.m_config);
    is_first_time = false;
  }
  return self;
}

void
mw::video_config::load_config(const std::string &path, config &cfg)
{
  eth::sandbox ether;

  std::ostringstream cmd;
  cmd << "first $ load '" << path << "'";
  const eth::value conf = ether(cmd.str());

  cfg.window_size.w = conf["video"]["window_size"][0];
  cfg.window_size.h = conf["video"]["window_size"][1];
  cfg.resolution.x = conf["video"]["resolution"][0];
  cfg.resolution.y = conf["video"]["resolution"][1];
  cfg.fullscreen = bool(conf["video"]["fullscreen"]);
  cfg.hardware_acceleration = bool(conf["video"]["hardware_acceleration"]);
  cfg.vsync = bool(conf["video"]["vsync"]);
  cfg.fps_limit = conf["video"]["fps_limit"];
  cfg.font.path = conf["video"]["font"][0].str();
  cfg.font.point_size = conf["video"]["font"][1];
}

eth::value
mw::video_config::dump_config(const config &cfg)
{
  eth::sandbox ether;
  return ether("record")(eth::rev_list(std::vector<eth::value> {
    eth::tuple("window_size", eth::tuple(cfg.window_size.w, cfg.window_size.h)),
    eth::tuple("resolution", eth::tuple(cfg.resolution.x, cfg.resolution.y)),
    eth::tuple("fullscreen", eth::boolean(cfg.fullscreen)),
    eth::tuple("hardware_acceleration", eth::boolean(cfg.hardware_acceleration)),
    eth::tuple("vsync", eth::boolean(cfg.vsync)),
    eth::tuple("fps_limit", cfg.fps_limit),
    eth::tuple("font", eth::tuple(cfg.font.path, cfg.font.point_size)),
  }));
}

void
mw::fps_guardian_type::notify() noexcept
{
  m_prev_present_time = SDL_GetTicks();

  const time_t cur_second = m_prev_present_time/1000;
  if (cur_second == m_cur_second)
    m_frame_count += 1;
  else
  {
    double r = 0.5;
    if (m_avg_fps.has_value())
      m_avg_fps = r*m_frame_count + (1 - r)*m_avg_fps.value();
    else
      m_avg_fps = m_frame_count;

    m_last_fps = m_frame_count;
    m_cur_second = cur_second;
    m_frame_count = 1;
  }
}


mw::video_manager&
mw::video_manager::instance()
{
  static video_manager self;
  return self;
}

mw::video_manager::~video_manager()
{
  if (m_sdl.m_rend)
    SDL_DestroyRenderer(m_sdl.m_rend);
  if (m_sdl.m_win)
    SDL_DestroyWindow(m_sdl.m_win);
  if (m_font)
    TTF_CloseFont(m_font);
  if (m_small_font)
    TTF_CloseFont(m_small_font);
  SDL_Quit();
  TTF_Quit();
}

void
mw::video_manager::init()
{
  const video_config::config &cfg = video_config::instance().get_config();

  if (SDL_Init(SDL_INIT_EVERYTHING))
  {
    error("failed to initialize SDL subsystems (%s)", SDL_GetError());
    throw std::runtime_error {"failed to initilize SDL"};
  }

  if (TTF_Init() < 0)
  {
    error("failed to initialize fonts (%s)", TTF_GetError());
    throw std::runtime_error {"failed to initilize TTF"};
  }

  if (IMG_Init(IMG_INIT_PNG) == 0)
  {
    error("failed to initialize images (%s)", IMG_GetError());
    throw std::runtime_error {"failed to initilize IMG"};
  }

  m_sdl.m_win = SDL_CreateWindow("Mad World", SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, cfg.window_size.w, cfg.window_size.h,
      SDL_WINDOW_RESIZABLE);
  if (m_sdl.m_win == NULL)
  {
    error("failed to create main window (%s)", SDL_GetError());
    throw std::runtime_error {"failed to initilize main window"};
  }
  if (cfg.fullscreen)
  {
    info("fullscreen mode ON");
    SDL_SetWindowSize(m_sdl.m_win, cfg.resolution.x, cfg.resolution.y);
    SDL_SetWindowFullscreen(m_sdl.m_win, SDL_WINDOW_FULLSCREEN);
  }

  int flags = SDL_RENDERER_TARGETTEXTURE;
  if (cfg.hardware_acceleration)
    flags |= SDL_RENDERER_ACCELERATED;
  if (cfg.vsync)
    flags |= SDL_RENDERER_PRESENTVSYNC;
  m_sdl.m_rend = SDL_CreateRenderer(m_sdl.m_win, -1, flags);
  if (m_sdl.m_rend == NULL)
  {
    error("failed to initialize renderer (%s)", SDL_GetError());
    throw std::runtime_error {"failed to initilize renderer"};
  }

  if (not (m_font = TTF_OpenFont(cfg.font.path.c_str(), cfg.font.point_size)))
  {
    error("failed to open main font (%s)", SDL_GetError());
    throw std::runtime_error {"failed to open main font"};
  }
  if (not (m_small_font = TTF_OpenFont(cfg.font.path.c_str(), cfg.font.point_size*0.65)))
  {
    error("failed to open small font (%s)", SDL_GetError());
    throw std::runtime_error {"failed to open small font"};
  }
}

mw::sdl_environment&
mw::video_manager::get_sdl()
{
  if (m_sdl.m_win == nullptr or m_sdl.m_rend == nullptr)
    throw std::logic_error {"mw::video_manager::init() must be called first"};
  return m_sdl;
}

mw::gui::basic_menu*
mw::video_manager::make_new_settings()
{
  const color_manager &cman = color_manager::instance();
  const video_config::config &cfg = video_config::instance().get_config();

  const int chrw = cfg.font.point_size/2;
  const int chrh = cfg.font.point_size;

  boost::format fmt_dxd {"%dx%d"};
  boost::format fmt_d {"%d"};

  sdl_string_factory make_label_string {m_font, cman["InteractiveText"]};
  sdl_string_factory make_hl_label_string {m_font, cman["HighlightedInteractiveText"]};
  make_hl_label_string.set_style(TTF_STYLE_BOLD);

  sdl_string_factory make_normal_string {m_font, cman["Normal"]};
  sdl_string_factory make_boolean_string {m_font, cman["Boolean"]};
  sdl_string_factory make_string_string {m_font, cman["String"]};
  make_string_string.set_bg_color(0x77101010);

  auto on_hover_begin = [=] (MWGUI_CALLBACK_ARGS) {
    self->set_string(make_hl_label_string(self->get_string().get_text()));
    return menu_callback_request::nothing;
  };
  auto on_hover_end = [=] (MWGUI_CALLBACK_ARGS) {
    self->set_string(make_label_string(self->get_string().get_text()));
    return menu_callback_request::nothing;
  };

  vertical_layout *menu_layout = new vertical_layout {m_sdl};
  basic_menu *menu = new basic_menu {m_sdl, menu_layout};


  //---------------------------< window size >----------------------------------
  //
  label *window_size_label =
    new label {m_sdl, make_label_string("window size: ")};
  window_size_label->on("hover-begin", on_hover_begin);
  window_size_label->on("hover-end", on_hover_end);

  text_entry *window_size_text_entry =
    new text_entry {m_sdl, m_font, cman["Number"], chrw*20, chrh};
  window_size_text_entry
    ->on("<text_entry>:return", [this, menu] (MWGUI_CALLBACK_ARGS) {
    menu->detach_keyboard_receiver();
    self->set_cursor(false);
    int w, h;
    if (sscanf(self->get_text().c_str(), "%dx%d", &w, &h) == 2)
    {
      info("new window size: %dx%d", w, h);
      video_config::config &cfg = video_config::instance().m_config;
      cfg.window_size.w = w;
      cfg.window_size.h = h;
      SDL_SetWindowSize(m_sdl.m_win, w, h);
    }
    else
    {
      const video_config::config &cfg = video_config::instance().get_config();
      boost::format fmt_dxd {"%dx%d"};
      self->set_text((fmt_dxd % cfg.window_size.w % cfg.window_size.h).str());
    }
    return menu_callback_request::nothing;
  });
  window_size_text_entry
    ->set_text((fmt_dxd % cfg.window_size.w % cfg.window_size.h).str());

  horisontal_layout *window_size_component = new horisontal_layout {m_sdl};
  window_size_component->on("clicked", [=] (MWGUI_CALLBACK_ARGS) {
    menu->attach_keyboard_receiver(window_size_text_entry);
    window_size_text_entry->set_cursor(true);
    return menu_callback_request::nothing;
  });
  window_size_component->forward("hover-begin", window_size_label);
  window_size_component->forward("hover", window_size_label);
  window_size_component->forward("hover-end", window_size_label);
  window_size_component->add_component(window_size_label);
  window_size_component->add_component(window_size_text_entry);
  menu_layout->add_component(window_size_component);


  //---------------------------< resolution >-----------------------------------
  //
  label *resolution_label =
    new label {m_sdl, make_label_string("resolution: ")};
  resolution_label->on("hover-begin", on_hover_begin);
  resolution_label->on("hover-end", on_hover_end);

  text_entry *resolution_text_entry =
    new text_entry {m_sdl, m_font, cman["Number"], chrw*20, chrh};
  resolution_text_entry
    ->on("<text_entry>:return", [this, menu] (MWGUI_CALLBACK_ARGS) {
    menu->detach_keyboard_receiver();
    self->set_cursor(false);
    int x, y;
    const video_config::config &cfg = video_config::instance().get_config();
    if (sscanf(self->get_text().c_str(), "%dx%d", &x, &y) == 2)
    {
      info("screen resolution changed: %dx%d", x, y);
      video_config::config &cfg = video_config::instance().m_config;
      cfg.resolution.x = x;
      cfg.resolution.y = y;
      if (cfg.fullscreen)
        SDL_SetWindowSize(m_sdl.m_win, x, y);
    }
    else
    {
      const video_config::config &cfg = video_config::instance().get_config();
      boost::format fmt_dxd {"%dx%d"};
      self->set_text((fmt_dxd % cfg.resolution.x % cfg.resolution.y).str());
    }
    return menu_callback_request::nothing;
  });
  resolution_text_entry->
    set_text((fmt_dxd % cfg.resolution.x % cfg.resolution.y).str());

  horisontal_layout *resolution_component = new horisontal_layout {m_sdl};
  resolution_component->on("clicked", [=] (MWGUI_CALLBACK_ARGS) {
    menu->attach_keyboard_receiver(resolution_text_entry);
    resolution_text_entry->set_cursor(true);
    return menu_callback_request::nothing;
  });
  resolution_component->forward("hover-begin", resolution_label);
  resolution_component->forward("hover", resolution_label);
  resolution_component->forward("hover-end", resolution_label);
  resolution_component->add_component(resolution_label);
  resolution_component->add_component(resolution_text_entry);
  menu_layout->add_component(resolution_component);


  //---------------------------< fullscreen >-----------------------------------
  //
  label *fullscreen_label =
    new label {m_sdl, make_label_string("fullscreen: ")};
  fullscreen_label->on("hover-begin", on_hover_begin);
  fullscreen_label->on("hover-end", on_hover_end);

  label *fullscreen_value_label =
    new label {m_sdl, make_boolean_string(cfg.fullscreen ? "true" : "false")};

  horisontal_layout *fullscreen_component = new horisontal_layout {m_sdl};
  fullscreen_component->on("clicked", [=] (MWGUI_CALLBACK_ARGS) {
    video_config::config &cfg = video_config::instance().m_config;
    if (not cfg.fullscreen)
    {
      info("fullscreen ON");
      SDL_SetWindowSize(m_sdl.m_win, cfg.resolution.x, cfg.resolution.y);
      SDL_SetWindowFullscreen(m_sdl.m_win, SDL_WINDOW_FULLSCREEN);
      cfg.fullscreen = true;
      fullscreen_value_label->set_string(make_boolean_string("true"));
    }
    else
    {
      info("fullscreen OFF");
      SDL_SetWindowFullscreen(m_sdl.m_win, 0);
      SDL_SetWindowSize(m_sdl.m_win, cfg.window_size.w, cfg.window_size.h);
      cfg.fullscreen = false;
      fullscreen_value_label->set_string(make_boolean_string("false"));
    }
    return menu_callback_request::nothing;
  });
  fullscreen_component->forward("hover-begin", fullscreen_label);
  fullscreen_component->forward("hover", fullscreen_label);
  fullscreen_component->forward("hover-end", fullscreen_label);
  fullscreen_component->add_component(fullscreen_label);
  fullscreen_component->add_component(fullscreen_value_label);
  menu_layout->add_component(fullscreen_component);


  //-----------------------< hardware acceleration >----------------------------
  //
  label *hardware_acc_label =
    new label {m_sdl, make_normal_string("hardware acceleration: ")};

  label *hardware_acc_value_label =
    new label {m_sdl,
      make_boolean_string(cfg.hardware_acceleration ? "true" : "false")};

  horisontal_layout *hardware_acc_component = new horisontal_layout {m_sdl};
  hardware_acc_component->add_component(hardware_acc_label);
  hardware_acc_component->add_component(hardware_acc_value_label);
  menu_layout->add_component(hardware_acc_component);


  //-----------------------------< V-sync >-------------------------------------
  //
  label *vsync_label = new label {m_sdl, make_normal_string("V-sync: ")};
  vsync_label->on("hover-begin", on_hover_begin);
  vsync_label->on("hover-end", on_hover_end);

  label *vsync_value_label =
    new label {m_sdl, make_boolean_string(cfg.vsync ? "true" : "false")};

  horisontal_layout *vsync_component = new horisontal_layout {m_sdl};
  vsync_component->add_component(vsync_label);
  vsync_component->add_component(vsync_value_label);
  menu_layout->add_component(vsync_component);


  //----------------------------< FPS limit >-----------------------------------
  //
  label *fps_limit_lable = new label {m_sdl, make_label_string("FPS limit: ")};
  fps_limit_lable->on("hover-begin", on_hover_begin);
  fps_limit_lable->on("hover-end", on_hover_end);

  text_entry *fps_limit_text_entry =
    new text_entry {m_sdl, m_font, cman["Number"], chrw*10, chrh};
  fps_limit_text_entry
    ->on("<text_entry>:return", [this, menu] (MWGUI_CALLBACK_ARGS) {
    menu->detach_keyboard_receiver();
    self->set_cursor(false);
    int new_fps;
    if (sscanf(self->get_text().c_str(), "%d", &new_fps) == 1)
    {
      info("changing FPS limit to %d", new_fps);
      video_config::config &cfg = video_config::instance().m_config;
      cfg.fps_limit = new_fps;
      fps_guardian.update_fps_limit();
    }
    else
    {
      const video_config::config &cfg = video_config::instance().get_config();
      boost::format fmt_d {"%d"};
      self->set_text((fmt_d % cfg.fps_limit).str());
    }
    return menu_callback_request::nothing;
  });
  fps_limit_text_entry->set_text((fmt_d % cfg.fps_limit).str());

  horisontal_layout *fps_limit_component = new horisontal_layout {m_sdl};
  fps_limit_component->on("clicked", [=] (MWGUI_CALLBACK_ARGS) {
    menu->attach_keyboard_receiver(fps_limit_text_entry);
    fps_limit_text_entry->set_cursor(true);
    return menu_callback_request::nothing;
  });
  fps_limit_component->forward("hover-begin", fps_limit_lable);
  fps_limit_component->forward("hover", fps_limit_lable);
  fps_limit_component->forward("hover-end", fps_limit_lable);
  fps_limit_component->add_component(fps_limit_lable);
  fps_limit_component->add_component(fps_limit_text_entry);
  menu_layout->add_component(fps_limit_component);


  //-- ---------------------------< font >--------------------------------------
  //
  label *font_label = new label {m_sdl, make_normal_string("font: ")};

  label *font_value_label =
    new label {m_sdl, make_string_string(
        (boost::format("%s:%d") % cfg.font.path % cfg.font.point_size).str())};

  horisontal_layout *font_component = new horisontal_layout {m_sdl};
  font_component->add_component(font_label);
  font_component->add_component(font_value_label);
  menu_layout->add_component(font_component);

  //-- --------------------------< back >---------------------------------------
  //
  menu_layout->add_component(new padding {m_sdl, 0, cfg.font.point_size});
  label *back_button = new label {m_sdl, make_label_string("Back")};
  back_button
    ->on("clicked", [menu] (MWGUI_CALLBACK_ARGS) {
      self->send("hover-end", 0);
      menu->close();
      return menu_callback_request::exit_loop;
    })
    ->on("hover-begin", on_hover_begin)
    ->on("hover-end", on_hover_end);
  menu_layout->add_component(back_button);

  return menu;
}

