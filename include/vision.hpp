/**
 * @file vision.hpp
 * @brief Vision processing algorithms
 * @author Ivan Pidhurskyi <ivanpidhurskyi1997@gmail.com>
 */
#ifndef VISION_HPP
#define VISION_HPP

#include "geometry.hpp"
#include "common.hpp"
#include "exceptions.hpp"

#include <SDL2/SDL.h>

#include <memory>
#include <list>
#include <vector>


namespace mw {

class vis_obstacle;

/** @defgroup Vision Vision
 * @brief Vision processing algorithms
 * @{
 */

/**
 * @brief Cast angle to the standardised range of [-`M_PI`, `M_PI`].
 * @details All functions in this module accept angles only within this range.
 * @see contains(), overlaps(), is_before()
 */
inline double
fix_angle(double phi) noexcept
{
  while (phi < -M_PI) phi = 2*M_PI + phi;
  while (+M_PI < phi) phi = phi - 2*M_PI;
  return phi;
}

/**
 * @brief Storage for sight information on some shape.
 */
struct sight {
  enum tag_type { circle, line };

  struct static_data_type {
    static_data_type(const mw::circle &c): circle {c} { }
    static_data_type(const line_segment &l): line {l} { }
    union {
      mw::circle circle;
      line_segment line;
    };
    qword aux_data;
    const vis_obstacle *obs; // TODO: change this to a void-pointer
  };


  tag_type tag;

  union sight_data_type {
    struct circle_sight_data {
      /**
       * @{
       * @brief Directional angles defining visible segment of the circle.
       * @details Indices of these angles correspond to the indices of the
       * "sight-angles" @ref sight.phi1 and @ref sight.phi2. <br>
       * I.e. two beams
       * - <from observer center @ phi1(2)>, and
       * - <from circle center @ cphi1(2)>
       * .
       * intersect at the first(second) point of the visible segment.
       */
      double cphi1, cphi2;
      /** @} */
    } circle;

    struct line_sight_data {
      /**
       * @{
       * @brief "Time points" defining an interval of the line covered by the
       * sight.
       * @details Index of a time point corresponds to an index of a directional
       * angle.
       */
      double t1, t2;
      /** @} */
    } line;
  } sight_data;

  std::shared_ptr<static_data_type> static_data;

