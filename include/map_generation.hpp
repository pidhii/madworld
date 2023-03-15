#ifndef MAP_GENERATION_HPP
#define MAP_GENERATION_HPP

#include "geometry.hpp"
#include "video_manager.hpp"


namespace mw {

/*
 * 1. Pick a random doorway cell g in G.
 * 2. Pick a random doorway cell h in H.
 * 3. Overlay them (Gij = g, Hij = h).
 * 4. Rotate rooms s.t. or(g) = -or(h).
 * 5. Move H along G untill a match, or the starting point reached. In the later
 *    case pick a different room and go to step 1.
 */

enum orientation {
  up       = 1 << 0,
  right    = 1 << 1,
  down     = 1 << 2,
  left     = 1 << 3,

}; // enum mw::orientation

enum cell_value {
  nothing = 0x00,
  issue   = 0x10,
  wall    = 0x20,
  doorway = 0x30,
}; // enum mw::cell_value

static constexpr uint8_t
orientation_mask = 0x0F;

static constexpr uint8_t
value_mask = 0xF0;

inline uint8_t
rotate_or_right(uint8_t c)
{
  const uint8_t oldor = c & orientation_mask;
  const uint8_t newor = (oldor << 1) | ((oldor & left) >> 3);
  return (c & ~orientation_mask) | (newor & orientation_mask);
}

inline uint8_t
rotate_or_left(uint8_t c)
{
  const uint8_t oldor = c & orientation_mask;
  const uint8_t newor = (oldor >> 1) | ((oldor & up) << 3);
  return (c & ~orientation_mask) | newor;
}

class carbon_grid {
  public:
  carbon_grid(int w, int h);

  carbon_grid(const carbon_grid &other);

  carbon_grid(carbon_grid &&other);

  ~carbon_grid();

  carbon_grid&
  operator = (const carbon_grid &other) noexcept;

  carbon_grid&
  operator = (carbon_grid &&other) noexcept;

  uint8_t
  get(const vec2d_i &uv) const noexcept;

  void
  set(const vec2d_i &uv, uint8_t c) noexcept;

  void
  shift(const vec2d_i &v) noexcept;

  void
  rotate_left(const vec2d_i &_fix) noexcept;

  void
  rotate_right(const vec2d_i &_fix) noexcept;

  void
  get_x_range(int &begin, int &end) const noexcept;

  void
  get_y_range(int &begin, int &end) const noexcept;

  private:
  bool
  _check_xy(const vec2d_i xy) const noexcept
  { return xy.x >= 0 and xy.x < m_w and xy.y >= 0 and xy.y < m_h; }

  vec2d_i
  _viewport(const vec2d_i &v) const noexcept
  { return m_rot(v) + m_offs; }

  vec2d_i
  _viewport_inv(const vec2d_i &v) const noexcept
  { return m_invrot(v - m_offs); }

  private:
  int m_w, m_h;
  vec2d_i m_offs;
  mat2d_i m_rot, m_invrot;
  int m_rotcnt;
  uint8_t* m_data;
}; // class mw::carbon_grid

void
draw_carbon_grid(sdl_environment &sdl, const mapping &viewport,
    const carbon_grid &cg);


class map_generator {
  public:
  map_generator(int w, int h)
  : m_w {w}, m_h {h},
    m_mg {w, h}
  { }

  const carbon_grid&
  get_main_grid() const noexcept
  { return m_mg; }

  uint8_t
  get(const vec2d_i &xy) const noexcept
  { return m_mg.get(xy); }

  bool
  is_within(const carbon_grid &cg) const noexcept;

  void
  blind_write(const carbon_grid &cg);

  void
  write(const carbon_grid &cg);

  struct overlay_data {
    int n_errors = 0;
    int n_ovl_doorways = 0;
    int n_new_walls = 0;
  }; // struct mw::map_generator::overlay_data
  void
  overlay(const carbon_grid &cg, overlay_data &ovdata);

  private:
  int m_w, m_h;
  carbon_grid m_mg;
}; // class mw::map_generator

} // namespace mw

#endif
