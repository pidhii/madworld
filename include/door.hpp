#ifndef DOOR_HPP
#define DOOR_HPP

#include "object.hpp"
#include "area_map.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>


namespace mw {

class door: public phys_obstacle, public vis_obstacle {
  public:
  door(const pt2d_d a, const pt2d_d b)
  : m_door {a, b - a},
    m_state {1},
    m_statemod {0}
  { }

  void
  open()
  { m_statemod = -0.005; }

  void
  close()
  { m_statemod = +0.005; }

  void
  update(area_map &, int n_ticks_passed) override
  { m_state = std::max(0., std::min(1., m_state + m_statemod*n_ticks_passed)); }

  bool
  is_gone() const override
  { return false; }

  vec2d_d
  act_on_object(area_map &map, phys_object *subj) override
  {
    vec2d_d force = {0, 0};

    const line_segment wall {m_door.origin, m_state * m_door.direction};

    sight s;
    const double r = subj->get_radius();
    if (cast_sight({subj->get_position(), subj->get_radius()}, wall, s))
    {
      const double dphi = interval_size({s.phi1, s.phi2});
      const double sector_area = r*r*dphi/2;
      const double triang_area = r*r*sin(dphi/2)*cos(dphi/2);
      const double overlap_area = sector_area - triang_area;
      vec2d_d n = normalized(vec2d_d(-wall.direction.y, wall.direction.x));
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
    return force;
  }

  bool
  overlap_box(const rectangle &box) const override
  {
    const line_segment wall {m_door.origin, m_state * m_door.direction};
    if (overlap_box_linesegm(box, wall))
      return true;
    return false;
  }

  pt2d_d
  sample_point() const override
  { return m_door(1); }

  void
  draw(const area_map &map) const override
  {
    const pt2d_i a = map.point_to_pixels(m_door(0));
    const pt2d_i b = map.point_to_pixels(m_door(m_state));
    aalineColor(map.get_sdl().get_renderer(), a.x, a.y, b.x, b.y, 0xFFFF00FF);
  }

  void
  get_sights(vision_processor &visproc) const override
  {
    const line_segment visiblepart {m_door.origin, m_state * m_door.direction};
    sight s;
    if (cast_sight(visproc.get_source(), visiblepart, s))
      visproc.add_sight(s, 0);
  }

  void
  draw(const area_map &map, const sight &s) const override
  {
    SDL_Renderer *rend = map.get_sdl().get_renderer();
    const line_segment &visiblepart = s.static_data.line;
    const double t1 = s.sight_data.line.t1;
    const double t2 = s.sight_data.line.t2;
    const pt2d_i start = map.point_to_pixels(visiblepart(t1));
    const pt2d_i end = map.point_to_pixels(visiblepart(t2));
    aalineColor(rend, start.x, start.y, end.x, end.y, 0xFFFF00FF);
  }

  private:
  const line_segment m_door;
  double m_state;
  double m_statemod;
};

} // namespace mw

#endif
