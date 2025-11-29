/**
 * @file geometry.hpp
 * @brief Geometry primitives and utilities
 * @author Ivan Pidhurskyi <ivanpidhurskyi1997@gmail.com>
 */
#ifndef GEOMENTRY_H
#define GEOMENTRY_H

#include <cmath>
#include <cfloat>
#include <ostream>
#include <tuple>

#include <SDL2/SDL_rect.h>

struct SDL_Renderer;


namespace mw {
inline namespace geo {
/** @defgroup Geometry Geometry
 * @brief Geometry primitives and utilities
 * @{
 */

template <typename T>
struct pt2d {
  pt2d(T _x, T _y): x {_x}, y {_y} { }
  pt2d() = default;
  template <typename U>
  explicit operator pt2d<U> () const { return {U(x), U(y)}; }
  template <typename U, typename V>
  operator std::tuple<U, V> () noexcept { return {x, y}; }
  template <typename U, typename V>
  operator std::tuple<U, V> () const noexcept { return {x, y}; }
  T x, y;
};

template <typename T>
struct vec2d {
  vec2d(T _x, T _y): x {_x}, y {_y} { }
  vec2d() = default;
  template <typename U>
  explicit operator vec2d<U> () const { return {U(x), U(y)}; }
  vec2d operator - () const noexcept { return {-x, -y}; }
  T x, y;
}; // struct mw::geo::vec2d<T>

template <typename T>
[[nodiscard]] vec2d<T>
to_vec(const pt2d<T> &p) noexcept
{ return {p.x, p.y}; }

template <typename T>
[[nodiscard]] pt2d<T>
to_pt(const vec2d<T> &v) noexcept
{ return {v.x, v.y}; }

template <typename T>
struct mat2d {
  constexpr
  mat2d(const T &_xx, const T &_xy,
        const T &_yx, const T &_yy)
  : xx {_xx}, xy {_xy},
    yx {_yx}, yy {_yy}
  { }

  [[nodiscard]] vec2d<T>
  operator () (const vec2d<T> &v) const noexcept
  { return {xx*v.x + xy*v.y, yx*v.x + yy*v.y}; }

  T xx, xy, yx, yy;
}; // struct mw::mat2d<T>

typedef pt2d<double> pt2d_d;
typedef vec2d<double> vec2d_d;
typedef mat2d<double> mat2d_d;

typedef pt2d<int> pt2d_i;
typedef vec2d<int> vec2d_i;
typedef mat2d<int> mat2d_i;


struct rotation { double s, c; };

struct line_segment {
  pt2d_d origin;
  vec2d_d direction;
  [[nodiscard]] pt2d_d operator () (double t) const noexcept;
};

struct circle {
  circle() = default;
  circle(const pt2d_d &c, double r): center {c}, radius {r} { }
  [[nodiscard]] pt2d_d operator () (const rotation &rot) const noexcept;
  [[nodiscard]] pt2d_d operator () (double phi /* dir. angle in radians */) const noexcept;
  pt2d_d center;
  double radius;
}; // struct mw::geo::circle

struct tunnel {
  pt2d_d origin;
  vec2d_d translation;
  double radius;
}; // struct mw::geo::tunnel

struct atunnel {
  pt2d_d origin;
  vec2d_d velocity;
  vec2d_d acceleration;
  double radius;
  [[nodiscard]] pt2d_d operator () (double t) const noexcept;
  void advance(double t) noexcept { origin = (*this)(t); }
}; // struct mw::geo::tunnel


struct rectangle {
  pt2d_d offset;
  double width, height;

  [[nodiscard]] bool
  contains(const pt2d_d &pt) const noexcept
  {
    return
      pt.x > offset.x and pt.y > offset.y and
      pt.x - offset.x < width and
      pt.y - offset.y < height;
  }

  [[nodiscard]] bool
  contains_inc(const pt2d_d &pt) const noexcept
  {
    return
      pt.x >= offset.x and pt.y >= offset.y and
      pt.x - offset.x <= width and
      pt.y - offset.y <= height;
  }

  pt2d_d
  center() const noexcept
  { return pt2d_d(offset.x + width/2, offset.y + height/2); }

  operator SDL_Rect () const noexcept
  {
    return SDL_Rect {
      .x = int(offset.x),
      .y = int(offset.y),
      .w = int(width),
      .h = int(height),
    };
  }
}; // struct mw::geo::rectangle


} // namespace mw::geo
} // namespace mw


