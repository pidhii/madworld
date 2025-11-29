#include "geometry.hpp"
#include "logging.h"
#include "vision.hpp"

#include <vector>
#include <algorithm>
#include <limits>

#include <SDL2/SDL.h>

#define AIRBAG 0.00001
#define AIRBAG_SCALE 0.01

template <>
const mw::linear_mapping<mw::mapping_order::mul_add>
    mw::linear_mapping<mw::mapping_order::mul_add>::identity {{0, 0}, 1, 1};

template <>
const mw::linear_mapping<mw::mapping_order::add_mul>
    mw::linear_mapping<mw::mapping_order::add_mul>::identity {{0, 0}, 1, 1};


template <>
mw::linear_mapping<mw::mapping_order::mul_add>
mw::linear_mapping<mw::mapping_order::mul_add>::from_sdl_viewport(SDL_Renderer *rend)
{
  SDL_Rect viewport;
  float scale_x, scale_y;
  SDL_RenderGetScale(rend, &scale_x, &scale_y);
  SDL_RenderGetViewport(rend, &viewport);

  const mw::vec2d_d offset {viewport.x*scale_x, viewport.y*scale_y};
  return {offset, scale_x, scale_y};
}

template <>
mw::linear_mapping<mw::mapping_order::add_mul>
mw::linear_mapping<mw::mapping_order::add_mul>::from_sdl_viewport(SDL_Renderer *rend)
{
  SDL_Rect viewport;
  float scale_x, scale_y;
  SDL_RenderGetScale(rend, &scale_x, &scale_y);
  SDL_RenderGetViewport(rend, &viewport);

  const mw::vec2d_i offset {viewport.x, viewport.y};
  return {mw::vec2d_d(offset), scale_x, scale_y};
}



#define EQN_EPSILON 1e-10

mw::quadratic_solver::quadratic_solver(double a, double b, double c, bool debug)
{
  const double
    D = b*b - 4*a*c;

  x1 = (-b + sqrt(D))/(2*a);
  x2 = (-b - sqrt(D))/(2*a);

  if (debug)
  {
    info("+-------[ quadratic_solver ]--------");
    info("| x1 = %f  --> %f", x1, a*x1*x1 + b*x1 + c);
    info("| x2 = %f  --> %f", x2, a*x2*x2 + b*x2 + c);
    info("+");
  }
}

mw::cubic_solver::cubic_solver(double _a, double _b, double _c, double _d,
    bool debug)
{
  if (fabs(_a) < EQN_EPSILON)
  {
    const quadratic_solver eqn {_b, _c, _d, debug};
    std::tie(x1, x2) = eqn.solutions();
    x3 = std::numeric_limits<double>::quiet_NaN();
    return;
  }

  const double
    a0 = _d/_a,
    a1 = _c/_a,
    a2 = _b/_a;

  const double
    q = a1/3 - a2*a2/9,
    r = (a1*a2 - 3*a0)/6 - a2*a2*a2/27;

  const double r2q3 = r*r + q*q*q;
  if (r2q3 > 0)
  { // Only one real solution.
    const double
      A = cbrt(fabs(r) + sqrt(r2q3));
    const double
      t1 = r >= 0 ? A - q/A
                  : q/A - A;
    xmax = x1 = t1 - a2/3;
    x2 = x3 = std::numeric_limits<double>::quiet_NaN();
  }
  else
  { // Three real solutions.
    const double
      theta = q < 0 ? acos(r/pow(sqrt(-q), 3)) : 0;

    const double
      phi1 = theta/3;
    const double
      phi2 = phi1 - 2*M_PI/3,
      phi3 = phi1 + 2*M_PI/3;

    const double
      sqrtmq = sqrt(-q);

    xmax = x1 = 2*sqrtmq*cos(phi1) - a2/3;
    x2 = 2*sqrtmq*cos(phi2) - a2/3;
    x3 = 2*sqrtmq*cos(phi3) - a2/3;
  }

  if (debug)
  {
    info("+-------[ cubic_solver ]--------");
    info("| x1 = %f  --> %f", x1, _a*x1*x1*x1 + _b*x1*x1 + _c*x1 + _d);
    info("| x2 = %f  --> %f", x2, _a*x2*x2*x2 + _b*x2*x2 + _c*x2 + _d);
    info("| x3 = %f  --> %f", x3, _a*x3*x3*x3 + _b*x3*x3 + _c*x3 + _d);
    info("+");
  }
}

