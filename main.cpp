#include "logging.h"
#include "geometry.hpp"
#include "area_map.hpp"
#include "player.hpp"
#include "npc.hpp"
#include "game_manager.hpp"
#include "ui_manager.hpp"
#include "video_manager.hpp"
#include "map_editor.hpp"
#include "fps_display.hpp"
#include "textures.hpp"
#include "gui/menu.hpp"
#include "gui/composer.hpp"
#include "controls/keyboard_controller.hpp"
#include "central_config.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <SDL2/SDL_keyboard.h>
#include <ether/sandbox.hpp>

#include <vector>
#include <list>

#include <boost/format.hpp>


static mw::component*
_make_npc_ctl_hud(mw::sdl_environment &sdl,
    mw::area_map *map,
    mw::heads_up_display *hud,
    const mw::sdl_string_factory &strfac,
    const mw::safe_pointer<mw::npc> &npcptr,
    mw::radio_group_ptr &mem_radios)
{
  mw::label *label = new mw::label {sdl, strfac(npcptr.get()->get_nickname())};
  label->on("update", [=] (MWGUI_CALLBACK_ARGS) -> int {
      const mw::pt2d_d &pos = npcptr.get()->get_position();
      const std::string text =
        (boost::format("%s @ (%.0f, %.0f)")
         % npcptr.get()->get_nickname()
         % npcptr.get()->get_position().x
         % npcptr.get()->get_position().y).str();
      self->set_string(strfac(text));
      return 0;
  });
  label->set_min_width(150);

  mw::label *mem_label = new mw::label {sdl, strfac("[show memory]")};
  mw::radio_entry *mem_radio = new mw::radio_entry {sdl, 0xFF33AA33, 8};
  auto add_heatmap_hud = [=] () -> void {
      // check if there exists a heatmap for this NPC
      mw::simple_ai *npcai = nullptr;
      if (npcptr and (npcai = dynamic_cast<mw::simple_ai*>(&npcptr->get_mind())))
      { // enable the heatmap
        mw::custom_hud *heatmaphud = new mw::custom_hud;
        heatmaphud->on_update([=] (mw::heads_up_display &hud) -> void {
            if (not npcptr or mem_radio->get_state() == false)
              hud.remove_component(heatmaphud->get_id());
        });
        heatmaphud->on_draw([=] (mw::rectangle &box) -> void {
            box.offset = {0, 0};
            box.width = box.height = 0;
            npcai->get_explorer().draw_heatmap(sdl.get_renderer(), map->get_view());

            const double vr = npcai->get_explorer().get_vision_radius() * map->get_scale();
            const double mr = npcai->get_explorer().get_mark_radius() * map->get_scale();
            const mw::pt2d_i c = mw::pt2d_i(map->get_view()(npcptr->get_position()));
            aacircleColor(sdl.get_renderer(), c.x, c.y, vr, 0x77AA3333);
            aacircleColor(sdl.get_renderer(), c.x, c.y, mr, 0x77AA33AA);
        });
        hud->add_component(heatmaphud);
      }
  };
  mw::add_to_radio_group(mem_radios, mem_radio);
  mem_radio->on("clicked", [=] (MWGUI_CALLBACK_ARGS) -> int {
      if (self->get_state() == false)
      {
        for (mw::radio_entry *r : *mem_radios)
          r->set_state(false);
        self->set_state(true);
        add_heatmap_hud();
      }
      else // self state == true
        self->set_state(false);
      return true;
  });
  mem_radio->set_top_margin(1);
  mem_radio->set_bottom_margin(1);
  mem_radio->set_left_margin(4);
  mem_radio->set_right_margin(1);
  mem_label->forward("clicked", mem_radio);

  mw::label *chase_label = new mw::label {sdl, strfac("[chase player]")};
  mw::radio_entry *chase_radio = new mw::radio_entry {sdl, 0xFF33AA33, 8};
  chase_radio->on("update", [=] (MWGUI_CALLBACK_ARGS) -> int {
      mw::simple_ai *npcai = nullptr;
      if (npcptr and (npcai = dynamic_cast<mw::simple_ai*>(&npcptr->get_mind())))
        self->set_state(npcai->get_chase_player());
      else
        self->set_state(false);
      return 0;
  });
  chase_radio->on("clicked", [=] (MWGUI_CALLBACK_ARGS) -> int {
      mw::simple_ai *npcai = nullptr;
      if (npcptr and (npcai = dynamic_cast<mw::simple_ai*>(&npcptr->get_mind())))
      {
        const bool newstate = !self->get_state();
        npcai->set_chase_player(newstate);
        self->set_state(newstate);
      }
      return 0;
  });
  chase_radio->set_top_margin(1);
  chase_radio->set_bottom_margin(1);
  chase_radio->set_left_margin(4);
  chase_radio->set_right_margin(1);
  chase_label->forward("clicked", chase_radio);

  mw::horisontal_layout *layout = new mw::horisontal_layout {sdl};
  layout->on("update", [=] (MWGUI_CALLBACK_ARGS) -> int {
      label->send("update", 0);
      chase_radio->send("update", 0);
      return 0;
  });
  layout->add_component(label);
  layout->add_component(mem_label);
  layout->add_component(mem_radio);
  layout->add_component(new mw::padding {sdl, 15, 0});
  layout->add_component(chase_label);
  layout->add_component(chase_radio);

  return layout;
}