  /**
   * @{
   * @brief Directional angles defining segment of the sight obscured by the
   * line segment.
   * @details Obscured segment of sight is then obtained as a counter-clockwise
   * rotation from @ref sight.phi1 to @ref sight.phi2.
   */
  double phi1, phi2;
  bool is_transparent = false;
};

/**
 * @brief Compute sight on a line circle.
 * @param src Source of sight: observer's position and vision radius.
 * @param circ Circle obscuring the sight.
 * @param[out] res If circle @p circ is within the sight, @p res is filled with
 * the sight data. Angles @p res.phi1 and @p res.phi2 are guaranteed to lie with
 * the range of [-`M_PI`, `M_PI`].
 * @return Whether the @p line is within the sight.
 * @note Contents of @p res are undefined if return value is `false`.
 */
bool
cast_sight(const circle &src, const circle &circ, sight &res) noexcept;

/**
 * @brief Compute sight on a line segment.
 * @param src Source of sight: observer's position and vision radius.
 * @param line Line segment obscuring the sight.
 * @param[out] res If line segment @p line is within the sight, @p res is filled
 * with the sight data. Angles @p res.phi1 and @p res.phi2 are guaranteed to lie
 * with the range of [-`M_PI`, `M_PI`].
 * @return Whether the line segment @p line is within the sight.
 * @note Contents of @p res are undefined if return value is `false`.
 */
bool
cast_sight(const circle &src, const line_segment &line, sight &res) noexcept;

/**
 * @brief Check if an angle is contained in a given open interval.
 * @param a A pair of angles defining the interval via counter-clockwise
 * rotation from @p a.first to @p a.second.
 * @param b An angle to be checked for belonging to the interval.
 * @note All angles must be given in radians and lie within the range of
 * [-`M_PI`, `M_PI`].
 * @see fix_angle()
 */
inline bool
contains(const std::pair<double, double> &a, double b) noexcept
{
  if (a.second < a.first)
    return b < a.second or a.first < b;
  else
    return a.first < b and b < a.second;
}

inline bool
contains_inc(const std::pair<double, double> &a, double b) noexcept
{
  if (a.second < a.first)
    return b <= a.second or a.first <= b;
  else
    return a.first <= b and b <= a.second;
}


inline bool
contains(const std::pair<double, double> &a, const std::pair<double, double> &b)
  noexcept
{
  if (a.first < a.second)
  {
    if (b.first < b.second)
      return a.first < b.first  and b.first  < a.second
         and a.first < b.second and b.second < a.second;
    else
      return false;
  }
  else // a.second < a.first
  {
    if (b.first < b.second)
      return (b.first < a.second and b.second < a.second)
          or (a.first < b.first  and a.first  < b.second);
    else
      return b.second < a.second and a.first < b.first;
  }
}

inline bool
contains_inc(const std::pair<double, double> &a,
    const std::pair<double, double> &b)
  noexcept
{
  if (a.first < a.second)
  {
    if (b.first < b.second)
      return a.first <= b.first  and b.first  <= a.second
         and a.first <= b.second and b.second <= a.second;
    else
      return false;
  }
  else // a.second < a.first
  {
    if (b.first < b.second)
      return (b.first <= a.second and b.second <= a.second)
          or (a.first <= b.first  and a.first  <= b.second);
    else
      return b.second <= a.second and a.first <= b.first;
  }
}

/**
 * @brief Check if two open angular intervals overlap.
 * @details Intervals are defined
 * @note All angles must be given in radians and lie within the range of
 * [-`M_PI`, `M_PI`].
 * @see fix_angle()
 */
inline bool
overlaps(const std::pair<double, double> &a, const std::pair<double, double> &b)
  noexcept
{
  return contains(a, b.first)
      or contains(a, b.second)
      or contains(b, a.first)
      or contains(b, a.second)
      or a == b;
}

inline bool
overlaps_inc(const std::pair<double, double> &a, const std::pair<double, double> &b)
  noexcept
{
  return contains_inc(a, b.first)
      or contains_inc(a, b.second)
      or contains_inc(b, a.first)
      or contains_inc(b, a.second)
      or a == b;
}


inline int
orientation(const std::pair<double, double> &a)
{ return copysign(1, a.second - a.first); }

inline int
orientation(double from, double to)
{ return copysign(1, to - from); }

inline int
direction(double from, double to)
{ return copysign(1, to - from); }

enum substraction_result { empty, adjust, split };
substraction_result
substract(double &a1, double &a2, const std::pair<double, double> &b);

/**
 * @note Result is undefined in case when @p ab.first == @p ab.second.
 */
inline double
interval_size(const std::pair<double, double> &ab)
{
  const double a = ab.first < 0 ? ab.first+2*M_PI : ab.first;
  const double b = ab.second < 0 ? ab.second+2*M_PI : ab.second;
  const double dphi = b - a;
  return dphi < 0 ? dphi+2*M_PI : dphi;
}

/*   pi-pi/4
 *   +----------+ pi/4
 *   |\        /|
 *   | \   1  / |
 *   |  \    /  |
 *   |   \  /   |
 *   | 2  \/  0 |
 *   |    /\    |
 *   |   /  \   |
 *   |  /    \  |
 *   | /   3  \ |
 *   |/        \|
 *   +----------+ -pi/4
 *   -pi+pi/4
 */
inline int
square_sector(double phi)
{
  if (-M_PI_4 < phi and phi <= M_PI_4)
    return 0;
  else if (M_PI_4 < phi and phi <= M_PI-M_PI_4)
    return 1;
  else if (phi <= -M_PI+M_PI_4 or M_PI-M_PI_4 < phi)
    return 2;
  else
    return 3;
}

inline void
box_edge(const rectangle &box, int sector, line_segment &out)
{
  pt2d_d a, b;
  switch (sector)
  {
    case 0:
      a = box.offset + vec2d_d {box.width, 0};
      b = box.offset + vec2d_d {box.width, box.height};
      break;
    case 1:
      a = box.offset + vec2d_d {0, box.height};
      b = box.offset + vec2d_d {box.width, box.height};
      break;
    case 2:
      a = box.offset + vec2d_d {0, 0};
      b = box.offset + vec2d_d {0, box.height};
      break;
    case 3:
      a = box.offset + vec2d_d {0, 0};
      b = box.offset + vec2d_d {box.width, 0};
      break;
  }
  out = line_segment {a, b - a};
}

struct box_info {
  pt2d_d ul, ur, dr, dl;
  double ulang, urang, drang, dlang;
  std::pair<double, double> sectors[4];
  double phi1, phi2, dphi;
  bool is_source_inside;
};

void
study_box(const pt2d_d &src, const rectangle &box, box_info &res);

/* XXX this is broken */
void
cast_shadows_in_the_box(const pt2d_d &src, const line_segment &line, double t1,
    double t2, double phi1, double phi2, const rectangle &box,
    const box_info &boxinfo, std::vector<pt2d_d> &out);

void
cast_shadows_on_the_box(const pt2d_d &src, const line_segment &line, double t1,
    double t2, double phi1, double phi2, const rectangle &box,
    const box_info &boxinfo, std::vector<pt2d_d> &out);

/**
 * @brief Check if one obstacle is located "before" the other with respect to
 * the observer.
 * @note
 * - This function assumes that sights overlap, i.e.
 *   @code{cpp}
 *   overlaps({a.phi1, a.phi2}, {b.phi1, b.phi2}) == true
 *   @endcode
 * - All angles stored in @p a and @p b must be given in radians and lie
 *   within the range of [-`M_PI`, `M_PI`].
 * @see overlaps(), fix_angle()
 */
bool
is_before(const pt2d_d &source, const sight &a, const sight &b);

void
adjust_sight(const circle &source, sight &s, double new_phi1, double new_phi2);


class vision_processor {
  public:
  static constexpr char class_name[] = "mw::vision_processor";
  using exception = scoped_exception<class_name>;

