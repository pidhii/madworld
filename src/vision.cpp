#include "vision.hpp"
#include "object.hpp"
#include "exceptions.hpp"

#include <SDL2/SDL2_gfxPrimitives.h>

#include <boost/format.hpp>


bool
mw::cast_sight(const circle &src, const circle &circ, sight &res) noexcept
{
  const vec2d_d dO = circ.center - src.center;
  const double dO2 = mag2(dO);

  const double R = src.radius + circ.radius;
  if (R*R <= dO2)
    return false;

  double r = sqrt(mag2(circ.center-src.center) - circ.radius*circ.radius);
  double dphi;
  // FIXME
  //if (src.radius < r)
  //{
    //const double r1 = src.radius;
    //const double r2 = circ.radius;
    //const double r12 = r1*r1;
    //const double r22 = r2*r2;
    //// 'rlc' = radical line center
    //const double rlcmag = (dO2 - r22 + r12)/(2*dO2);
    //// 'rlcp' = perpendicular to rlc
    //const double rlcpmag = sqrt(r12 - rlcmag*rlcmag);
    //dphi = fabs(atan2(rlcpmag, r1));
    //r = src.radius;
  //}
  //else
    dphi = fabs(atan2(circ.radius, r));

  const double dirang = dirangle(dO);
  res.phi1 = fix_angle(dirang - dphi);
  res.phi2 = fix_angle(dirang + dphi);
  res.sight_data.circle.cphi1 =
    dirangle(circle(src.center, r)(res.phi1) - circ.center);
  res.sight_data.circle.cphi2 =
    dirangle(circle(src.center, r)(res.phi2) - circ.center);

  if (interval_size({res.phi1, res.phi2}) > M_PI)
  {
    std::swap(res.phi1, res.phi2);
    std::swap(res.sight_data.circle.cphi1, res.sight_data.circle.cphi2);
  }

  res.static_data = std::make_shared<sight::static_data_type>(circ);
  res.tag = sight::circle;
  return true;
}

bool
mw::cast_sight(const circle &src, const line_segment &line, sight &res) noexcept
{
  const double R = src.radius;
  const double R2 = R*R;
  const vec2d_d dO = line.origin - src.center;
  const vec2d_d d = line.direction;
  const double dOd = dot(dO, d);
  const double dOd2 = dOd*dOd;
  const double d2 = mag2(d);
  const double dO2 = mag2(dO);

  const double D = dOd2 - d2*(dO2 - R2);
  if (D < 0)
    return false;

  const double root = sqrt(D);
  double t1 = (-dOd - root)/d2;
  double t2 = (-dOd + root)/d2;

  if (t1 > t2)
    std::swap(t1, t2);

  /* t1 < t2 */
  const vec2d_d dir1 = line.origin - src.center;
  const vec2d_d dir2 = (line.origin + line.direction) - src.center;
  if (t1 < 0)
  {
    if (t2 < 0)
      // too far
      return false;
    else if (1 < t2)
    { // both ends visible
      res.sight_data.line.t1 = 0;
      res.phi1 = dirangle(dir1);
      // --
      res.sight_data.line.t2 = 1;
      res.phi2 = dirangle(dir2);
      goto l_return_true;
    }
    else /* 0 < t2 < 1 */
    { // line origin is visible, the other end is too far
      res.sight_data.line.t1 = 0;
      res.phi1 = dirangle(dir1);
      // --
      const vec2d_d dir2 = (line.origin + t2*line.direction) - src.center;
      res.sight_data.line.t2 = t2;
      res.phi2 = dirangle(dir2);
      // --
      goto l_return_true;
    }
  }
  else if (t1 < 1)
  {
    if (t2 < 1)
    { // line crosses the circle but both ends are invisible
      const vec2d_d dir1 = (line.origin + t1*line.direction) - src.center;
      res.sight_data.line.t1 = t1;
      res.phi1 = dirangle(dir1);
      // --
      const vec2d_d dir2 = (line.origin + t2*line.direction) - src.center;
      res.sight_data.line.t2 = t2;
      res.phi2 = dirangle(dir2);
      // --
      goto l_return_true;
    }
    else /* 1 < t2 */
    { // line origin is too far, but the other end is visible
      const vec2d_d dir1 = (line.origin + t1*line.direction) - src.center;
      res.sight_data.line.t1 = t1;
      res.phi1 = dirangle(dir1);
      // --
      res.sight_data.line.t2 = 1;
      res.phi2 = dirangle(dir2);
      // --
      goto l_return_true;
    }
  }
  else /* 1 < t1,2 */
    // too far
    return false;

l_return_true:
  // segment described by [phi1, phi2] must be counter-clockwise
  if (std::max(res.phi1, res.phi2) - std::min(res.phi1, res.phi2) > M_PI)
  { // segment intersects Pi (phi(t) = Pi, for some t in [0, 1])
    if (res.phi1 < res.phi2)
    {
      std::swap(res.phi1, res.phi2);
      std::swap(res.sight_data.line.t1, res.sight_data.line.t2);
    }
  }
  else if (res.phi1 > res.phi2)
  {
    std::swap(res.phi1, res.phi2);
    std::swap(res.sight_data.line.t1, res.sight_data.line.t2);
  }
  res.tag = sight::line;
  res.static_data = std::make_shared<sight::static_data_type>(line);
  return true;
}