template <typename T>
[[nodiscard]] bool
operator == (const mw::pt2d<T> &a, const mw::pt2d<T> &b) noexcept
{ return a.x == b.x and a.y == b.y; }

template <typename T>
[[nodiscard]] bool
operator != (const mw::pt2d<T> &a, const mw::pt2d<T> &b) noexcept
{ return not (a == b); }

template <typename T>
[[nodiscard]] bool
operator == (const mw::vec2d<T> &a, const mw::vec2d<T> &b) noexcept
{ return a.x == b.x and a.y == b.y; }

template <typename T>
[[nodiscard]] bool
operator != (const mw::vec2d<T> &a, const mw::vec2d<T> &b) noexcept
{ return not (a == b); }



template <typename T>
[[nodiscard]] mw::vec2d<T>
operator * (double a, const mw::vec2d<T> &v) noexcept
{ return {a*v.x, a*v.y}; }

template <typename T>
[[nodiscard]] mw::vec2d<T>
operator * (const mw::vec2d<T> &v, double a) noexcept
{ return a*v; }

template <typename T>
[[nodiscard]] mw::vec2d<T>
operator / (const mw::vec2d<T> &v, double a) noexcept
{ return {T(double(v.x)/a), T(double(v.y)/a)}; }

template <typename T>
[[nodiscard]] mw::vec2d<T>
operator - (const mw::pt2d<T> &a, const mw::pt2d<T> &b) noexcept
{ return {a.x-b.x, a.y-b.y}; }

template <typename T>
[[nodiscard]] mw::pt2d<T>
operator + (const mw::pt2d<T> &p, const mw::vec2d<T> &v) noexcept
{ return {p.x+v.x, p.y+v.y}; }

template <typename T>
[[nodiscard]] mw::pt2d<T>
operator - (const mw::pt2d<T> &p, const mw::vec2d<T> &v) noexcept
{ return {p.x-v.x, p.y-v.y}; }

template <typename T>
[[nodiscard]] mw::vec2d<T>
operator + (const mw::vec2d<T> &u, const mw::vec2d<T> &v) noexcept
{ return {u.x+v.x, u.y+v.y}; }

template <typename T>
[[nodiscard]] mw::vec2d<T>
operator - (const mw::vec2d<T> &u, const mw::vec2d<T> &v) noexcept
{ return {u.x-v.x, u.y-v.y}; }

template <typename T>
[[nodiscard]] mw::mat2d<T>
operator * (const mw::mat2d<T> &m, const T &a) noexcept
{ return {m.xx*a, m.xy*a, m.yx*a, m.yy*a}; }

template <typename T>
[[nodiscard]] mw::mat2d<T>
operator * (const T &a, const mw::mat2d<T> &m) noexcept
{ return m*a; }

template <typename T>
[[nodiscard]] mw::mat2d<T>
operator / (const mw::mat2d<T> &m, const T &a) noexcept
{ return {m.xx/a, m.xy/a, m.yx/a, m.yy/a}; }

template <typename T>
[[nodiscard]] mw::mat2d<T>
operator + (const mw::mat2d<T> &a, const mw::mat2d<T> &b) noexcept
{ return {a.xx+b.xx, a.xy+b.xy, a.yx+b.yx, a.yy+b.yy}; }

template <typename T>
[[nodiscard]] mw::mat2d<T>
operator - (const mw::mat2d<T> &a, const mw::mat2d<T> &b) noexcept
{ return {a.xx-b.xx, a.xy-b.xy, a.yx-b.yx, a.yy-b.yy}; }