mw::quartic_solver::quartic_solver(double _a, double _b, double _c, double _d,
    double _e, bool debug)
{
  if (fabs(_a) < EQN_EPSILON)
  {
    const cubic_solver eqn {_b, _c, _d, _e};
    std::tie(x1, x2, x3) = eqn.solutions();
    x4 = std::numeric_limits<double>::quiet_NaN();
    return;
  }

  const double
    a0 = _e/_a,
    a1 = _d/_a,
    a2 = _c/_a,
    a3 = _b/_a;

  const cubic_solver eqn {1, -a2, a1*a3 - 4*a0, 4*a0*a2 - a1*a1 - a0*a3*a3, debug};
  const double
    u1 = eqn.max_solution();

  const double
    Eg = a1 - a3*u1/2 > 0 ? 1 : -1;

  const double
    tmp1 = sqrt(a3*a3/4 + u1 - a2),
    tmp2 = sqrt(u1*u1/4 - a0);
  const double
    p1 = a3/2 - tmp1,
    p2 = a3/2 + tmp1,
    q1 = u1/2 + Eg*tmp2,
    q2 = u1/2 - Eg*tmp2;

  const double
    tmp3 = sqrt(p1*p1/4 - q1),
    tmp4 = sqrt(p2*p2/4 - q2);

  x1 = -p1/2 + tmp3;
  x2 = -p1/2 - tmp3;
  x3 = -p2/2 + tmp4;
  x4 = -p2/2 - tmp4;

  if (debug)
  {
    info("+-------[ quartic_solver ]--------");
    info("| x1 = %f  --> %f", x1, _a*x1*x1*x1*x1 + _b*x1*x1*x1 + _c*x1*x1 + _d*x1 + _e);
    info("| x2 = %f  --> %f", x2, _a*x2*x2*x2*x2 + _b*x2*x2*x2 + _c*x2*x2 + _d*x2 + _e);
    info("| x3 = %f  --> %f", x3, _a*x3*x3*x3*x3 + _b*x3*x3*x3 + _c*x3*x3 + _d*x3 + _e);
    info("| x4 = %f  --> %f", x4, _a*x3*x3*x3*x3 + _b*x3*x3*x3 + _c*x3*x3 + _d*x3 + _e);
    info("+");
  }
}

int
mw::intersect_tunnel_tunnel(const tunnel &a, const tunnel &b, double &t)
{
  const vec2d_d dO = a.origin - b.origin;
  const vec2d_d dT = a.translation - b.translation;
  const double R = a.radius + b.radius;
  const double R2 = R*R;
  const double dO2 = mag2(dO);
  const double dT2 = mag2(dT);
  const double dOdT = dot(dO, dT);

  // handle overlapping
  if (dO2 <= R2)
  {
    const double magdO = sqrt(dO2);
    const double airbag =  magdO > AIRBAG ? AIRBAG : magdO*AIRBAG_SCALE;

    const double ra = a.radius;
    const double rb = b.radius;
    const double newra = ra*(magdO - airbag)/R;
    const double newrb = newra*(rb/ra);

    const tunnel newa {a.origin, a.translation, newra};
    const tunnel newb {b.origin, b.translation, newrb};

    static int itcnt = 0;
    if (itcnt > 3)
    {
      warning("[intersect(tunnel, tunnel)] exceeded limit of iterations");
      return 0;
    }
    itcnt += 1;
    const int ret = intersect_tunnel_tunnel(newa, newb, t);
    itcnt = 0;

    if (ret > 0)
    {
      t = 0;
      return 1;
    }
    else
      return ret;


  }

  const double D = dOdT*dOdT - (dO2 - R2)*dT2;
  if (D < 0)
    return 0;

  const double root = sqrt(D);
  const double t1 = (-dOdT + root)/dT2;
  const double t2 = (-dOdT - root)/dT2;

  t = std::min(t1, t2);
  return (0 <= t and t <= 1) ? 1 : -1;
}