/* XXX: this function assumes that sights overlap */
static inline bool
_is_circle_before_circle(const mw::pt2d_d &source, const mw::sight& a,
    const mw::sight &b) noexcept
{
  const double r12 = mag2(a.static_data->circle.center - source);
  const double r22 = mag2(b.static_data->circle.center - source);
  return r12 < r22;
}

/* XXX: this function assumes that sights overlap */
static bool
_is_circle_before_line(const mw::pt2d_d &source, const mw::sight& a,
    const mw::sight &b) noexcept
{
  using namespace mw;

  const line_segment &line = b.static_data->line;
  const pt2d_d p1 = line.origin + b.sight_data.line.t1*line.direction;
  const pt2d_d p2 = line.origin + b.sight_data.line.t2*line.direction;
  const double p1r2 = mag2(p1 - source);
  const double p2r2 = mag2(p2 - source);
  const pt2d_d r1 = a.static_data->circle(a.sight_data.circle.cphi1);
  const pt2d_d r2 = a.static_data->circle(a.sight_data.circle.cphi2);
  const double r12 = mag2(r1 - source);
  const double r22 = mag2(r2 - source);
  if (contains_inc({a.phi1, a.phi2}, b.phi1))
  {
    const line_segment ray {
      source,
      b.static_data->line(b.sight_data.line.t1) - source
    };
    double _;
    return intersect(ray, a.static_data->circle, _) > 0;
  }
  else if (contains_inc({a.phi1, a.phi2}, b.phi2))
  {
    const line_segment ray {
      source,
      b.static_data->line(b.sight_data.line.t2) - source
    };
    double _;
    return intersect(ray, a.static_data->circle, _) > 0;
  }
  else
  {
    const line_segment ray {source, a.static_data->circle.center - source};
    double _;
    return not (intersect(ray, b.static_data->line, _, _) > 0);
  }
}

/* XXX: this function assumes that sights overlap */
static inline bool
_is_line_before_circle(const mw::pt2d_d &source, const mw::sight& a,
    const mw::sight &b) noexcept
{ return not _is_circle_before_line(source, b, a); }

/* XXX: this function assumes that sights overlap */
static bool
_is_line_before_line(const mw::pt2d_d &source, const mw::sight& a,
    const mw::sight &b) noexcept
{
  using namespace mw;

  if (contains({a.phi1, a.phi2}, b.phi1))
  { // draw a line from source to the first point on `b` and check if it
    // intersects `a`.
    const line_segment ray {
      source,
      b.static_data->line(b.sight_data.line.t1)-source
    };
    double _;
    return intersect(ray, a.static_data->line, _, _) > 0;
  }
  else if (contains({a.phi1, a.phi2}, b.phi2))
  { // draw a line from source to the second point on `b` and check if it
    // intersects `a`.
    const line_segment ray {
      source,
      b.static_data->line(b.sight_data.line.t2)-source
    };
    double _;
    return intersect(ray, a.static_data->line, _, _) > 0;
  }
  else if (a.phi1 == b.phi1 and a.phi2 == b.phi2)
  {
    const double tbcenter = (b.sight_data.line.t1 + b.sight_data.line.t2)/2;
    const pt2d_d bcenter = b.static_data->line(tbcenter);
    const line_segment ray {source, bcenter - source};
    double t1, t2;
    const int ret = intersect(ray, a.static_data->line, t1, t2);
    return ret > 0;
  }
  else
    return not _is_line_before_line(source, b, a);
}

