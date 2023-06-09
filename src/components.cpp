#include "gui/components.hpp"
#include "gui/utilities.hpp"
#include <algorithm>


mw::gui::label_base::label_base(sdl_environment &sdl, const sdl_string &str)
: component(sdl), m_str {str}
{ }

const mw::sdl_string&
mw::gui::label_base::get_string() const noexcept
{ return m_str; }

void
mw::gui::label_base::set_string(const sdl_string &str) noexcept
{
  m_str = str;
  send("updated", 0);
}

int
mw::gui::label_base::get_width() const
{
  texture_info texinfo;
  m_str.get_texture_info(m_sdl.get_renderer(), texinfo);
  return texinfo.w;
}

int
mw::gui::label_base::get_height() const
{
  texture_info texinfo;
  m_str.get_texture_info(m_sdl.get_renderer(), texinfo);
  return texinfo.h;
}

void
mw::gui::label_base::draw(const pt2d_i &at) const
{ m_str.draw(m_sdl.get_renderer(), at); }

int
mw::gui::label_base::send(const std::string &what, signal_data data)
{ return 0; }


void
mw::gui::detail::_draw_box(sdl_environment &sdl, const pt2d_i &at, int w, int h,
      const std::optional<color_t> &fill_color,
      const std::optional<color_t> &border_color)
{
  int16_t xs[4] = {short(at.x), short(at.x+w), short(at.x+w), short(at.x)};
  int16_t ys[4] = {short(at.y), short(at.y)  , short(at.y+h), short(at.y+h)};

  if (fill_color.has_value())
    filledPolygonColor(sdl.get_renderer(), xs, ys, 4, fill_color.value());
  if (border_color.has_value())
    aapolygonColor(sdl.get_renderer(), xs, ys, 4, border_color.value());
}


mw::gui::text_entry_base::text_entry_base(sdl_environment &sdl, TTF_Font *font, color_t fg,
    int width, int height)
: component(sdl), m_font {font}, m_fg {fg}, m_width {width}, m_height {height},
  m_cursor {sdl_string::blended(font, "_", fg)},
  m_enable_cursor {false}
{ }

void
mw::gui::text_entry_base::draw(const pt2d_i &at) const
{
  if (not m_str.has_value())
  {
    if (not m_text.empty())
      m_str = sdl_string::blended_wrapped(m_font, m_text, m_fg, m_width);
  }

  const int x0 = at.x, y0 = at.y;
  int x = x0, y = y0;
  texture_info texinfo;
  texinfo.w = texinfo.h = 0;
  if (m_str.has_value())
  {
    m_str.value().draw(m_sdl.get_renderer(), at, texinfo);
    x += texinfo.w;
  }

  if (m_enable_cursor)
  {
    texture_info cursinfo;
    m_cursor.get_texture_info(m_sdl.get_renderer(), cursinfo);
    if (texinfo.w + cursinfo.w > m_width)
    {
      x = x0;
      y += texinfo.h;
    }
    m_cursor.draw(m_sdl.get_renderer(), {x, y});
  }
}

int
mw::gui::text_entry_base::send(const std::string &what, signal_data data)
{
  if (what == "keydown")
  {
    if (isprint(data.c))
    {
      m_text.push_back(data.c);
      m_str = std::nullopt;
    }
    else if (data.i == SDLK_BACKSPACE)
    {
      if (not m_text.empty())
      {
        m_text.pop_back();
        m_str = std::nullopt;
      }
    }
    else if (data.i == SDLK_RETURN)
    {
      send("<text_entry>:return", 0);
    }
  }
  return 0;
}

void
mw::gui::text_entry_base::set_text(const std::string &text)
{
  m_text = text;
  m_str = std::nullopt;
}


mw::gui::message_log_base::message_log_base(sdl_environment &sdl, TTF_Font *font,
    color_t fg, int width, int height)
: component(sdl), m_font {font}, m_width {width}, m_height {height},
  m_strfac {font, fg}
{
  m_strfac.set_wrap(width);
}

void
mw::gui::message_log_base::add_message(const std::string &msg)
{ m_msgs.push_back(m_strfac(msg)); }