class npc_access_hud: public mw::hud_component {
  public:
  npc_access_hud(mw::sdl_environment &sdl, TTF_Font *font, const mw::pt2d_i &at)
  : m_sdl {sdl},
    m_at {at},
    m_gui {new mw::vertical_layout {sdl}}
  { }

  ~npc_access_hud()
  { delete m_gui; }

  void
  add_npc(const mw::safe_pointer<mw::npc> &npcptr, mw::component *entry)
  {
    auto entry_id = m_gui->add_component(entry);
    m_data.emplace_back(entry_id, npcptr);
  }

  void
  update(mw::heads_up_display &hud) override
  {
    auto it = m_data.begin();
    while (it != m_data.end())
    {
      auto gui_entry_id = it->first;
      auto npc_ptr = it->second;
      if (not npc_ptr)
      {
        m_gui->remove_entry(gui_entry_id);
        auto tmp = it;
        ++it;
        m_data.erase(tmp);
        continue;
      }
      ++it;
    }
  }

  void
  draw(mw::rectangle &box) const override
  {
    for (const auto &[entry_id, npcptr] : m_data)
      m_gui->get_component(entry_id)->send("update", 0);

    _get_gui_box(box);
    m_gui->draw(m_at);
  }

  void
  click(const mw::pt2d_i &at) override
  {
    mw::rectangle box;
    _get_gui_box(box);
    m_gui->send("hover-begin", std::any(at - mw::pt2d_i(box.offset)));
    m_gui->send("clicked", {});
    m_gui->send("hover-end", {});
  }

  private:
  void
  _get_gui_box(mw::rectangle &box) const
  {
    box.offset = mw::pt2d_d(m_at);
    box.width = m_gui->get_width();
    box.height = m_gui->get_height();
  }

  private:
  mw::sdl_environment &m_sdl;
  mw::pt2d_i m_at;
  mw::linear_layout *m_gui;
  std::list<std::pair<mw::linear_layout::entry_id, mw::safe_pointer<mw::npc>>>
    m_data;
}; // class npc_access_hud


class npc_name_hud: public mw::hud_component {
  public:
  npc_name_hud(mw::sdl_environment &sdl, const mw::area_map &map,
      const mw::safe_pointer<mw::npc> &npcptr,
      const mw::sdl_string_factory &strfac)
  : m_sdl {sdl},
    m_map {map},
    m_string {strfac(npcptr->get_nickname())},
    m_npcptr {npcptr}
  { }

  void
  update(mw::heads_up_display &hud) override
  {
    if (not m_npcptr)
      hud.remove_component(get_id());
  }

