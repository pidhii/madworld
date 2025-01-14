#ifndef HUD_HPP
#define HUD_HPP

#include "common.hpp"
#include "geometry.hpp"
#include "utl/safe_access.hpp"

#include <list>
#include <vector>
#include <functional>


namespace mw {

class hud_component;


class hud_footprint {
  public:
  void
  add(const rectangle &box, const safe_pointer<hud_component> &hud)
  { m_boxes.emplace_back(box, hud); }

  bool
  contains(const pt2d_i pt, safe_pointer<hud_component> &hud) const noexcept;

  bool
  contains(const pt2d_i pt) const noexcept
  { safe_pointer<hud_component> hud; return contains(pt, hud); }

  void
  clear() noexcept
  { m_boxes.clear(); }

  private:
  std::vector<std::pair<rectangle, safe_pointer<hud_component>>> m_boxes;
}; // class mw::hud_footprint


class heads_up_display {
  public:
  using hud_container = std::list<hud_component*>;
  using hud_id = identifier<hud_container::const_iterator, heads_up_display>;

  ~heads_up_display();

  hud_id
  add_component(hud_component *hud) noexcept;

  void
  remove_component(hud_id id);

  void
  update();

  void
  draw(hud_footprint &area) const;

  private:
  hud_container m_huds;
}; // class mw::heads_up_display

using hud_id = heads_up_display::hud_id;


class hud_component: public safe_access<hud_component> {
  public:
  hud_component(): safe_access(this) { }

  virtual
  ~hud_component() = default;

  /**
   * @brief Update HUD component.
   *
   * This method is called for all HUD components after each game-tick and, if
   * the rendering is done before the next tick, the call preceeds the rendering
   * (i.e. before calling @ref hud_component::draw()). Note that it means that
   * expencive operations, such as operations with textures, should be avoided
   * in this method, and should rather be done within @ref hud_component::draw().
   *
   * @note It is safe for a component to delete itself and/or other components
   * during the update.
   */
  virtual void
  update(heads_up_display &hud) = 0;

  virtual void
  draw(rectangle &box) const = 0;

  virtual void
  click(const pt2d_i &at) { }

  virtual void
  destroy() { delete this; }

  void
  set_id(heads_up_display::hud_id id) noexcept
  { m_id = id; }

  heads_up_display::hud_id
  get_id() const
  { return m_id.value(); }

  private:
  std::optional<heads_up_display::hud_id> m_id;
}; // class mw::hud_component


class custom_hud: public hud_component {
  public:
  static void dummy_update(heads_up_display &) { }
  static void dummy_draw(rectangle &) { }
  static void dummy_click(const pt2d_i &) { }

  custom_hud()
  : m_on_update {dummy_update},
    m_on_draw {dummy_draw},
    m_on_click {dummy_click}
  { }

  template <typename Fn> void on_update(Fn fn) { m_on_update = fn; }
  template <typename Fn> void on_draw(Fn fn) { m_on_draw = fn; }
  template <typename Fn> void on_click(Fn fn) { m_on_update = fn; }

  void update(heads_up_display &hud) override { m_on_update(hud); }
  void draw(rectangle &box) const override { m_on_draw(box); }
  void click(const pt2d_i &at) override { m_on_click(at); }

  private:
  std::function<void(heads_up_display &hud)> m_on_update;
  std::function<void(rectangle &box)> m_on_draw;
  std::function<void(const pt2d_i &at)> m_on_click;
};


//template <typename GuiLayout>
//class linear_layout_hud: public hud_component {
  //public:
  //struct entry;
  //using entry_id = typename std::list<entry>::const_iterator;
  //struct entry {
    //component *gui;
    //std::function<void(linear_layout_hud &obs, entry_id)> on_update;
    //typename GuiLayout::entry_id layout_id;
  //};

  //linear_layout_hud(sdl_environment &sdl)
  //: m_layout {new GuiLayout {sdl}}
  //{ }

  //private:
  //std::list<entry> m_entries;
  //GuiLayout *m_layout;
//}; // class npc_observer_hud


} // namespace mw


#endif
