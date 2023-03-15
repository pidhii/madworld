#ifndef MAP_EDITOR_HPP
#define MAP_EDITOR_HPP

#include "area_map.hpp"
#include "ui_layer.hpp"
#include "sdl_environment.hpp"
#include "gui/components.hpp"

namespace mw {

class map_editor: public ui_layer {
  public:
  map_editor(sdl_environment &sdl, area_map &map);

  ~map_editor()
  {
    delete m_gui;
    if (m_processor)
      delete m_processor;
  }

  void
  draw() const override;

  void
  run_layer(ui_manager &uiman) override;

  void
  destroy_layer() override
  { delete this; }

  private:
  component*
  _build_gui();

  void
  _fit_map_to_screen(pt2d_i &at, int &w, int &h) const noexcept;

  public:
  struct processor {
    virtual ~processor() = default;

    virtual void mouse_button_down(int button) = 0;
    virtual void mouse_button_up(int button) = 0;

    virtual void mouse_move_within(const pt2d_d &at) = 0;
    virtual void mouse_move_outside() = 0;

    virtual void key_down(int sym) = 0;
    virtual void key_up(int sym) = 0;

    virtual void visualise() const = 0;
  };

  private:
  sdl_environment &m_sdl;
  area_map &m_map;
  component *m_gui;
  processor *m_processor;
  bool m_pending_exit;
}; // class mw::map_editor

} // namespace mw

#endif
