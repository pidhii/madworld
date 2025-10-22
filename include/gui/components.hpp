#ifndef GUI_COMPONENTS_HPP
#define GUI_COMPONENTS_HPP

#ifdef error
# undef error
#endif
#include <sol/sol.hpp>
#define error(fmt, ...)                                                        \
  _error("%s:%d " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)

#include "geometry.hpp"
#include "common.hpp"
#include "gui/sdl_string.hpp"
#include "sdl_environment.hpp"
#include "gui/ttf_font.hpp"
#include "logging.h"
#include "video_manager.hpp"

#include <SDL2/SDL2_gfxPrimitives.h>

#include <optional>
#include <list>
#include <map>
#include <functional>
#include <set>
#include <any>


namespace mw {

class ui_manager;

inline namespace gui {

using signal_data = qword;

struct component {
  component(sdl_environment &sdl): m_sdl {sdl} { }
  component(): component {mw::video_manager::instance().get_sdl()} { }

  virtual
  ~component() = default;

  virtual int
  get_width() const
  { return get_dimentions().first; }

  virtual int
  get_height() const
  { return get_dimentions().second; }

  virtual std::pair<int, int>
  get_dimentions() const
  { return {get_width(), get_height()}; }

  virtual void
  draw(const pt2d_i &at) const = 0;

  virtual int
  send(const std::string &what, const std::any &data = std::any()) = 0;

  virtual void
  lua_export(sol::table &self)
  { }

  protected:
  sdl_environment &m_sdl;
};


#define MWGUI_CALLBACK_ARGS auto *self, const std::string &what, const std::any &data

namespace detail {

  void
  _draw_box(sdl_environment &sdl, pt2d_i at, int w, int h, int lw,
      const std::optional<color_t> &fill_color,
      const std::optional<color_t> &border_color);

  struct _forwarder {
    explicit
    _forwarder(component *_c): c {_c} { }
    int
    operator () (void*, const std::string &what, const std::any &data)
    { return c->send(what, data); }
    component *c;
  };

} // namespace mw::gui::detail

template <typename Base>
class enriched: public Base {
  public:
  template <typename ...Args>
  enriched(Args&& ...args)
  : Base(std::forward<Args>(args)...),
    m_left_pad {0},
    m_right_pad {0},
    m_top_pad {0},
    m_bottom_pad {0},
    m_border_width {0},
    m_ignore_signals {false}
  { }

  virtual
  ~enriched() override = default;

  enriched*
  set_left_margin(int val) noexcept
  { m_left_pad = val; return this; }

  enriched*
  set_right_margin(int val) noexcept
  { m_right_pad = val; return this; }

  enriched*
  set_top_margin(int val) noexcept
  { m_top_pad = val; return this; }

  enriched*
  set_bottom_margin(int val) noexcept
  { m_bottom_pad = val; return this; }

  enriched*
  set_fill_color(color_t color) noexcept
  { m_fill_color = color; return this; }

  enriched*
  set_border_color(color_t color) noexcept
  { m_border_color = color; return this; }

  enriched*
  set_border_width(int w) noexcept
  { m_border_width = w; return this; }

  enriched*
  set_min_width(int w) noexcept
  { m_min_width = w; return this; }

  enriched*
  set_min_height(int h) noexcept
  { m_min_height = h; return this; }

  enriched*
  ignore_signals(bool val) noexcept
  { m_ignore_signals = val; return this; }

  template <typename Callback>
  enriched*
  on(const std::string &what, Callback callback)
  { m_callbacks.emplace(what, std::forward<Callback>(callback)); return this; }

  enriched*
  forward(const std::string &what, component *c)
  { return on(what, detail::_forwarder {c}); }

  virtual std::pair<int, int>
  get_dimentions() const override final
  {
    auto [w, h] = Base::get_dimentions();
    // apply padding
    w += m_left_pad + m_right_pad + m_border_width*2;
    h += m_top_pad + m_bottom_pad + m_border_width*2;
    // apply minimal width and hight
    if (m_min_width.has_value())
      w = std::max(m_min_width.value(), w);
    if (m_min_height.has_value())
      h = std::max(m_min_height.value(), h);
    return {w, h};
  }

  int
  send(const std::string &what, const std::any &data) override final
  {
    if (m_ignore_signals)
      return 0;

    if (what == "hover" or what == "hover-begin" or what == "clicked")
    {
      try {
        const vec2d_i oldval = std::any_cast<vec2d_i>(data);
        const vec2d_i newval = oldval - vec2d_i(m_left_pad + m_border_width,
                                                m_top_pad + m_border_width);
        return _send(what, std::any(newval));
      }
      catch (const std::bad_any_cast&)
      {
        return _send(what, std::any(vec2d_i(0, 0)));
      }
    }
    else
      return _send(what, data);
  }

  void
  draw(const pt2d_i &at) const override final
  {
    const auto [w, h] = get_dimentions();
    detail::_draw_box(Base::m_sdl, at, w, h, 0, m_fill_color,
                      std::nullopt);
    Base::draw(at + vec2d_i(m_left_pad+m_border_width, m_top_pad+m_border_width));
    detail::_draw_box(Base::m_sdl, at, w, h, m_border_width, std::nullopt,
                      m_border_color);
  }

  void
  lua_export(sol::table &self) override
  {
    Base::lua_export(self);

    #define E(method_name) \
      self.set_function(#method_name, &enriched::method_name, this);
    E(get_dimentions)
    E(set_left_margin)
    E(set_right_margin)
    E(set_top_margin)
    E(set_bottom_margin)
    E(set_border_color)
    E(set_border_width)
    E(set_fill_color)
    E(set_min_width)
    E(set_min_height)
    E(ignore_signals)
    self.set("on", [this](std::string what, sol::function cb) {
      on(what, [this, cb](MWGUI_CALLBACK_ARGS) { cb(this); return 0; });
    });
    #undef E
  }

  private:
  int
  _send(const std::string &what, const std::any &data)
  {
    auto callback_it = m_callbacks.find(what);
    if (callback_it != m_callbacks.end())
      return callback_it->second(this, what, data);
    else
      return Base::send(what, data);
  }

  private:
  std::map<
    std::string,
    std::function<int(enriched*, const std::string&, const std::any&)>
  > m_callbacks;
  int m_left_pad, m_right_pad, m_top_pad, m_bottom_pad;
  std::optional<int> m_min_width, m_min_height;
  std::optional<color_t> m_fill_color, m_border_color;
  int m_border_width;
  bool m_ignore_signals;
};


class label_base: public component {
  public:
  label_base(sdl_environment &sdl, const sdl_string &str);
  label_base(const sdl_string &str)
  : label_base {mw::video_manager::instance().get_sdl(), str}
  { }

  virtual
  ~label_base() override = default;

  const sdl_string&
  get_string() const noexcept;

  void
  set_string(const sdl_string &str) noexcept;

  virtual int
  get_width() const override
  { return get_dimentions().first; }

  virtual int
  get_height() const override
  { return get_dimentions().second; }

  virtual std::pair<int, int>
  get_dimentions() const override;

  virtual void
  draw(const pt2d_i &at) const override;

  virtual int
  send(const std::string &what, const std::any &data) override;

  private:
  sdl_string m_str;
}; // class mw::gui::label_base
using label = enriched<label_base>;


class button_base: public component {
  public:
  button_base(sdl_environment &sdl, component *normal, component *hover)
  : component(sdl),
    m_ishover {false},
    m_normalc {normal},
    m_hoverc {hover}
  { }

  button_base(component *normal, component *hover)
  : button_base {mw::video_manager::instance().get_sdl(), normal, hover}
  { }

  virtual
  ~button_base() override
  { delete m_normalc; delete m_hoverc; }

  void
  set_hover(bool val) noexcept
  { m_ishover = val; }

  virtual std::pair<int, int>
  get_dimentions() const override
  { return m_ishover ? m_hoverc->get_dimentions() : m_normalc->get_dimentions(); }

  virtual void
  draw(const pt2d_i &at) const override
  { return m_ishover ? m_hoverc->draw(at) : m_normalc->draw(at); }

  virtual int
  send(const std::string &what, const std::any &data) override
  {
    if (what == "hover-begin" or what == "hover")
    {
      m_ishover = true;
      return 0;
    }
    else if (what == "hover-end")
    {
      m_ishover = false;
      return 0;
    }
    else if (m_ishover)
      return m_hoverc->send(what, data);
    else
      return m_normalc->send(what, data);
  }

  private:
  bool m_ishover;
  component *m_normalc;
  component *m_hoverc;
};
using button = enriched<button_base>;


class text_entry_base: public component {
  public:
  text_entry_base(sdl_environment &sdl, const sdl_string_factory &strfac,
                  int width, int height);

  text_entry_base(const sdl_string_factory &strfac, int width, int height)
  : text_entry_base {video_manager::instance().get_sdl(), strfac, width, height}
  { }

  virtual
  ~text_entry_base() override = default;

  const std::string&
  get_text() const noexcept
  { return m_text; }

  void
  set_text(const std::string &text);

  void
  set_cursor(bool enable) noexcept
  { m_enable_cursor = enable; }

  virtual int
  get_width() const override
  {
    // Return specified fixed with if set
    if (m_width > 0)
      return m_width;

    texture_info cursinfo;
    m_cursor.get_texture_info(m_sdl.get_renderer(), cursinfo);

    // Otherwize determine width from the text texture
    texture_info texinfo;
    if (m_str)
      m_str->get_texture_info(m_sdl.get_renderer(), texinfo);
    else
      m_strfac(m_text).get_texture_info(m_sdl.get_renderer(), texinfo);
    return texinfo.w + cursinfo.w;
  }

  virtual int
  get_height() const override
  { return m_height; }

  virtual void
  draw(const pt2d_i &at) const override;

  virtual int
  send(const std::string &what, const std::any &data) override;

  private:
  sdl_string_factory m_strfac;
  int m_width, m_height;
  std::string m_text;
  sdl_string m_cursor;
  bool m_enable_cursor;
  mutable std::optional<sdl_string> m_str;
}; // class mw::gui::text_entry_base
using text_entry = enriched<text_entry_base>;


class circular_stack_base: public component {
  public:
  template <typename Begin, typename End>
  circular_stack_base(sdl_environment &sdl, Begin begin, End end)
  : component(sdl), m_components {begin, end}, m_active_index {0}
  { }

  template <typename Begin, typename End>
  circular_stack_base(Begin begin, End end)
  : circular_stack_base {video_manager::instance().get_sdl(), begin, end}
  { }

  virtual ~circular_stack_base() override = default;

  virtual int
  get_width() const override
  { return get_active_component()->get_width(); }

  virtual int
  get_height() const override
  { return get_active_component()->get_height(); }

  virtual void
  draw(const pt2d_i &at) const override
  { return get_active_component()->draw(at); }

  virtual int
  send(const std::string &what, const std::any &data) override
  { return get_active_component()->send(what, data); }

  component*
  get_active_component() const noexcept
  { return m_components[m_active_index % m_components.size()].get(); }

  void
  switch_to_next_component() noexcept
  { m_active_index = (m_active_index + 1) % m_components.size(); }

  void
  switch_to_prev_component() noexcept
  { m_active_index = (m_active_index + m_components.size() - 1) % m_components.size(); }

  virtual void
  lua_export(sol::table &self) override
  {
    #define E(method_name) \
      self.set_function(#method_name, &circular_stack_base::method_name, this);
    E(get_active_component)
    E(switch_to_next_component)
    E(switch_to_prev_component)
    #undef E
  }

  private:
  std::vector<std::unique_ptr<component>> m_components;
  int m_active_index;
};
using circular_stack = enriched<circular_stack_base>;


class message_log_base: public component {
  public:
  message_log_base(sdl_environment &sdl, const ttf_font &font, color_t fg,
                   int width, int height);

  virtual
  ~message_log_base() override = default;

  void
  add_message(const std::string &msg);

  using message_iterator = std::list<sdl_string>::const_iterator;

  message_iterator
  begin() const noexcept
  { return m_msgs.begin(); }

  message_iterator
  end() const noexcept
  { return m_msgs.end(); }

  virtual int
  get_width() const override
  { return m_width; }

  virtual int
  get_height() const override
  { return m_height; }

  virtual void
  draw(const pt2d_i &at) const override;

  virtual int
  send(const std::string &, const std::any &) override
  { return 0; }

  private:
  ttf_font m_font;
  int m_width, m_height;
  sdl_string_factory m_strfac;
  std::list<sdl_string> m_msgs;
}; // class mw::gui::message_log_base
using message_log = enriched<message_log_base>;


class linear_layout_base: public component {
  public:
  using entry_id =
    identifier<std::list<component*>::const_iterator, linear_layout_base>;

  using component::component;

  virtual
  ~linear_layout_base() override;

  entry_id
  add_component(component *c) noexcept
  { m_list.push_back(c); return {--m_list.end()}; }

  entry_id
  add_component_before(entry_id id, component *c)
  { return {m_list.insert(id, c)}; }

  entry_id
  add_component_after(entry_id id, component *c)
  { return {m_list.insert(std::next(id.get()), c)}; }

  void
  remove_entry(entry_id id)
  {
    if (m_old_hover == *id.get())
      m_old_hover = std::nullopt;
    delete *id.get();
    m_list.erase(id);
  }

  component*
  get_component(entry_id id) const
  { return *id.get(); }

  void
  clear()
  {
    m_old_hover = std::nullopt;
    for (component *c : m_list)
      delete c;
    m_list.clear();
  }

  entry_id
  find_component(const component *c) const noexcept;

  entry_id
  begin() const noexcept
  { return m_list.begin(); }

  entry_id
  end() const noexcept
  { return m_list.end(); }

  entry_id
  get_last_entry() const noexcept
  { return --m_list.end(); }

  virtual int
  get_width() const override
  { return get_dimentions().first; }

  virtual int
  get_height() const override
  { return get_dimentions().second; }

  virtual std::pair<int, int>
  get_dimentions() const override;

  virtual void
  draw(const pt2d_i &at) const override;

  virtual int
  send(const std::string &what, const std::any &data) override;

  protected:
  virtual pt2d_i
  _next(const pt2d_i &prevpos, int prevw, int prevh) const = 0;

  private:
  std::list<component*> m_list;
  std::optional<component*> m_old_hover;
};
using linear_layout = enriched<linear_layout_base>;


static inline void
add_component(component *container, component *child)
{
  if (linear_layout *ll = dynamic_cast<linear_layout*>(container))
    ll->add_component(child);
  else
    throw std::runtime_error {"not a linear layout"};
}

static inline void
add_component_after(component *container, linear_layout_base::entry_id id,
                    component *child)
{
  if (linear_layout *ll = dynamic_cast<linear_layout*>(container))
    ll->add_component_after(id, child);
  else
    throw std::runtime_error {"not a linear layout"};
}

static inline void
add_component_before(component *container, linear_layout_base::entry_id id,
                    component *child)
{
  if (linear_layout *ll = dynamic_cast<linear_layout*>(container))
    ll->add_component_before(id, child);
  else
    throw std::runtime_error {"not a linear layout"};
}




class horisontal_layout: public linear_layout {
  public:
  using linear_layout::linear_layout;
  virtual ~horisontal_layout() override = default;

  private:
  pt2d_i
  _next(const pt2d_i &prevpos, int prevw, int prevh) const override
  { return {prevpos.x + prevw, prevpos.y}; }
}; // mw::gui::horisontal_layout


class vertical_layout: public linear_layout {
  public:
  using linear_layout::linear_layout;
  virtual ~vertical_layout() override = default;

  private:
  pt2d_i
  _next(const pt2d_i &prevpos, int prevw, int prevh) const override
  { return {prevpos.x, prevpos.y + prevh}; }
}; // mw::gui::vertical_layout


class padding final: public component {
  public:
  padding(sdl_environment &sdl, int width, int height)
  : component(sdl), m_width {width}, m_height {height}
  { }

  int
  get_width() const override
  { return m_width; }

  int
  get_height() const override
  { return m_height; }

  void
  draw(const pt2d_i &) const override
  { }

  int
  send(const std::string &, const std::any &) override
  { return 0; }

  private:
  int m_width, m_height;
};


struct radio_entry_base;
using radio_entry = enriched<radio_entry_base>;

using radio_group_ptr = std::shared_ptr<std::set<radio_entry*>>;

struct radio_entry_base: public component {
  radio_entry_base(sdl_environment &sdl, color_t color, int size)
  : component(sdl),
    m_color {color},
    m_size {size},
    m_is_on {false}
  { }

  ~radio_entry_base() override;

  void
  set_state(bool val) noexcept
  { m_is_on = val; }

  bool
  get_state() const noexcept
  { return m_is_on; }

  virtual int
  get_width() const override
  { return m_size; }

  virtual int
  get_height() const override
  { return m_size; }

  virtual void
  draw(const pt2d_i &at) const override;

  virtual int
  send(const std::string &, const std::any &) override
  { return 0; }

  private:
  color_t m_color;
  int m_size;
  bool m_is_on;

  radio_group_ptr m_group;
  std::set<radio_entry*>::const_iterator m_group_id;

  friend void add_to_radio_group(radio_group_ptr group, radio_entry *entry);
};

void
add_to_radio_group(radio_group_ptr group, radio_entry *entry);

inline radio_group_ptr
make_radio_group()
{ return std::make_shared<std::set<radio_entry*>>(); }

} // namespace mw::gui
} // namespace mw

#endif
