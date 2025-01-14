#include "map_generation.hpp"
#include "gui/sdl_string.hpp"

#include <cstring>
#include <algorithm>


mw::carbon_grid::carbon_grid(int w, int h)
: m_w {w}, m_h {h},
  m_offs {0, 0},
  m_rot {1, 0, 0, 1},
  m_invrot {1, 0, 0, 1},
  m_rotcnt {0},
  m_data {new uint8_t [w*h]}
{ std::memset(m_data, 0, sizeof(uint8_t)*w*h); }

mw::carbon_grid::carbon_grid(const carbon_grid &other)
: m_w {other.m_w}, m_h {other.m_h},
  m_offs {other.m_offs},
  m_rot {other.m_rot},
  m_invrot {other.m_invrot},
  m_rotcnt {0},
  m_data {new uint8_t [m_w*m_h]}
{ std::memcpy(m_data, other.m_data, sizeof(uint8_t)*m_w*m_h); }

mw::carbon_grid::carbon_grid(carbon_grid &&other)
: m_w {other.m_w}, m_h {other.m_h},
  m_offs {other.m_offs},
  m_rot {other.m_rot},
  m_invrot {other.m_invrot},
  m_rotcnt {0},
  m_data {other.m_data}
{ other.m_data = nullptr; }

mw::carbon_grid::~carbon_grid()
{ delete[] m_data; }

mw::carbon_grid&
mw::carbon_grid::operator = (const carbon_grid &other) noexcept
{
  if (this == &other)
    return *this;
  delete[] m_data;
  new (this) mw::carbon_grid {other};
  return *this;
}

mw::carbon_grid&
mw::carbon_grid::operator = (carbon_grid &&other) noexcept
{
  if (this == &other)
    return *this;
  delete[] m_data;
  new (this) mw::carbon_grid {other};
  return *this;
}

uint8_t
mw::carbon_grid::get(const vec2d_i &uv) const noexcept
{
  const vec2d_i xy = _viewport(uv);
  if (_check_xy(xy))
  {
    uint8_t c = m_data[xy.y + xy.x*m_h];
    // apply left rotations
    for (int i = m_rotcnt; i < 0; ++i)
      c = rotate_or_left(c);
    // apply right rotations
    for (int i = m_rotcnt; i > 0; --i)
      c = rotate_or_right(c);
    return c;
  }
  else
    return 0;
}

void
mw::carbon_grid::set(const vec2d_i &uv, uint8_t c) noexcept
{
  const vec2d_i xy = _viewport(uv);
  if (_check_xy(xy))
  {
    // undo left rotations
    for (int i = m_rotcnt; i < 0; ++i)
      c = rotate_or_right(c);
    // undo right rotations
    for (int i = m_rotcnt; i > 0; --i)
      c = rotate_or_left(c);
    m_data[xy.y + xy.x*m_h] = c;
  }
}

void
mw::carbon_grid::shift(const vec2d_i &v) noexcept
{ m_offs = m_offs - m_rot(v); }

void
mw::carbon_grid::rotate_left(const vec2d_i &_fix) noexcept
{
  static constexpr mat2d_i R {0, 1, -1, 0};
  static constexpr mat2d_i L {0, -1, 1, 0};
  const vec2d_i fix = _viewport(_fix);
  const vec2d_i d {fix.x + fix.y, fix.y - fix.x};
  m_rot = L*m_rot;
  m_invrot = m_invrot*R;
  m_offs = L(m_offs) + d;
  m_rotcnt = (m_rotcnt - 1) % 4;
}

void
mw::carbon_grid::rotate_right(const vec2d_i &_fix) noexcept
{
  static constexpr mat2d_i R {0, 1, -1, 0};
  static constexpr mat2d_i L {0, -1, 1, 0};
  const vec2d_i fix = _viewport(_fix);
  const vec2d_i d {fix.x - fix.y, fix.x + fix.y};
  m_rot = R*m_rot;
  m_invrot = m_invrot*L;
  m_offs = R(m_offs) + d;
  m_rotcnt = (m_rotcnt + 1) % 4;
}

void
mw::carbon_grid::get_x_range(int &begin, int &end) const noexcept
{
  const vec2d_i
    a = _viewport_inv({0, 0}),
    b = _viewport_inv({m_w-1, m_h-1});
  std::tie(begin, end) = std::minmax(a.x, b.x);
  end += 1;
}

void
mw::carbon_grid::get_y_range(int &begin, int &end) const noexcept
{
  const vec2d_i
    a = _viewport_inv({0, 0}),
    b = _viewport_inv({m_w-1, m_h-1});
  std::tie(begin, end) = std::minmax(a.y, b.y);
  end += 1;
}


void
mw::draw_carbon_grid(sdl_environment &sdl, const mapping &viewport,
    const carbon_grid &cg)
{
  // FIXME
  static TTF_Font *tiny_font =
    TTF_OpenFont(mw::video_config::instance().get_config().font.path.c_str(),10);

  mw::sdl_string_factory or_strfac {tiny_font};
  or_strfac.set_fg_color(0xFF000000);
  auto make_or_string = [&] (uint8_t c) -> mw::sdl_string {
    std::string str;
    if (mw::up & c) str += 'u';
    if (mw::right & c) str += 'r';
    if (mw::down & c) str += 'd';
    if (mw::left & c) str += 'l';
    return or_strfac(str);
  };

  int winw, winh;
  SDL_GetWindowSize(sdl.get_window(), &winw, &winh);

  int xbegin, xend, ybegin, yend;
  cg.get_x_range(xbegin, xend);
  cg.get_y_range(ybegin, yend);
  for (int i = xbegin; i < xend; ++i)
  {
    for (int j = ybegin; j < yend; ++j)
    {
      if (cg.get({i, j}))
      {
        switch (cg.get({i, j}) & value_mask)
        {
          case issue:
            SDL_SetRenderDrawColor(sdl.get_renderer(), 0xAA, 0x33, 0x33, 0x77);
            break;
          case wall:
            SDL_SetRenderDrawColor(sdl.get_renderer(), 0x33, 0xAA, 0x33, 0x77);
            break;
          case doorway:
            SDL_SetRenderDrawColor(sdl.get_renderer(), 0xAA, 0xAA, 0x33, 0x77);
            break;
        }
        SDL_Rect rect;
        rect.x = viewport({double(i), double(j)}).x;
        rect.y = viewport({double(i), double(j)}).y;
        rect.w = viewport.get_x_scale();
        rect.h = viewport.get_y_scale();
        SDL_RenderFillRect(sdl.get_renderer(), &rect);
        if (cg.get({i, j}) & orientation_mask)
        {
          make_or_string(cg.get({i, j}))
            .draw(sdl.get_renderer(), {rect.x+2, rect.y+2});
        }
      }
    }
  }

  SDL_SetRenderDrawColor(sdl.get_renderer(), 0x33, 0x33, 0xAA, 0x77);
  SDL_Rect rect;
  rect.x = viewport({double(xbegin), double(ybegin)}).x;
  rect.y = viewport({double(xbegin), double(ybegin)}).y;
  rect.w = (xend - xbegin)*viewport.get_x_scale();
  rect.h = (yend - ybegin)*viewport.get_y_scale();
  SDL_RenderDrawRect(sdl.get_renderer(), &rect);
}