mw::substraction_result
mw::substract(double &a1, double &a2, const std::pair<double, double> &b)
{
  const double _a1 = a1;
  const double _a2 = a2;

  bool contained = false;
  if (contains_inc(b, a1))
  {
    if (not contains_inc({a1, a2}, b.second))
      return empty;
    a1 = b.second;
    contained = true;
  }
  if (contains_inc(b, a2))
  {
    if (not contains_inc({a1, a2}, b.first))
      return empty;
    a2 = b.first;
    contained = true;
  }

  if (contained)
    return adjust;
  else // not contained
  {
    a1 = b.first;
    a2 = b.second;
    return split;
  }
}

void
mw::study_box(const pt2d_d &src, const rectangle &box, box_info &res)
{
  using namespace mw;

  res.is_source_inside = box.contains(src);

  // determine angular position of box sectors
  res.ul = box.offset + vec2d_d {0, box.height};
  res.ur = box.offset + vec2d_d {box.width, box.height};
  res.dr = box.offset + vec2d_d {box.width, 0};
  res.dl = box.offset + vec2d_d {0, 0};
  res.ulang = dirangle(res.ul - src);
  res.urang = dirangle(res.ur - src);
  res.drang = dirangle(res.dr - src);
  res.dlang = dirangle(res.dl - src);
  res.sectors[0] = {res.drang, res.urang};
  res.sectors[1] = {res.urang, res.ulang};
  res.sectors[2] = {res.ulang, res.dlang};
  res.sectors[3] = {res.dlang, res.drang};
  // fix angular ranges so that they are counter-clockwise
  for (size_t i = 0; i < 4; ++i)
  {
    const double phi1 = res.sectors[i].first;
    const double phi2 = res.sectors[i].second;
    if (interval_size({phi1, phi2}) > M_PI)
      std::swap(res.sectors[i].first, res.sectors[i].second);
  }

  // figure out angular interval of the whole box
  if (res.is_source_inside)
  {
    res.phi1 = res.phi2 = 0;
    res.dphi = 2*M_PI;
  }
  else
  {
    double phi1 = 0;
    double phi2 = 0;
    double dphi = -1;
    const double corners[4] = {res.ulang, res.urang, res.drang, res.dlang};
    for (size_t i = 1; i < 4; ++i)
    {
      for (size_t j = i+1; j < 4; ++j)
      {
        double newphi1 = corners[i];
        double newphi2 = corners[j];
        if (interval_size({newphi1, newphi2}) >= M_PI)
          std::swap(newphi1, newphi2);

        const double newdphi = interval_size({newphi1, newphi2});
        assert(newdphi > 0 and newdphi <= M_PI);
        if (newdphi > dphi)
        {
          dphi = newdphi;
          phi1 = newphi1;
          phi2 = newphi2;
        }
      }
    }
    res.phi1 = phi1;
    res.phi2 = phi2;
    res.dphi = dphi;
  }
}

void
mw::cast_shadows_in_the_box(const pt2d_d &src, const line_segment &line, double t1,
    double t2, double phi1, double phi2, const rectangle &box,
    const box_info &boxinfo, std::vector<pt2d_d> &out)
{
  const pt2d_d a = line(t1);
  const pt2d_d b = line(t2);

  int asec = -1;
  int bsec = -1;
  for (int i = 0; i < 4; ++i)
  {
    if (contains_inc(boxinfo.sectors[i], dirangle(a - src))) asec = i;
    if (contains_inc(boxinfo.sectors[i], dirangle(b - src))) bsec = i;
  }
  if (asec < 0 or bsec < 0)
  {
    throw exception {
      "cast_shadows_in_the_box() -- looks like source is not within the box"};
  }

  line_segment aedge, bedge;
  box_edge(box, asec, aedge);
  box_edge(box, bsec, bedge);

  out.push_back(a);

  const line_segment aray {src, a-src};
  double t, _;
  intersect(aray, aedge, t, _);
  out.push_back(aray(t));

  for (int sec = asec; sec != bsec; sec = (sec + 1) % 4)
  {
    switch (sec)
    {
      case 0: out.push_back(box.offset + vec2d_d(box.width, box.height)); break;
      case 1: out.push_back(box.offset + vec2d_d(0, box.height)); break;
      case 2: out.push_back(box.offset + vec2d_d(0, 0)); break;
      case 3: out.push_back(box.offset + vec2d_d(box.width, 0)); break;
    }
  }

  const line_segment bray {src, b-src};
  intersect(bray, bedge, t, _);
  out.push_back(bray(t));

  out.push_back(b);
}