bool
mw::intersect_atunnel_atunnel(const atunnel &t1, const atunnel &t2, double &t)
{
  const pt2d_d
    O1 = t1.origin,
    O2 = t2.origin;
  const vec2d_d
    v1 = t1.velocity,
    v2 = t2.velocity;
  const vec2d_d
    a1 = t1.acceleration,
    a2 = t2.acceleration;
  const double
    r1 = t1.radius,
    r2 = t2.radius;

  const vec2d_d
    dO = O1 - O2,
    dv = v1 - v2,
    da = a1 - a2;
  const double
    R = r1 + r2;

  // circles already overlap: return immediately.
  //if (mag2(dO) <= R*R)
  //{
    //t = 0;
    //return true;
  //}

  const double
    a = mag2(da)/4,
    b = dot(dv, da),
    c = mag2(dv) + dot(dO, da),
    d = 2*dot(dO, dv),
    e = mag2(dO) - R*R;

  double x1, x2, x3, x4;
  const quartic_solver eqn {a, b, c, d, e};
  std::tie(x1, x2, x3, x4) = eqn.solutions();

  const cubic dr {4*a, 3*b, 2*c, d};
  if (std::isnan(x1) or x1 < 0 or dr(x1) >= 0) x1 = DBL_MAX;
  if (std::isnan(x2) or x2 < 0 or dr(x2) >= 0) x2 = DBL_MAX;
  if (std::isnan(x3) or x3 < 0 or dr(x3) >= 0) x3 = DBL_MAX;
  if (std::isnan(x4) or x4 < 0 or dr(x4) >= 0) x4 = DBL_MAX;

  t = std::min({x1, x2, x3, x4});
  return t != DBL_MAX;
}

int
mw::intersect(const line_segment &l1, const line_segment &l2, double &t1,
    double &t2)
{
  const pt2d_d &O1 = l1.origin;
  const pt2d_d &O2 = l2.origin;
  const vec2d_d &T1 = l1.direction;
  const vec2d_d &T2 = l2.direction;

  if (fabs(T1.y*T2.x - T1.x*T2.y) < DBL_EPSILON)
    // lines are prallel
    return 0;

  double myt1, myt2;
  if (T2.y == 0)
  {
    myt1 = (1./T1.y)*(O2.y - O1.y + (T2.y/T2.x)*(O1.x - O2.x)) /
      (1. - (T2.y*T1.x)/(T2.x*T1.y));

    myt2 = (O1.x - O2.x + myt1*T1.x)/T2.x;
  }
  else if (T1.y == 0)
  {
    myt2 = (1./T2.y)*(O1.y - O2.y + (T1.y/T1.x)*(O2.x - O1.x)) /
      (1. - (T1.y*T2.x)/(T1.x*T2.y));

    myt1 = (O2.x - O1.x + myt2*T2.x)/T1.x;
  }
  else if (T2.x == 0)
  {
    myt1 = (1./T1.x)*(O2.x - O1.x + (T2.x/T2.y)*(O1.y - O2.y)) /
      (1. - (T2.x*T1.y)/(T2.y*T1.x));

    myt2 = (O1.y - O2.y + myt1*T1.y)/T2.y;
  }
  else // if (T1.x == 0)
  {
    myt2 = (1./T2.x)*(O1.x - O2.x + (T1.x/T1.y)*(O2.y - O1.y)) /
      (1. - (T1.x*T2.y)/(T1.y*T2.x));

    myt1 = (O2.y - O1.y + myt2*T2.y)/T1.y;
  }
  //else
    //return 0;

  t1 = myt1;
  t2 = myt2;
  return (0 <= myt1 and myt1 <= 1 and 0 <= myt2 and myt2 <= 1) ? 1 : -1;
}

