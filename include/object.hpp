/**
 * @file object.hpp
 * @brief Base classes for objects in the game.
 * @author Ivan Pidhurskyi <ivanpidhurskyi1997@gmail.com>
 */
#ifndef OBJECT_HPP
#define OBJECT_HPP

#include "geometry.hpp"
#include "vision.hpp"
#include "hit.hpp"

#include <ether/ether.hpp>


namespace mw {

class area_map;
class vision_processor;
class physics_processor;


struct object {
  virtual ~object() = default;

  virtual void
  draw(const area_map&) const = 0;

  virtual void
  update(area_map&, int n_ticks_passed) = 0;

  virtual void
  receive_hit(area_map &, const hit &hit) { };

  virtual bool
  is_gone() const = 0;

  virtual void
  destroy()
  { delete this; }

  virtual eth::value
  dump() const
  { return eth::nil(); }
}; // struct mw::projectile


class phys_object;

struct phys_obstacle: public virtual object {
  virtual ~phys_obstacle() = default;

  virtual vec2d_d
  act_on_object(area_map &map, phys_object *subj) = 0;

  virtual bool
  overlap_box(const rectangle &box) const = 0;

  virtual pt2d_d
  sample_point() const = 0;
}; // struct mw::phys_obstacle


class phys_object: public phys_obstacle {
  public:
  phys_object(double radius, const pt2d_d &position)
  : m_radius {radius},
    m_mass {80},
    m_friction_coeff {0.8},
    m_position {position},
    m_velocity {0, 0},
    m_internal_acceleration {0, 0}
  { }

  virtual
  ~phys_object() = default;

  double         get_radius() const noexcept { return m_radius; }
  double         get_mass() const noexcept { return m_mass; }
  double         get_friction_coeff() const noexcept { return m_friction_coeff; }
  const pt2d_d&  get_position() const noexcept { return m_position; }
  const vec2d_d& get_velocity() const noexcept { return m_velocity; }
  const vec2d_d& get_internal_acceleration() const noexcept { return m_internal_acceleration; }

  void           set_mass(double m) noexcept { m_mass = m; }
  void           set_friction_coeff(double k) noexcept { m_friction_coeff = k; }

  virtual vec2d_d
  act_on_object(area_map &map, phys_object *subj) override;

  bool
  overlap_box(const rectangle &box) const override
  { return overlap_box_circle(box, {m_position, m_radius}); }

  pt2d_d
  sample_point() const override
  { return m_position; }

  virtual
  void on_collision(area_map &map, phys_obstacle *obs) { }

  protected:
  void set_position(const pt2d_d &p) noexcept { m_position = p; }
  void set_velocity(const vec2d_d &v) noexcept { m_velocity = v; }
  void set_internal_acceleration(const vec2d_d &a) noexcept { m_internal_acceleration = a; }

  private:
  // shape
  const double m_radius;
  // physical properties
  double m_mass;
  double m_friction_coeff;
  // state
  pt2d_d m_position;
  vec2d_d m_velocity;
  vec2d_d m_internal_acceleration;

  friend class physics_processor;
}; // struct mw::phys_object


struct vis_obstacle: public virtual object {
  using object::draw;

  virtual ~vis_obstacle() = default;

  virtual void
  get_sights(vision_processor &visproc) const = 0;

  virtual void
  draw(const area_map&, const sight&) const = 0;
}; // struct mw::vis_obstacle


} // namespace mw

#endif