void
mw::cast_shadows_on_the_box(const pt2d_d &src, const line_segment &line_,
    double t1, double t2, double phi1, double phi2, const rectangle &box,
    const box_info &boxinfo, std::vector<pt2d_d> &out)
{
  const pt2d_d a = line_(t1);
  const pt2d_d b = line_(t2);
  const line_segment line {a, b - a};

  const line_segment aray {src, a - src};
  const line_segment bray {src, b - src};

  double xsum = 0, ysum = 0;
  for (int i = 0; i < 4; ++i)
  {
    line_segment edge;
    box_edge(box, i, edge);

    double tray, tedge;
    if (intersect(aray, edge, tray, tedge) and
        tray > 1 and
        0 < tedge and tedge < 1)
    {
      const pt2d_d p = edge(tedge);
      out.push_back(p);
      xsum += p.x;
      ysum += p.y;
    }
    if (intersect(bray, edge, tray, tedge) and
        tray > 1 and
        0 < tedge and tedge < 1)
    {
      const pt2d_d p = edge(tedge);
      out.push_back(p);
      xsum += p.x;
      ysum += p.y;
    }
    if (intersect(line, edge, tray, tedge) > 0)
    {
      const pt2d_d p = edge(tedge);
      out.push_back(p);
      xsum += p.x;
      ysum += p.y;
    }
  }

  if (box.contains_inc(a))
  {
    out.push_back(a);
    xsum += a.x;
    ysum += a.y;
  }
  if (box.contains_inc(b))
  {
    out.push_back(b);
    xsum += b.x;
    ysum += b.y;
  }

  for (const pt2d_d &p : {boxinfo.ul, boxinfo.ur, boxinfo.dr, boxinfo.dl})
  {
    const line_segment ray {src, p - src};
    double t, _;
    if (intersect(ray, line, t, _) > 0)
    {
      out.push_back(p);
      xsum += p.x;
      ysum += p.y;
    }
  }

  const pt2d_d center = {xsum/out.size(), ysum/out.size()};
  std::sort(out.begin(), out.end(), [&] (const pt2d_d &a, const pt2d_d &b) {
    return dirangle(a - center) < dirangle(b - center);
  });
}

bool
mw::is_before(const pt2d_d &source, const sight &a, const sight &b)
{
  switch (a.tag)
  {
    case sight::circle:
      switch (b.tag)
      {
        case sight::circle: return _is_circle_before_circle(source, a, b);
        case sight::line: return _is_circle_before_line(source, a, b);
      }
      throw std::logic_error {(boost::format("undefined sight b:%d") % b.tag).str()};
    // end case circle
    case sight::line:
      switch (b.tag)
      {
        case sight::circle: return _is_line_before_circle(source, a, b);
        case sight::line: return _is_line_before_line(source, a, b);
      }
      throw std::logic_error {(boost::format("undefined sight b:%d") % b.tag).str()};
    // end case line
  }
  throw std::logic_error {(boost::format("undefined sight a:%d") % a.tag).str()};
}

static void
_adjust_line_sight(const mw::pt2d_d &source, mw::sight &s, double phi1,
    double phi2)
{
  using namespace mw;

  if (phi1 != s.phi1)
  {
    const line_segment ray {source, rotated(vec2d_d(1, 0), phi1)};
    double rayt, linet;
    // we know there MUST be intersection
    intersect(ray, s.static_data->line, rayt, linet);
    s.phi1 = phi1;
    s.sight_data.line.t1 = linet;
  }

  if (phi2 != s.phi2)
  {
    const line_segment ray {source, rotated(vec2d_d(1, 0), phi2)};
    double rayt, linet;
    // we know there MUST be intersection
    intersect(ray, s.static_data->line, rayt, linet);
    s.phi2 = phi2;
    s.sight_data.line.t2 = linet;
  }
}