  void
  draw(mw::rectangle &box) const override
  {
    box.offset = {0, 0};
    box.width = box.height = 0;

    const double r = m_npcptr->get_radius();
    const mw::mapping viewport = m_map.get_view();

    mw::texture_info texinfo;
    m_string.get_texture_info(m_sdl.get_renderer(), texinfo);
    const mw::pt2d_i npcpos = mw::pt2d_i(viewport(m_npcptr->get_position()));
    const mw::pt2d_i at =
      npcpos + mw::vec2d_i(-texinfo.w/2, -(int(r) + texinfo.h + 5));
    m_string.draw(m_sdl.get_renderer(), at);
  }

  private:
  mw::sdl_environment &m_sdl;
  const mw::area_map &m_map;
  mw::sdl_string m_string;
  mw::safe_pointer<mw::npc> m_npcptr;
}; // class npc_name_hud


static int
the_main(int argc, char **argv)
{
  mw::video_manager& vman = mw::video_manager::instance();
  vman.init();

  mw::sdl_environment &sdl = vman.get_sdl();

  mw::texture_storage textures {sdl.get_renderer()};
  textures.load("PlayerGlow", "./textures/vision-texture.png");
  textures.load("BloodStain", "./textures/blood-stain-1.png");
  textures.load("BulletGlow", "./textures/BulletGlow2.png");

  mw::area_map map {sdl, textures};
  map.load("./map.eth");
  map.init_background(4096*2, 4096*2);

  mw::player *player1 = new mw::player {{20, 20}, 0.007};
  info("player1 = %p", static_cast<mw::vis_obstacle*>(player1));
  player1->set_glow(textures["PlayerGlow"].tex, 20);
  // ---
  mw::simple_gun *gun = new mw::simple_gun {30, 10, 1e-3};
  gun->set_bullet_glow(textures["BulletGlow"].tex, 0x55);
  player1->set_ability(mw::ability_slot::lmb, gun);
  // ---
  mw::object_id player1_id = map.add_object(player1);
  map.register_phys_object(player1_id);
  map.register_phys_obstacle(player1_id);
  map.register_vis_obstacle(player1_id);

  std::vector<mw::safe_pointer<mw::npc>> npcs;
  for (int i = 0; i < 10; ++i)
  {
    mw::npc *npc = new mw::npc {0.3, {41. + i*1., 20}};
    npc->set_nickname(std::string("NPC-") + std::to_string(i));
    npc->set_move_speed(0.01);
    npc->set_vision_radius(21);
    npc->make_mind<mw::simple_ai>(*npc, map).set_chase_player(false);
    mw::simple_body &npcbody = npc->make_body<mw::simple_body>(*npc, 16., 7.);
    npcbody.set_blood_stain(textures["BloodStain"].tex);
    npcbody.set_blood(false);
    npc->set_color(0xFF5555AA);
    mw::object_id npc_id = map.add_object(npc);
    map.register_phys_object(npc_id);
    map.register_phys_obstacle(npc_id);
    map.register_vis_obstacle(npc_id);

    npcs.push_back(npc->get_safe_pointer());
  }

  mw::ui_manager uiman {sdl};

  TTF_Font *font = vman.get_font();
  TTF_Font *small_font =
    TTF_OpenFont(mw::video_config::instance().get_config().font.path.c_str(),10);

  mw::vertical_layout *menu_layout = new mw::vertical_layout {sdl};
  std::shared_ptr<mw::basic_menu> main_menu = std::make_shared<mw::basic_menu>(sdl, menu_layout);

  mw::keyboard_controller kbrd {sdl};
  const eth::value &inputcfg =
    mw::central_config::instance().get_controls_config();
  kbrd.make_button("menu", SDL_GetScancodeFromName("escape"));
  kbrd.make_button("zoom-in", SDL_GetScancodeFromName("="), mw::keyboard_controller::shift);
  kbrd.make_button("zoom-out", SDL_GetScancodeFromName("-"));
  kbrd.make_button("zoom-reset", SDL_GetScancodeFromName("="));
  kbrd.make_button("camera-left", SDL_GetScancodeFromName("left"));
  kbrd.make_button("camera-right", SDL_GetScancodeFromName("right"));
  kbrd.make_button("camera-up", SDL_GetScancodeFromName("up"));
  kbrd.make_button("camera-down", SDL_GetScancodeFromName("down"));
  kbrd.make_button("camera-center", SDL_GetScancodeFromName("space"));
  kbrd.make_button("fow-toggle", SDL_GetScancodeFromName("`"));
  kbrd.make_analog("move-x-axis", SDL_GetScancodeFromName("d"), SDL_GetScancodeFromName("a"));
  kbrd.make_analog("move-y-axis", SDL_GetScancodeFromName("s"), SDL_GetScancodeFromName("w"));
  kbrd.make_left_mouse_button("ability1");
  kbrd.make_left_mouse_button("pointer-click");

  mw::composer guicomp {sdl, font};
  menu_layout->add_component(
    guicomp.make_button("Enter the Game")
    ->on("clicked", [&] (MWGUI_CALLBACK_ARGS) {
      info("starting game_manager");
      self->set_hover(false);
      std::shared_ptr<mw::game_manager> gman =
        std::make_shared<mw::game_manager>(sdl, map, kbrd);
      gman->set_player(*player1, 20);

      mw::sdl_string_factory hud_strfac {small_font};

      npc_access_hud *npchud = new npc_access_hud {sdl, small_font, {100, 10}};
      mw::radio_group_ptr memradios = mw::make_radio_group();
      for (auto it = npcs.begin(); it != npcs.end(); ++it)
      {
        mw::safe_pointer<mw::npc> npcptr = *it;
        mw::component *npcctlhud =
          _make_npc_ctl_hud(sdl, &map, &gman->hud(), hud_strfac, npcptr, memradios);
        npchud->add_npc(npcptr, npcctlhud);
      }
      gman->hud().add_component(npchud);

      for (auto it = npcs.begin(); it != npcs.end(); ++it)
      {
        mw::safe_pointer<mw::npc> npcptr = *it;
        gman->hud().add_component(
            new npc_name_hud {sdl, map, npcptr, hud_strfac});
      }

      uiman.add_layer(gman);
      return 0;
    })
  );
  menu_layout->add_component(
    guicomp.make_button("Map editor")
    ->on("clicked", [&] (MWGUI_CALLBACK_ARGS) {
      info("starting map_editor");
      self->set_hover(false);
      uiman.add_layer(std::make_shared<mw::map_editor>(sdl, map));
      return 0;
    })
  );
  menu_layout->add_component(
    guicomp.make_button("Settings")
    ->on("clicked", [&] (MWGUI_CALLBACK_ARGS) {
      self->set_hover(false);
      uiman.add_layer(std::shared_ptr<mw::ui_layer> {
          mw::video_manager::instance().make_new_settings()});
      return 0;
    })
  );
  const int point_size =
    mw::video_config::instance().get_config().font.point_size;
  menu_layout->add_component(new mw::padding {sdl, 0, point_size});
  menu_layout->add_component(
    guicomp.make_button("Quit")
    ->on("clicked", [&] (MWGUI_CALLBACK_ARGS) {
      self->set_hover(false);
      info("quit");
      main_menu->close();
      return 0;
    })
  );

  menu_layout->set_min_width(menu_layout->get_width() + 15);

  uiman.add_layer(main_menu);

  mw::sdl_string_factory fpsstrfac {small_font};
  fpsstrfac.set_fg_color(0xFF00FF00);
  std::shared_ptr<mw::fps_display> fpsdisp =
    std::make_shared<mw::fps_display>(sdl, fpsstrfac, 5, 5);
  fpsdisp->set_format("FPS: %d");
  uiman.add_float(fpsdisp);

  uiman.draw(uiman.get_top_layer_id());
  uiman.run();

  TTF_CloseFont(small_font);

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
