#ifndef WALLS_HPP
#define WALLS_HPP

#include "object.hpp"
#include "area_map.hpp"
#include "common.hpp"
#include "vision.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <vector>

namespace mw {


namespace basic_wall_impl {
void
_draw_walls(const area_map &map, const std::vector<pt2d_d> &vertices,
    color_t color);
} // namespace basic_wall_impl


// TODO: remove this template-parameter
template <typename EndsSpec = solid_ends>
class basic_wall: public phys_obstacle, public vis_obstacle {
  public:
  basic_wall(const std::vector<pt2d_d> &vertices)
  : m_vertices {vertices},
    m_color {0xFFFFFFFF}
  { }

  virtual void
  set_color(color_t color) noexcept
  { m_color = color; }

  const std::vector<pt2d_d>&
  get_vertices() const noexcept
  { return m_vertices; }

  void
  draw(const area_map &map) const override
  { basic_wall_impl::_draw_walls(map, m_vertices, m_color); }

  void
  update(area_map &map,int n_ticks_passed) override
  { }

  bool
  is_gone() const override
  { return false; }

  vec2d_d
  act_on_object(area_map &map, phys_object *subj) override
  {
    vec2d_d force = {0, 0};
    for (size_t j = 1; j < m_vertices.size(); ++j)
    {
      const size_t i = j - 1;
      const vec2d_d walldir = m_vertices[j] - m_vertices[i];
      const line_segment wall {m_vertices[i], walldir};

      sight s;
      const double r = subj->get_radius();
      if (cast_sight({subj->get_position(), subj->get_radius()}, wall, s))
      {
        const double dphi = interval_size({s.phi1, s.phi2});
        const double sector_area = r*r*dphi/2;
        const double triang_area = r*r*sin(dphi/2)*cos(dphi/2);
        const double overlap_area = sector_area - triang_area;
        vec2d_d n = normalized(vec2d_d(-walldir.y, walldir.x));
        n = n * copysign(1., dot(n, subj->get_position() - wall(s.sight_data.line.t1)));
        force = force + overlap_area * n;

        // special treatment for corners
        double t1 = s.sight_data.line.t1;
        double t2 = s.sight_data.line.t2;
        if (t1 > t2)
          std::swap(t1, t2);
        if (t1 == 0)
        {
          const vec2d_d r = subj->get_position() - wall(t1);
          force = force + normalized(r)*(subj->get_radius() - mag(r))/2;
        }
        if (t2 == 1)
        {
          const vec2d_d r = subj->get_position() - wall(t2);
          force = force + normalized(r)*(subj->get_radius() - mag(r))/2;
        }
      }
    }
    return force;
  }

  bool
  overlap_box(const rectangle &box) const override
  {
    for (size_t j = 1; j < m_vertices.size(); ++j)
    {
      const size_t i = j - 1;
      const vec2d_d walldir = m_vertices[j] - m_vertices[i];
      const line_segment wall {m_vertices[i], walldir};
      if (overlap_box_linesegm(box, wall))
        return true;
    }
    return false;
  }

  pt2d_d
  sample_point() const override
  { return m_vertices.front(); }

  void
  get_sights(vision_processor &visproc) const override
  {
    for (size_t j = 1; j < m_vertices.size(); ++j)
    {
      const size_t i = j - 1;
      const line_segment wall {m_vertices[i], m_vertices[j] - m_vertices[i]};
      sight s;
      if (cast_sight(visproc.get_source(), wall, s))
        visproc.add_sight(s, i);
    }
  }

  void
  draw(const area_map &map, const sight &s) const override
  {
    SDL_Renderer *rend = map.get_sdl().get_renderer();
    const line_segment &wallline = s.static_data.line;
    const double t1 = s.sight_data.line.t1;
    const double t2 = s.sight_data.line.t2;
    const pt2d_i start = map.point_to_pixels(wallline(t1));
    const pt2d_i end = map.point_to_pixels(wallline(t2));
    aalineColor(rend, start.x, start.y, end.x, end.y, m_color);
  }

  protected:
  std::vector<pt2d_d> m_vertices;
  color_t m_color;
}; // class basic_wall


class filled_wall: public basic_wall<solid_ends> {
  public:
  filled_wall(const std::vector<pt2d_d> &vertices)
  : basic_wall(vertices),
    m_edge_color {m_color},
    m_fill_color {m_color}
  { m_vertices.push_back(m_vertices[0]); }

  void
  set_color(color_t color) noexcept override
  { m_edge_color = m_fill_color = color; }

  void
  set_edge_color(color_t color) noexcept
  { m_edge_color = color; }

  void
  set_fill_color(color_t color) noexcept
  { m_fill_color = color; }

  void
  draw(const area_map &map) const override
  {
    SDL_Renderer *rend = map.get_sdl().get_renderer();
    const size_t n = m_vertices.size() - 1;
    int16_t xs[n], ys[n];
    for (size_t i = 0; i < n; ++i)
    {
      pt2d_i pix = map.point_to_pixels(m_vertices[i]);
      xs[i] = pix.x;
      ys[i] = pix.y;
    }
    //filledPolygonColor(rend, xs, ys, n, m_fill_color);
    aapolygonColor(rend, xs, ys, n, m_edge_color);
  }

  private:
  color_t m_edge_color;
  color_t m_fill_color;
}; // class basic_filled_wall


} // namespace mw

#endif