template <typename T>
[[nodiscard]] mw::mat2d<T>
operator * (const mw::mat2d<T> &a, const mw::mat2d<T> &b) noexcept
{
  return {
    a.xx*b.xx+a.xy*b.yx, a.xx*b.xy+a.xy*b.yy,
    a.yx*b.xx+a.yy*b.yx, a.yx*b.xy+a.yy*b.yy,
  };
}

namespace mw {
inline namespace geo {

[[nodiscard]] inline pt2d_d
line_segment::operator () (double t) const noexcept
{ return origin + t*direction; }

[[nodiscard]] inline pt2d_d
circle::operator () (const rotation &rot) const noexcept
{ return center + radius*vec2d_d(rot.c, rot.s); }

[[nodiscard]] inline pt2d_d
circle::operator () (double phi) const noexcept
{ return this->operator()({sin(phi), cos(phi)}); }

[[nodiscard]] inline pt2d_d
atunnel::operator () (double t) const noexcept
{ return origin + velocity*t + acceleration*t*t/2; }

template <typename T>
[[nodiscard]] double
dot(const mw::vec2d<T> &v, const mw::vec2d<T> &u) noexcept
{ return fma(v.x, u.x, v.y*u.y); }

template <typename T>
[[nodiscard]] T
mag2(const mw::vec2d<T> &v) noexcept
{ return fma(v.x, v.x, v.y*v.y); }

template <typename T>
[[nodiscard]] T
mag(const mw::vec2d<T> &v) noexcept
{ return std::sqrt(mag2(v)); }

template <typename T>
[[nodiscard]] mw::vec2d<T>
normalized(const mw::vec2d<T> &vec) noexcept
{
  const double m = mag(vec);
  return m == 0 ? vec2d<T> {0, 0} : vec/m;
}

/* Return the angle between two vectors in radians in the range [0, pi]. */
template <typename T>
[[nodiscard]] double
angle(const vec2d<T> &v, const vec2d<T> &u) noexcept
{ return acos(dot(v, u)/(mag(u)*mag(v))); }

/* Return the directional angle of the vector in radians in the rante [-pi, pi]. */
template <typename T>
[[nodiscard]] double
dirangle(const vec2d<T> &v) noexcept
{ return atan2(v.y, v.x); }

template <typename T>
[[nodiscard]] vec2d<T>
rotated(const vec2d<T> &v, const rotation &rot) noexcept
{
  return {
    fma(v.x, rot.c, -v.y*rot.s),
    fma(v.x, rot.s,  v.y*rot.c),
  };
}

template <typename T>
mw::vec2d<T>
rotated(const mw::vec2d<T> &v, double phi) noexcept
{ return rotated(v, {sin(phi), cos(phi)}); }

inline mw::vec2d_d
reflected(const mw::vec2d_d &n, const mw::vec2d_d &v)
{ return v - 2*dot(v, n)*n/mag2(n); }

/**
 * @brief Calculate intersection of two tunnels.
 * @param a,b Tunnels.
 * @param[out] t **Smallest** time point at which tunnels intersect. It implies
 * that if there are two distinct time points for intersection (which is usually
 * the case), and one of them is negative then this negative time point will be
 * stored in @p t regardless of the value of the second time point. Value of
 * @p t is unchanged if return value is 0 (see below).
 * @retval  0 Tunnles dont intersect each other.
 * @retval  1 Tunnels intersect within the time interval of [0, 1].
 * @retval -1 Tunnles intersect outside the time interval of [0, 1].
 * @note Return value is deduces from the value of the **smallest** time point,
 * i.e. the one returned to the caller via parameter @p t. This may lead to
 * unexpected behaviour of the function if tunnles intersect at `t=0`.
 */
int
intersect_tunnel_tunnel(const tunnel &a, const tunnel &b, double &t);

/**
 * @brief Calculate intersection of two tunnels.
 * @details Same as @ref intersect(const tunnel&,const tunnel&,double&) but the
 * time point argument is ommited.
 * @see intersect(tunnel, tunnel, double)
 */
inline int
intersect_tunnel_tunnel(const tunnel &a, const tunnel &b)
{ double t; return intersect_tunnel_tunnel(a, b, t); }

/**
 * @brief Calculate intersection of two accelerating tunnels.
 * @param a,b Tunnels.
 * @param[out] t **Smallest** non-negative time point at which tunnels intersect.
 * @return Whether tunnels intersect.
 */
bool
intersect_atunnel_atunnel(const atunnel &a, const atunnel &b, double &t);

bool
intersect_atunnel_linesegm(const atunnel &a, const line_segment &l, double &t,
    double &alpha);

int
intersect(const line_segment &l1, const line_segment &l2, double &t1,
    double &t2);

int
intersect(const tunnel &tun, const pt2d_d &pt, double &t);

struct ignore_ends {};
struct better_ends {};
struct solid_ends {};

inline const char* dump_ends_spec(ignore_ends) noexcept { return "ignore_ends"; }
inline const char* dump_ends_spec(better_ends) noexcept { return "better_ends"; }
inline const char* dump_ends_spec(solid_ends) noexcept { return "solid_ends"; }


bool
intersect(const tunnel &p, const line_segment &l, double &t, solid_ends);

bool
intersect(const tunnel &p, const line_segment &l, double &t, better_ends);

bool
intersect(const tunnel &p, const line_segment &l, double &t, ignore_ends);

int
intersect(const line_segment &l, const circle &c, double &t);

bool
overlap_box_circle(const rectangle &r, const circle &c);

bool
overlap_box_linesegm(const rectangle &r, const line_segment &l);


// As i do often use offsets and scales directly, it is important to
// distinguis between the to variants of possible implementation:
// - mul_add: s * x + o   -- what i have historicaly
// - add_mul: s * (x + o) -- what someone else (e.g. SDL) may prefer
enum class mapping_order { mul_add, add_mul };
template <mapping_order Order = mapping_order::mul_add>
class linear_mapping {
  public:
  static constexpr mapping_order order = Order;

  static const linear_mapping identity;

  linear_mapping(): m_xscale {1}, m_yscale {1}, m_offs {0, 0} { }

  linear_mapping(const vec2d<double> &offs, double xscale, double yscale)
  : m_xscale {xscale},
    m_yscale {yscale},
    m_offs {offs}
  { }

  template <mapping_order OtherOrder>
  linear_mapping(const linear_mapping<OtherOrder> &other);

  /** 
   * @brief Construct a mapping with current SDL viewport offset and scales.
   * @param rend SDL renderer where the viewport and scales will be taken from.
   * @return Mapping from the current viewport to the pixel coordinates.
   */
  [[nodiscard]] static linear_mapping
  from_sdl_viewport(SDL_Renderer *rend);

  [[nodiscard]] pt2d<double>
  operator () (const pt2d<double> &p) const noexcept
  {
    if constexpr (order == mapping_order::mul_add)
      return {m_xscale * p.x + m_offs.x, m_yscale * p.y + m_offs.y};
    else
      return {m_xscale * (p.x + m_offs.x), m_yscale * (p.y + m_offs.y)};
  }

  [[nodiscard]] vec2d<double>
  operator () (const vec2d<double> &p) const noexcept
  { return {m_xscale * p.x, m_yscale * p.y}; }

  [[nodiscard]] rectangle
  operator () (const rectangle &rect) const noexcept
  {
    return rectangle {
      (*this)(rect.offset),
      rect.width*m_xscale,
      rect.height*m_yscale
    };
  }

  [[nodiscard]] double
  get_x_scale() const noexcept
  { return m_xscale; }

  [[nodiscard]] double
  get_y_scale() const noexcept
  { return m_yscale; }

  [[nodiscard]] vec2d<double>
  get_offset() const noexcept
  { return m_offs; }

  [[nodiscard]] linear_mapping
  inverse() const noexcept
  {
    /* * * * * * * * * * * *
     *   mul_add:
     *   y = ax + o
     *   x = (y - o)/a = (1/a)y - o/a
     *   
     *   add_mul:
     *   y = a(x + o)
     *   x = y/a - o = (1/a)(y - ao)
     */
    if constexpr (order == mapping_order::mul_add)
      return {-1*vec2d_d {m_offs.x/m_xscale, m_offs.y/m_yscale}, 1/m_xscale, 1/m_yscale};
    else
      return {-1*vec2d_d {m_offs.x*m_xscale, m_offs.y*m_yscale}, 1/m_xscale, 1/m_yscale};
  }

  private:
  double m_xscale, m_yscale;
  vec2d<double> m_offs;
}; // class linear_mapping
using mapping = linear_mapping<mapping_order::mul_add>;
using mapping_ = linear_mapping<mapping_order::add_mul>;

// ax + o = a(x + o/a)
template <>
template <> inline
mapping_::linear_mapping(const mapping &other)
: m_xscale {other.get_x_scale()},
  m_yscale {other.get_y_scale()},
  m_offs {other.get_offset().x / other.get_x_scale(),
          other.get_offset().y / other.get_y_scale()}
{ }

// a(x + o) = ax + ao
template <>
template <>
inline
mapping::linear_mapping(const mapping_ &other)
: m_xscale {other.get_x_scale()},
  m_yscale {other.get_y_scale()},
  m_offs {other.get_offset().x * other.get_x_scale(),
          other.get_offset().y * other.get_y_scale()}
{ }

inline mapping
compose(const mapping &f, const mapping &g)
{
  return {
    to_vec(f(to_pt(g.get_offset()))),
    g.get_x_scale()*f.get_x_scale(),
    g.get_y_scale()*f.get_y_scale()
  };
}

struct quadratic_solver {
  double x1, x2;

  quadratic_solver(double a, double b, double c, bool debug = false);

  std::tuple<double, double>
  solutions() const noexcept
  { return {x1, x2}; }
};

struct cubic_solver {
  double x1, x2, x3;
  double xmax;

  cubic_solver(double a, double b, double c, double d, bool debug = false);

  std::tuple<double, double, double>
  solutions() const noexcept
  { return {x1, x2, x3}; }

  double
  max_solution() const noexcept
  { return xmax; }
};

struct cubic {
  double a, b, c, d;
  double
  operator () (double t) const noexcept
  { return a*t*t*t + b*t*t + c*t + d; }
};

struct quartic_solver {
  double x1, x2, x3, x4;

  quartic_solver(double a, double b, double c, double d, double e,
      bool debug = false);

  std::tuple<double, double, double, double>
  solutions() const noexcept
  { return {x1, x2, x3, x4}; }
};

struct quartic {
  double a, b, c, d, e;
  double
  operator () (double t) const noexcept
  { return a*t*t*t*t + b*t*t*t + c*t*t + d*t + e; }
};

/** @} */
} // namespace mw::geo
} // namespace mw


[[nodiscard]] inline mw::mapping
operator * (const mw::mapping &f, const mw::mapping &g)
{ return mw::compose(f, g); }


namespace std {
/** @ingroup Geometry */
template <typename T>
void
swap(mw::geo::vec2d<T> &u, mw::geo::vec2d<T> &v)
{
  const mw::geo::vec2d tmp = u;
  u = v;
  v = tmp;
}
} // namespace std

/** @ingroup Geometry */
template <typename T>
std::ostream&
operator << (std::ostream &os, const mw::geo::vec2d<T> &v)
{ return os << "vec(" << v.x << ", " << v.y << ")"; }

/** @ingroup Geometry */
template <typename T>
std::ostream&
operator << (std::ostream &os, const mw::geo::pt2d<T> &p)
{ return os << "pt(" << p.x << ", " << p.y << ")"; }

#endif
