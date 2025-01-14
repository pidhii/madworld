#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include "object.hpp"

namespace mw {

class area_map;


void
inelastic_collision_1d(double CR, double m1, double &v1, double m2, double &v2);

void
inelastic_collision(double CR, const vec2d_d &_n, double m1, vec2d_d &v1,
    double m2, vec2d_d &v2);

void
pushing(double m1, const pt2d_d &o1, vec2d_d &a1,
        double m2, const pt2d_d &o2, vec2d_d &a2);


class physics_processor {
  public:
  virtual void add_object(phys_object *obj) = 0;
  virtual void add_obstacle(phys_obstacle *obs) = 0;
  virtual void process(area_map &map) = 0;

  protected:
  void
  set_position(phys_object *obj, const pt2d_d &p) const noexcept
  { obj->set_position(p); }

  void
  set_velocity(phys_object *obj, const vec2d_d &v) const noexcept
  { obj->set_velocity(v); }
};


// TODO: rename
class md_physics: public physics_processor {
  public:
  md_physics(double tick_size = 1)
  : m_tick_size {tick_size}
  { }

  void
  add_object(phys_object *obj) noexcept override
  { m_objects.push_back(obj); }

  void
  add_obstacle(phys_obstacle *obs) noexcept override
  { m_obstacles.push_back(obs); }

  void
  process(area_map &map) override;

  private:
  typedef std::pair<mw::phys_object*, mw::vec2d_d> obj_dynamics;

  void
  _calc_dynamics(area_map &map, std::vector<obj_dynamics> &dyn) const;

  void
  _advance_objects(area_map &map, double dt,
      const std::vector<obj_dynamics> &dyn);

  private:
  const double m_tick_size;
  std::vector<phys_object*> m_objects;
  std::vector<phys_obstacle*> m_obstacles;
}; // class mw::md_physics


} // namespace mw

#endif