int
mw::intersect(const tunnel &tun, const pt2d_d &pt, double &t)
{
  const vec2d_d dO = tun.origin - pt;
  const vec2d_d T = tun.translation;
  const double dO2 = mag2(dO);
  const double T2 = mag2(T);
  const double dOT = dot(dO, T);
  const double r2 = tun.radius*tun.radius;

  if (T2 == 0)
    return 0;

  const double D = dOT*dOT - T2*(dO2 - r2);
  if (D < 0)
    return 0;

  const double root = sqrt(D);
  const double t1 = (-dOT + root)/T2;
  const double t2 = (-dOT - root)/T2;

  t = std::min(t1, t2);
  return (0 <= t and t <= 1) ? 1 : -1;
}

static bool
_intersect_atunnel_line(const mw::atunnel &atun, const mw::line_segment &ls,
    double &t, double &line_alpha)
{
  using namespace mw;

  const pt2d_d
    o = atun.origin,
    p = ls.origin;
  const vec2d_d
    op = o - p,
    v = atun.velocity,
    a = atun.acceleration,
    l = ls.direction;
  const double
    r = atun.radius;

  const double
    r2 = r*r,
    al = dot(a, l),
    vl = dot(v, l),
    opl = dot(op, l),
    l2 = mag2(l),
    op2 = mag2(op),
    v2 = mag2(v),
    a2 = mag2(a),
    opv = dot(op, v),
    opa = dot(op, a),
    va = dot(v, a);

  const double
    aa = 0.5*al/l2,
    ba = vl/l2,
    ca = opl/l2;

  double t1, t2, t3, t4;
  const quartic_solver eqn {
    0.25*a2 + l2*aa*aa - al*aa,
    l2*2*aa*ba + va - 2*vl*aa - al*ba,
    v2 + l2*(ba*ba + 2*aa*ca) + opa - 2*opl*aa - 2*vl*ba - al*ca,
    l2*2*ba*ca + 2*opv - 2*opl*ba - 2*vl*ca,
    op2 + l2*ca*ca - 2*opl*ca - r2,
  };
  std::tie(t1, t2, t3, t4) = eqn.solutions();

  const auto alpha = [=](double t) -> double { return aa*t*t + ba*t + ca; };
  const auto is_on_line = [=](double a) -> bool { return 0 < a and a < 1; };

  if (std::isnan(t1) or t1 < 0 or not is_on_line(alpha(t1))) t1 = DBL_MAX;
  if (std::isnan(t2) or t2 < 0 or not is_on_line(alpha(t2))) t2 = DBL_MAX;
  if (std::isnan(t3) or t3 < 0 or not is_on_line(alpha(t3))) t3 = DBL_MAX;
  if (std::isnan(t4) or t4 < 0 or not is_on_line(alpha(t4))) t4 = DBL_MAX;

  t = std::min({t1, t2, t3, t4});
  line_alpha = alpha(t);
  return t != DBL_MAX;
}

static bool
_intersect_atunnel_point(const mw::atunnel &atun, const mw::pt2d_d &p,
    double &t)
{
  using namespace mw;

  const vec2d_d
    op = atun.origin - p,
    v = atun.velocity,
    a = atun.acceleration;

  const double
    op2 = mag2(op),
    v2 = mag2(v),
    a2 = mag2(a),
    opv = dot(op, v),
    opa = dot(op, a),
    va = dot(v, a),
    r2 = atun.radius*atun.radius;

  double t1, t2, t3, t4;
  const quartic_solver eqn {a2/4, va, v2 + opa, 2*opv, op2 - r2};
  std::tie(t1, t2, t3, t4) = eqn.solutions();

  if (std::isnan(t1) or t1 < 0) t1 = DBL_MAX;
  if (std::isnan(t2) or t2 < 0) t2 = DBL_MAX;
  if (std::isnan(t3) or t3 < 0) t3 = DBL_MAX;
  if (std::isnan(t4) or t4 < 0) t4 = DBL_MAX;

  t = std::min({t1, t2, t3, t4});
  return t != DBL_MAX;
}