static void
_adjust_circle_sight(const mw::pt2d_d &source, mw::sight &s, double phi1,
    double phi2)
{
  using namespace mw;

  if (phi1 != s.phi1)
  {
    const line_segment ray {source, rotated(vec2d_d(1, 0), phi1)};
    double t;
    // we know there MUST be intersection
    intersect(ray, s.static_data->circle, t);
    s.phi1 = phi1;
    s.sight_data.circle.cphi1 = dirangle(ray(t) - s.static_data->circle.center);
  }

  if (phi2 != s.phi2)
  {
    const line_segment ray {source, rotated(vec2d_d(1, 0), phi2)};
    double t;
    // we know there MUST be intersection
    intersect(ray, s.static_data->circle, t);
    s.phi2 = phi2;
    s.sight_data.circle.cphi2 = dirangle(ray(t) - s.static_data->circle.center);
  }
}

void
mw::adjust_sight(const circle &source, sight &s, double phi1, double phi2)
{
  switch (s.tag)
  {
    case sight::circle:
      _adjust_circle_sight(source.center, s, phi1, phi2);
      break;

    case sight::line:
      _adjust_line_sight(source.center, s, phi1, phi2);
      break;

    default:
      throw std::logic_error {(boost::format("undefined sight s:%d") % s.tag).str()};
  }
}



void
mw::vision_processor::_load_obstacle(const vis_obstacle *obs)
{ obs->get_sights(*this); }

void
mw::vision_processor::process()
{
  const circle &source = get_source();

  auto it1 = m_sights.begin();
  while (it1 != m_sights.end())
  {
    sight &s1 = *it1;

    auto it2 = std::next(it1);
    while (it2 != m_sights.end())
    {
      sight &s2 = *it2;

      // TODO: this branching can be resolved more efficienly
      if (overlaps({s1.phi1, s1.phi2}, {s2.phi1, s2.phi2}))
      {
        if (is_before(source.center, s1, s2))
        {
          if (s1.is_transparent)
            continue;

          double newphi1 = s2.phi1;
          double newphi2 = s2.phi2;
          switch (substract(newphi1, newphi2, {s1.phi1, s1.phi2}))
          {
            case empty:
            {
              auto tmp = it2;
              ++it2;
              m_sights.erase(tmp);
              continue;
            }
            case adjust:
            {
              adjust_sight(source, s2, newphi1, newphi2);
              break;
            }
            case split:
            {
              sight s2copy = s2;
              adjust_sight(source, s2, s2.phi1, newphi1);
              adjust_sight(source, s2copy, newphi2, s2copy.phi2);
              m_sights.emplace_back(s2copy);
            }
          }
        }
        else
        {
          if (s2.is_transparent)
            continue;

          double newphi1 = s1.phi1;
          double newphi2 = s1.phi2;
          switch (substract(newphi1, newphi2, {s2.phi1, s2.phi2}))
          {
            case empty:
            {
              auto tmp = it1;
              ++it1;
              m_sights.erase(tmp);
              goto l_loop1_continue; // dont increment it1 again
            }
            case adjust:
            {
              adjust_sight(source, s1, newphi1, newphi2);
              break;
            }
            case split:
            {
              sight s1copy = s1;
              adjust_sight(source, s1, s1.phi1, newphi1);
              adjust_sight(source, s1copy, newphi2, s1copy.phi2);
              m_sights.emplace_back(s1copy);
            }
          }
        }
      }

      ++it2;
    }

    ++it1;
    l_loop1_continue:;
  }
}

