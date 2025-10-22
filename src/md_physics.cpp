#include "physics.hpp"
#include "area_map.hpp"

#include <sys/time.h>


static void
_get_object_movement(const mw::phys_object *obj, mw::atunnel &move)
{
  move.origin = obj->get_position();
  move.velocity = obj->get_velocity();
  move.acceleration = obj->get_internal_acceleration();
  move.radius = obj->get_radius();
}

void
mw::md_physics::process(area_map &map)
{
  timespec start, stop;
  clock_gettime(CLOCK_MONOTONIC, &start);
  std::vector<obj_dynamics> dyn;
  const double dt = m_tick_size/10;
  for (int i = 0; i < 10; ++i)
  {
    dyn.clear();
    _calc_dynamics(map, dyn);
    _advance_objects(map, dt, dyn);
  }
  clock_gettime(CLOCK_MONOTONIC, &stop);

  const double startsec = double(start.tv_sec) + start.tv_nsec*1e-9;
  const double stopsec = double(stop.tv_sec) + stop.tv_nsec*1e-9;
  const double duration = stopsec - startsec;
  if (duration > m_tick_size*1e-3)
    warning("[md_physics] took %g/%g [sec]", duration, m_tick_size*1e-3);
}


void
mw::md_physics::_calc_dynamics(area_map &map, std::vector<obj_dynamics> &dyn)
  const
{
  dyn.reserve(m_objects.size());
  for (phys_object *obj : m_objects)
  {
    if (obj->is_gone())
      continue;

    vec2d_d tot_force = {0, 0};
    tot_force = tot_force + obj->get_internal_acceleration()*obj->get_mass();
    tot_force = tot_force - obj->get_friction_coeff()*obj->get_velocity();

    // 1) vs obstacles
    for (phys_obstacle *obs : m_obstacles)
    {
      if (obs->is_gone())
        continue;

      tot_force = tot_force + obs->act_on_object(map, obj);
    }

    // 2) vs other objects
    for (phys_object *obj2 : m_objects)
    {
      if (obj2 == obj or obj2->is_gone())
        continue;

      tot_force = tot_force + obj2->act_on_object(map, obj);
    }

    dyn.emplace_back(obj, tot_force);
  }
}

void
mw::md_physics::_advance_objects(area_map &map, double dt,
                                 const std::vector<obj_dynamics> &dyn)
{
  if (dt <= 0)
    return;

  for (const auto &[obj, tot_force] : dyn)
  {
    const pt2d_d p0 = obj->get_position();
    const vec2d_d v0 = obj->get_velocity();
    const vec2d_d a = tot_force/obj->get_mass();

    set_position(obj, p0 + v0*dt + a*dt*dt/2);
    set_velocity(obj, v0 + a*dt);
  }
}