bool
mw::intersect_atunnel_linesegm(const atunnel &atun, const line_segment &l,
    double &t, double &alpha)
{
  std::vector<std::pair<double, double>> buf;

  if (_intersect_atunnel_point(atun, l.origin, t))
    buf.emplace_back(t, 0);
  if (_intersect_atunnel_point(atun, l.origin + l.direction, t))
    buf.emplace_back(t, 1);
  if (_intersect_atunnel_line(atun, l, t, alpha))
    buf.emplace_back(t, alpha);

  if (buf.empty())
    return false;

  const auto it = std::min_element(buf.begin(), buf.end(), [=](auto &a, auto &b) {
      return a.first < b.first;
  });
  t = it->first;
  alpha = it->second;
  return true;
}

bool
mw::intersect(const tunnel &p, const line_segment &l, double &t,
    solid_ends)
{
  if (p.radius == 0)
  {
    double _;
    line_segment pline {p.origin, p.translation};
    return intersect(pline, l, t, _) > 0;
  }

  const vec2d_d dO = p.origin - l.origin, T = p.translation, d = l.direction;
  const double r2 = p.radius*p.radius;
  const double dO2 = mag2(dO), T2 = mag2(T), d2 = mag2(d);
  const double dOT = dot(dO, T), Td = dot(T, d), dOd = dot(dO, d);

  const double a = T2 - Td*Td/d2;
  const double b = dOT - dOd*Td/d2;
  const double c = dO2 - dOd*dOd/d2 - r2;

  const double D = fma(b, b, -a*c);
  if (D < 0)
  {
    return intersect(p, l.origin + l.direction, t) > 0
        or intersect(p, l.origin, t) > 0;
  }
  const double root = sqrt(D);
  double t1 = (-b + root)/a;
  double t2 = (-b - root)/a;
  if (t1 > t2)
    std::swap(t1, t2);

  const double alpha1 = fma(Td, t1, dOd)/d2;
  const double alpha2 = fma(Td, t2, dOd)/d2;
  if (0 <= t1 and t1 <= 1)
  {
    if (0 <= alpha1 and alpha1 <= 1)
    {
      t = t1;
      return true;
    }
    else if (alpha1 > 1)
      return intersect(p, l.origin + l.direction, t) > 0;
    else
      return intersect(p, l.origin, t) > 0;
  }

l_check_end_points:
  return intersect(p, l.origin + l.direction, t) > 0
      or intersect(p, l.origin, t) > 0;
}

bool
mw::intersect(const tunnel &p, const line_segment &l, double &t,
    better_ends)
{
  if (p.radius == 0)
  {
    double _;
    line_segment pline {p.origin, p.translation};
    return intersect(pline, l, t, _) > 0;
  }

  const vec2d_d dO = p.origin - l.origin, T = p.translation, d = l.direction;
  const double r2 = p.radius*p.radius;
  const double dO2 = mag2(dO), T2 = mag2(T), d2 = mag2(d);
  const double dOT = dot(dO, T), Td = dot(T, d), dOd = dot(dO, d);

  const double a = T2 - Td*Td/d2;
  const double b = dOT - dOd*Td/d2;
  const double c = dO2 - dOd*dOd/d2 - r2;

  const double D = fma(b, b, -a*c);
  if (D < 0)
    return false;
  const double root = sqrt(D);
  double t1 = (-b + root)/a;
  double t2 = (-b - root)/a;
  if (t1 > t2)
    std::swap(t1, t2);

  const double alpha1 = fma(Td, t1, dOd)/d2;
  const double alpha2 = fma(Td, t2, dOd)/d2;
  if (0 <= t1 and t1 <= 1)
  {
    if (0 <= alpha1 and alpha1 <= 1)
    {
      t = t1;
      return true;
    }
    else if (alpha1 > 1)
      return intersect(p, l.origin + l.direction, t) > 0;
    else
      return intersect(p, l.origin, t) > 0;
  }
  return false;
}

