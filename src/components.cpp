#include "gui/components.hpp"
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

std::pair<int, int>
mw::gui::label_base::get_dimentions() const
{
  texture_info texinfo;
  m_str.get_texture_info(m_sdl.get_renderer(), texinfo);
  return {texinfo.w, texinfo.h};
}

void
mw::gui::label_base::draw(const pt2d_i &at) const
{ m_str.draw(m_sdl.get_renderer(), at); }

int
mw::gui::label_base::send(const std::string &what, const std::any &data)
{ return 0; }


void
mw::gui::detail::_draw_box(sdl_environment &sdl, pt2d_i at, int w, int h,
                           int lw, const std::optional<color_t> &fill_color,
                           const std::optional<color_t> &border_color)
{
  if (fill_color.has_value())
  {
    const int16_t xs[4] = {short(at.x), short(at.x+w-1), short(at.x+w-1), short(at.x)};
    const int16_t ys[4] = {short(at.y), short(at.y)  , short(at.y+h-1), short(at.y+h-1)};
    filledPolygonColor(sdl.get_renderer(), xs, ys, 4, fill_color.value());
  }

  if (border_color.has_value())
  {
    while (lw-- > 0)
    {
      const int16_t xs[4] = {short(at.x), short(at.x+w-1), short(at.x+w-1), short(at.x)};
      const int16_t ys[4] = {short(at.y), short(at.y)  , short(at.y+h-1), short(at.y+h-1)};
      if (border_color.has_value())
        aapolygonColor(sdl.get_renderer(), xs, ys, 4, border_color.value());
      at.x += 1;
      at.y += 1;
      w -= 2;
      h -= 2;
    }
  }
}

mw::gui::text_entry_base::text_entry_base(sdl_environment &sdl,
                                          const sdl_string_factory &strfac,
                                          int width, int height)
: component(sdl),
  m_strfac {strfac},
  m_width {width},
  m_height {height},
  m_cursor {strfac("_")},
  m_enable_cursor {false}
{ }

void
mw::gui::text_entry_base::draw(const pt2d_i &at) const
{
  if (not m_str.has_value())
  {
    if (not m_text.empty())
    {
      if (m_width > 0)
        m_str = m_strfac.wrap(m_width)(m_text);
      else
        m_str = m_strfac(m_text);
    }
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
    if (m_width > 0 and texinfo.w + cursinfo.w > m_width)
    {
      x = x0;
      y += texinfo.h;
    }
    m_cursor.draw(m_sdl.get_renderer(), {x, y});
  }
}

int
mw::gui::text_entry_base::send(const std::string &what, const std::any &data)
{
  if (what == "keydown")
  {
    if (isprint((unsigned char)std::any_cast<int>(data)))
    {
      m_text.push_back(std::any_cast<int>(data));
      m_str = std::nullopt;
    }
    else if (std::any_cast<int>(data) == SDLK_BACKSPACE)
    {
      if (not m_text.empty())
      {
        m_text.pop_back();
        m_str = std::nullopt;
      }
    }
    else if (std::any_cast<int>(data) == SDLK_RETURN)
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

mw::gui::message_log_base::message_log_base(sdl_environment &sdl,
                                            const ttf_font &font, color_t fg,
                                            int width, int height)
: component(sdl),
  m_font {font},
  m_width {width},
  m_height {height},
  m_strfac {font, fg}
{ m_strfac.set_wrap(width); }

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

std::pair<int, int>
mw::gui::linear_layout_base::get_dimentions() const
{
  int resw = 0, resh = 0;
  pt2d_i cpos {0, 0};
  for (const component *c : m_list)
  {
    const auto [w, h] = c->get_dimentions();
    resw = std::max(resw, cpos.x + w);
    resh = std::max(resh, cpos.y + h);
    cpos = _next(cpos, w, h);
  }
  return {resw, resh};
}

void
mw::gui::linear_layout_base::draw(const pt2d_i &at) const
{
  pt2d_i cpos = at;
  for (const component *c : m_list)
  {
    const auto [w, h] = c->get_dimentions();
    c->draw(cpos);
    cpos = _next(cpos, w, h);
  }
}

int
mw::gui::linear_layout_base::send(const std::string &what, const std::any &data)
{
  if (what == "hover-begin" or what == "hover")
  {
    const vec2d_i hover = std::any_cast<vec2d_i>(data);
    pt2d_i start {0, 0};
    for (component *c : m_list)
    {
      const auto [w, h] = c->get_dimentions();
      const pt2d_i end {start.x + w, start.y + h};
      if (start.x < hover.x and hover.x < end.x and
          start.y < hover.y and hover.y < end.y)
      {
        if (m_old_hover.has_value())
        {
          if (m_old_hover.value() == c)
          {
            c->send("hover", std::any(pt2d_i(hover.x, hover.y) - pt2d_i(start.x, start.y)));
            return 0;
          }
          else
          {
            m_old_hover.value()->send("hover-end", 0);
            c->send("hover-begin", std::any(pt2d_i(hover.x, hover.y) - pt2d_i(start.x, start.y)));
            m_old_hover = c;
            return 0;
          }
        }
        else
        {
          c->send("hover-begin", std::any(pt2d_i(hover.x, hover.y) - pt2d_i(start.x, start.y)));
          m_old_hover = c;
          return 0;
        }
      }
      start = _next(start, w, h);
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

