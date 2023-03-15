/**
 * @file area_map.hpp
 * @brief Container for the game
 * @author Ivan Pidhurskyi <ivanpidhurskyi1997@gmail.com>
 */
#ifndef AREA_MAP_H
#define AREA_MAP_H

#include "common.hpp"
#include "geometry.hpp"
#include "object.hpp"
#include "exceptions.hpp"
#include "textures.hpp"
#include "video_manager.hpp"
#include "utl/grid.hpp"
#include "gui/sdl_string.hpp"
#include "gui/components.hpp"

#include <SDL2/SDL.h>

#include <float.h>
#include <math.h>
#include <tuple>
#include <list>
#include <optional>
#include <boost/optional.hpp>


namespace mw {

class area_map;
class physics_processor;

/** @defgroup Core Core
 * @brief Bind everything together to let the game emerge
 * @{
 */

/** @private */
typedef std::list<phys_object*>::const_iterator phys_object_iterator;
/** @private */
typedef std::list<phys_obstacle*>::const_iterator phys_obstacle_iterator;
/** @private */
typedef std::list<vis_obstacle*>::const_iterator vis_obstacle_iterator;

/** @private */
struct object_entry {
  object_entry(object *_objptr): objptr {_objptr}, flags {0} { }

  object *objptr;
  std::optional<phys_object_iterator> pobjit;
  std::optional<phys_obstacle_iterator> pobsit;
  std::optional<vis_obstacle_iterator> vobsit;
  uint8_t flags;
};
/** @private */
typedef std::list<object_entry>::iterator object_iterator;

using object_id = identifier<object_iterator, area_map>;

enum blit_flags {
  none = 0,
  apply_global_vision = 1 << 0,
  draw_sights = 1 << 1,
};

class area_map {
  private:
  enum oflag {
    is_static = 1 << 0,
  };

  public:
  static constexpr char class_name[] = "mw::area_map";
  using exception = scoped_exception<class_name>;

  area_map(sdl_environment &sdl, texture_storage &texstorage);
  area_map(area_map &&other) = default;

  ~area_map();

  void
  init_background(int pixw, int pixh);

  void
  build_walls();

  void
  load(const std::string &path);

  void
  build_grid();

  const grid<bool>&
  get_grid() const;

  const sdl_environment&
  get_sdl() const noexcept
  { return m_sdl; }

  /** @name Map size
   * @{ */
  double
  get_width() const noexcept
  { return m_width; }

  double
  get_height() const noexcept
  { return m_height; }
  /** @} */

  /** @name Viewport
   * @{ */
  /** @brief Get offset of the map from (0, 0) in IGU. */
  vec2d_d
  get_offset() const noexcept
  { return {m_x_offs, m_y_offs}; }

  /** @brief Set offset of the map from (0, 0) in IGU. */
  void
  set_offset(const vec2d_d &offs) noexcept
  { m_x_offs = offs.x; m_y_offs = offs.y; }

  /** @brief Get scale of a viewport w.r.t. a physical screen.
   *
   * This scale defined conversion ratio of an in game unit length to a length
   * of a physical screen (in pixels).
   *
   * I.e. a line of length 1 [IGU] will be span SCALE pixels on a screen when
   * displayed.
   */
  double
  get_scale() const noexcept
  { return m_scale; }

  /**
   * @brief Get a viewport as a \ref mw::mapping from in game coordinates
   * (a.k.a. "world") to pixel coordinates.
   */
  mapping
  get_view() const noexcept
  { return {{m_x_offs, m_y_offs}, m_scale, m_scale}; }

  /**
   * @brief Set a viewport via a \ref mw::mapping from world to pixel
   * coordinates.
   */
  void
  set_view(const mapping &world_to_pixels) noexcept
  {
    m_x_offs = world_to_pixels.get_offset().x;
    m_y_offs = world_to_pixels.get_offset().y;
    m_scale = world_to_pixels.get_x_scale();
  }

  /** @brief Map a point in world to pixel position on a screen. */
  pt2d_i
  point_to_pixels(const pt2d_d &p) const noexcept
  {
    return {
      int(int(m_scale*p.x) + m_x_offs),
      int(int(m_scale*p.y) + m_y_offs)
    };
  }

  /** @brief Map a pixel position to a point in world. */
  pt2d_d
  pixels_to_point(const pt2d_i &pix) const noexcept
  {
    return {
      (double(pix.x) - m_x_offs)/m_scale,
      (double(pix.y) - m_y_offs)/m_scale
    };
  }

  /** @brief Zoom/unzoom the viewport w.r.t. a given point. */
  void
  zoom(const pt2d_i &p, double newscale) noexcept;
  /** @} */


  /** @name Adding objects
   * @{ */
  object_id
  add_object(object *obj) noexcept;

  void
  register_phys_object(object_id it);

  void
  register_phys_obstacle(object_id it);

  void
  register_vis_obstacle(object_id it);
  /** @} */

  const std::list<phys_obstacle*>&
  get_phys_obstacles() const noexcept
  { return m_phys_obstacles; }

  const std::list<vis_obstacle*>&
  get_vis_obstacles() const noexcept
  { return m_vis_obstacles; }

  void
  adjust_offset(const pt2d_d &p, const pt2d_i &pix) noexcept;

  void
  adjust_to_box_w(const pt2d_i &at, int w) noexcept;

  void
  adjust_to_box_h(const pt2d_i &at, int h) noexcept;

  void
  tick(physics_processor &physproc, int msec);

  /** @name Render map contents
   * @{ */
  void
  draw() const noexcept;

  void
  draw_all() const;

  void
  draw_visible(const vision_processor &local_vision,
    const vision_processor &global_vision) const;

  void
  draw_obstacles() const;

  void
  draw_messages() const;
  /** @} */

  void
  add_message(const std::string &msg);

  void
  blit_glow_with_shadowcast(SDL_Texture *tex, const rectangle &dstbox,
      uint8_t alpha, int flags, vision_processor &visproc) const;

  void
  blit_glow_with_shadowcast(SDL_Texture *tex, const rectangle &dstbox,
      uint8_t alpha, int flags) const;

  void
  add_terrain_overlay(SDL_Texture *tex, const rectangle &dstbox, uint8_t alpha)
    const;

  bool
  has_global_vision() const noexcept
  { return m_global_vision.has_value(); }

  const vision_processor&
  get_global_vision() const
  { return m_global_vision.value(); }

  /** @name Textures
   * @{ */
  texture_storage&
  get_texture_storage() noexcept
  { return m_texstorage; }

  const texture_storage&
  get_texture_storage() const noexcept
  { return m_texstorage; }
  /** @} */

  eth::value
  dump() const;

  private:
  sdl_environment &m_sdl;
  SDL_Texture *m_bgtex;
  mapping m_world_to_bgtex;

  double m_scale;
  double m_x_offs, m_y_offs;
  double m_width, m_height;
  bool m_has_walls;

  texture_storage &m_texstorage;

  std::list<object_entry> m_objects;
  std::list<phys_object*> m_phys_objects;
  std::list<phys_obstacle*> m_phys_obstacles;
  std::list<vis_obstacle*> m_vis_obstacles;

  boost::optional<grid<bool>> m_static_grid;

  mutable boost::optional<const vision_processor&> m_global_vision;

  message_log m_msglog;
}; // class mw::area_map

/** @} group Core */

} // namespace mw

#endif