  using iterator = std::list<sight>::iterator;
  using const_iterator = std::list<sight>::const_iterator;

  vision_processor()
  : m_curobs {nullptr},
    m_ignore {nullptr}
  { }

  vision_processor(const circle &source)
  : m_source {source},
    m_curobs {nullptr},
    m_ignore {nullptr}
  { }

  vision_processor(vision_processor &&other)
  : m_source {other.m_source},
    m_curobs {nullptr},
    m_ignore {other.m_ignore}
  { std::swap(m_sights, other.m_sights); }

  vision_processor&
  operator = (vision_processor &&other) noexcept
  {
    m_source = other.m_source;
    std::swap(m_sights, other.m_sights);
    m_ignore = other.m_ignore;
    return *this;
  }

  void
  set_source(const circle &source) noexcept
  { m_source = source; }

  const circle&
  get_source() const
  {
    if (not m_source.has_value())
      throw exception {"source was not initialized"}.in(__func__);
    return m_source.value();
  }

  void
  set_ignore(const vis_obstacle *obs) noexcept
  { m_ignore = obs; }

  class const_sights_view {
    public:
    const_sights_view(const vision_processor &visproc): m_visproc {visproc} { }
    const_iterator begin() const noexcept { return m_visproc.m_sights.begin(); }
    const_iterator end() const noexcept { return m_visproc.m_sights.end(); }
    private:
    const vision_processor &m_visproc;
  };

  class sights_view {
    public:
    sights_view(vision_processor &visproc): m_visproc {visproc} { }
    iterator begin() const noexcept { return m_visproc.m_sights.begin(); }
    iterator end() const noexcept { return m_visproc.m_sights.end(); }
    private:
    vision_processor &m_visproc;
  };

  const_sights_view
  get_sights() const noexcept
  { return const_sights_view {*this}; }

  sights_view
  get_sights() noexcept
  { return sights_view {*this}; }

  template <typename Compare>
  void
  sort_sights(Compare cmp)
  { m_sights.sort(cmp); }

  template <typename Iterator>
  void
  load_obstacles(Iterator begin, Iterator end)
  {
    try
    {
      for (Iterator it = begin; it != end; ++it)
      {
        const vis_obstacle *obs = (*it);
        if (obs == m_ignore)
          continue;

        m_curobs = obs;
        _load_obstacle(obs);
      }
    }
    catch (const std::exception &exn)
    {
      m_curobs = nullptr;
      throw exn;
    }
    m_curobs = nullptr;
  }

  template <typename Container>
  void
  load_obstacles(const Container &container)
  { load_obstacles(container.begin(), container.end()); }

  void
  load_obstacle(const vis_obstacle *obs)
  { load_obstacles(&obs, &obs+1); }

  //template <typename Iterator>
  //void
  //load_sights(Iterator begin, Iterator end)
  //{ m_sights.insert(m_sights.end(), begin, end); }

  //template <typename Container>
  //void
  //load_sights(Container container)
  //{ m_sights.insert(m_sights.end(), container. begin(), container.end()); }

  void
  process();

  void
  apply(const pt2d_d &tgtsrc, std::list<sight> &tgtsights) const;

  void
  apply(vision_processor &other) const
  {
    other.m_source = m_source;
    apply(other.get_source().center, other.m_sights);
  }

  void
  reset() noexcept
  {
    m_source = std::nullopt;
    m_sights.clear();
    m_ignore = nullptr;
  }

  void
  add_sight(const sight &s, qword aux_data) noexcept
  {
    s.static_data->aux_data = aux_data;
    if (m_curobs == nullptr)
    {
      error("m_currobs == (null)");
      abort();
    }
    s.static_data->obs = m_curobs;
    m_sights.emplace_back(s);
  }

  /** @todo TODO: Dont do anything when the texture is completely covered
   * by shadows.
   */
  void
  shadowcast(SDL_Renderer *rend, const rectangle &box, SDL_BlendMode blendmode,
      color_t color, const mapping &world_to_target = mapping::identity) const;

  private:
  void
  _load_obstacle(const vis_obstacle *obs);

  std::optional<circle> m_source;
  std::list<sight> m_sights;
  const vis_obstacle *m_curobs;
  const vis_obstacle *m_ignore;
};

/** @} */
} // namespace mw

#endif