void
mw::gui::message_log_base::draw(const pt2d_i &at) const
{
  SDL_Rect oldclip;
  SDL_RenderGetClipRect(m_sdl.get_renderer(), &oldclip);

  SDL_Rect clip;
  clip.x = at.x;
  clip.y = at.y;
  clip.w = m_width;
  clip.h = m_height;
  SDL_RenderSetClipRect(m_sdl.get_renderer(), &clip);

  int offs = m_height - 1;
  for (auto it = m_msgs.rbegin(); it != m_msgs.rend(); ++it)
  {
    const sdl_string &msg = *it;

    texture_info texinfo;
    msg.get_texture_info(m_sdl.get_renderer(), texinfo);
    offs -= texinfo.h;
    msg.draw(m_sdl.get_renderer(), {at.x, at.y + offs});

    if (offs <= 0)
      break;
  }

  SDL_RenderSetClipRect(m_sdl.get_renderer(), NULL);
}


mw::gui::linear_layout_base::~linear_layout_base()
{
  for (const component *c : m_list)
    delete c;
}

mw::gui::linear_layout_base::entry_id
mw::gui::linear_layout_base::find_component(const component *c) const noexcept
{ return std::find(m_list.begin(), m_list.end(), c); }

int
mw::gui::linear_layout_base::get_width() const
{
  int res = 0;
  pt2d_i cpos {0, 0};
  for (const component *c : m_list)
  {
    res = std::max(res, cpos.x + c->get_width());
    cpos = _next(cpos, c);
  }
  return res;
}

int
mw::gui::linear_layout_base::get_height() const
{
  int res = 0;
  pt2d_i cpos {0, 0};
  for (const component *c : m_list)
  {
    res = std::max(res, cpos.y + c->get_height());
    cpos = _next(cpos, c);
  }
  return res;
}

void
mw::gui::linear_layout_base::draw(const pt2d_i &at) const
{
  pt2d_i cpos = at;
  for (const component *c : m_list)
  {
    c->draw(cpos);
    cpos = _next(cpos, c);
  }
}

int
mw::gui::linear_layout_base::send(const std::string &what,
    signal_data data)
{
  if (what == "hover-begin" or what == "hover")
  {
    const vec2d_i hover = unpack_hover(data.u64);
    pt2d_i start {0, 0};
    for (component *c : m_list)
    {
      const pt2d_i end {start.x + c->get_width(), start.y + c->get_height()};
      if (start.x < hover.x and hover.x < end.x and
          start.y < hover.y and hover.y < end.y)
      {
        if (m_old_hover.has_value())
        {
          if (m_old_hover.value() == c)
          {
            c->send("hover", pack_hover({start.x, start.y}, {hover.x, hover.y}));
            return 0;
          }
          else
          {
            m_old_hover.value()->send("hover-end", 0);
            c->send("hover-begin", pack_hover({start.x, start.y}, {hover.x, hover.y}));
            m_old_hover = c;
            return 0;
          }
        }
        else
        {
          c->send("hover-begin", pack_hover({start.x, start.y}, {hover.x, hover.y}));
          m_old_hover = c;
          return 0;
        }
      }
      start = _next(start, c);
    }
    // loop finished normally => nothing is being hovered
    if (m_old_hover.has_value())
    {
      m_old_hover.value()->send("hover-end", 0);
      m_old_hover = std::nullopt;
      return 0;
    }
  }
  else if (what == "hover-end")
  {
    if (m_old_hover.has_value())
    {
      m_old_hover.value()->send("hover-end", 0);
      m_old_hover = std::nullopt;
      return 0;
    }
  }
  else if (m_old_hover.has_value())
    return m_old_hover.value()->send(what, data);

  return 0;
}

void
mw::gui::radio_entry_base::draw(const pt2d_i &at) const
{
  const color_rgba rgba {m_color};
  SDL_SetRenderDrawColor(m_sdl.get_renderer(), rgba.r, rgba.g, rgba.b, rgba.a);

  SDL_Rect rect;
  rect.x = at.x;
  rect.y = at.y;
  rect.w = m_size;
  rect.h = m_size;
  if (m_is_on)
    SDL_RenderFillRect(m_sdl.get_renderer(), &rect);
  else
    SDL_RenderDrawRect(m_sdl.get_renderer(), &rect);
}


mw::gui::radio_entry_base::~radio_entry_base()
{
  if (m_group)
    m_group->erase(m_group_id);
}

void
mw::gui::add_to_radio_group(radio_group_ptr group, radio_entry *entry)
{
  auto it = group->emplace(entry).first;

  if (entry->m_group)
    entry->m_group->erase(entry->m_group_id);

  entry->m_group = group;
  entry->m_group_id = it;
}