bool
mw::intersect(const tunnel &p, const line_segment &l, double &t,
    ignore_ends)
{
  if (p.radius == 0)
  {
    double _;
    line_segment pline {p.origin, p.translation};
    return intersect(pline, l, t, _) > 0;
  }

  const vec2d_d dO = p.origin - l.origin, T = p.translation, d = l.direction;
  const double r2 = p.radius*p.radius;
  const double dO2 = mag2(dO), T2 = mag2(T), d2 = mag2(d);
  const double dOT = dot(dO, T), Td = dot(T, d), dOd = dot(dO, d);

  const double a = T2 - Td*Td/d2;
  const double b = dOT - dOd*Td/d2;
  const double c = dO2 - dOd*dOd/d2 - r2;

  const double D = fma(b, b, -a*c);
  if (D < 0)
    return false;
  const double root = sqrt(D);
  double t1 = (-b + root)/a;
  double t2 = (-b - root)/a;
  if (t1 > t2)
    std::swap(t1, t2);

  const double alpha1 = fma(Td, t1, dOd)/d2;
  const double alpha2 = fma(Td, t2, dOd)/d2;
  if (0 <= t1 and t1 <= 1)
  {
    if (0 <= alpha1 and alpha1 <= 1)
    {
      t = t1;
      return true;
    }
    else
      return false;
  }
  return false;
}

int
mw::intersect(const line_segment &l, const circle &c, double &t)
{
  const vec2d_d dir1 = l.origin - c.center;
  const vec2d_d dir2 = (l.origin + l.direction) - c.center;

  const double R = c.radius;
  const double R2 = R*R;
  const vec2d_d dO = l.origin - c.center;
  const vec2d_d d = l.direction;
  const double dOd = dot(dO, d);
  const double dOd2 = dOd*dOd;
  const double d2 = mag2(d);
  const double dO2 = mag2(dO);

  const double D = dOd2 - d2*(dO2 - R2);
  if (D < 0)
    return 0;

  const double root = sqrt(D);
  double t1 = (-dOd - root)/d2;
  double t2 = (-dOd + root)/d2;
  if (t1 > t2)
    std::swap(t1, t2);

  t = t1;

  if (t1 < 0)
  {
    if (t2 < 0)
      return -1;
    else if (t2 < 1)
    {
      t = t2;
      return 1;
    }
    else
      return -1;
  }
  else if (t1 < 1)
  {
    t = t1;
    return 1;
  }
  else
    return -1;
}

static double
_clamp(double x, double min, double max)
{
  if (x < min)
    return min;
  else if (x > max)
    return max;
  else
    return x;
}

bool
mw::overlap_box_circle(const rectangle &r, const circle &c)
{
  const double closestX = _clamp(c.center.x, r.offset.x, r.offset.x + r.width);
  const double closestY = _clamp(c.center.y, r.offset.y, r.offset.y + r.height);

  const double distanceX = c.center.x - closestX;
  const double distanceY = c.center.y - closestY;

  const double d2 = distanceX*distanceX + distanceY*distanceY;
  return d2 < c.radius*c.radius;
}

bool
mw::overlap_box_linesegm(const rectangle &r, const line_segment &l)
{
  if (r.contains_inc(l(0)) or r.contains_inc(l(1)))
    return true;

  line_segment side;
  for (int sector = 0; sector < 4; ++sector)
  {
    box_edge(r, sector, side);
    double t1, t2;
    if (intersect(side, l, t1, t2) > 0)
      return true;
  }
  return false;
}