void
mw::vision_processor::apply(const pt2d_d &tgtsrc, std::list<sight> &tgtsights)
  const
{
  const circle &source = get_source();

  // fix target dir-angles
  const line_segment srcline {tgtsrc, source.center - tgtsrc};
  for (sight &s : tgtsights)
  {
    switch (s.tag)
    {
      case sight::circle:
      {
        const circle &circ = s.static_data->circle;
        const pt2d_d a = circ(s.sight_data.circle.cphi1);
        const pt2d_d b = circ(s.sight_data.circle.cphi2);
        s.phi1 = dirangle(a - source.center);
        s.phi2 = dirangle(b - source.center);
        if (interval_size({s.phi1, s.phi2}) > M_PI)
        {
          std::swap(s.phi1, s.phi2);
          std::swap(s.sight_data.circle.cphi1, s.sight_data.circle.cphi2);
        }
        break;
      }
      case sight::line:
      {
        const line_segment &line = s.static_data->line;
        const pt2d_d a = line(s.sight_data.line.t1);
        const pt2d_d b = line(s.sight_data.line.t2);
        s.phi1 = dirangle(a - source.center);
        s.phi2 = dirangle(b - source.center);
        if (interval_size({s.phi1, s.phi2}) > M_PI)
        {
          std::swap(s.phi1, s.phi2);
          std::swap(s.sight_data.line.t1, s.sight_data.line.t2);
        }
        break;
      }
    }
  }

  auto it1 = m_sights.begin();
  while (it1 != m_sights.end())
  {
    const sight &s1 = *it1;

    auto it2 = tgtsights.begin();
    while (it2 != tgtsights.end())
    {
      sight &s2 = *it2;

      // TODO: this branching can be resolved more efficienly
      if (overlaps({s1.phi1, s1.phi2}, {s2.phi1, s2.phi2}))
      {
        if (s1.static_data->obs == s2.static_data->obs and
            s1.static_data->aux_data == s2.static_data->aux_data)
        {
          ++it2;
          continue;
        }

        if (is_before(source.center, s1, s2))
        {
          if (s1.is_transparent)
            continue;

          double newphi1 = s2.phi1;
          double newphi2 = s2.phi2;
          switch (substract(newphi1, newphi2, {s1.phi1, s1.phi2}))
          {
            case empty:
            {
              auto tmp = it2;
              ++it2;
              tgtsights.erase(tmp);
              continue;
            }
            case adjust:
            {
              adjust_sight(source, s2, newphi1, newphi2);
              break;
            }
            case split:
            {
              sight s2copy = s2;
              adjust_sight(source, s2, s2.phi1, newphi1);
              adjust_sight(source, s2copy, newphi2, s2copy.phi2);
              tgtsights.emplace_back(s2copy);
            }
          }
        }
      }

      ++it2;
    }

    ++it1;
    l_loop1_continue:;
  }
}


void
mw::vision_processor::shadowcast(SDL_Renderer *rend, const rectangle &box,
    SDL_BlendMode blendmode, color_t color, const mapping &world_to_target)
  const
{
  const circle &source = get_source();

  box_info boxinfo;
  study_box(source.center, box, boxinfo);

  SDL_BlendMode oldblendmode;
  SDL_GetRenderDrawBlendMode(rend, &oldblendmode);
  SDL_SetRenderDrawBlendMode(rend, blendmode);
  std::vector<pt2d_d> pts;
  for (const sight &s : m_sights)
  {
    //if (not boxinfo.is_source_inside and
        //not overlaps_inc({boxinfo.phi1, boxinfo.phi2}, {s.phi1, s.phi2}))
      //continue;

    line_segment line;
    double t1, t2;
    switch (s.tag)
    {
      case sight::line:
        line = s.static_data->line;
        t1 = s.sight_data.line.t1;
        t2 = s.sight_data.line.t2;
        break;

      case sight::circle:
        {
          const circle &circ = s.static_data->circle;
          const pt2d_d a = circ(s.sight_data.circle.cphi1);
          const pt2d_d b = circ(s.sight_data.circle.cphi2);
          line = {a, b - a};
          t1 = 0;
          t2 = 1;
          break;
        }
    }

    pts.clear();
    cast_shadows_on_the_box(source.center, line, t1, t2, s.phi1, s.phi2, box,
        boxinfo, pts);
    if (pts.empty())
      continue;

    std::vector<int16_t> xs, ys;
    for (const pt2d_d &pt : pts)
    {
      const pt2d_d texpt = world_to_target(pt);
      xs.push_back(texpt.x);
      ys.push_back(texpt.y);
    }
    filledPolygonColor(rend, xs.data(), ys.data(), xs.size(), color);
  }
  SDL_SetRenderDrawBlendMode(rend, oldblendmode);
}

