#include "vision.hpp"
#include "object.hpp"
#include "exceptions.hpp"
#include "logging.h"

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

  // XXX im pretty damn certain this is wrong
  //if (interval_size({res.phi1, res.phi2}) > M_PI)
  //{
    //std::swap(res.phi1, res.phi2);
    //std::swap(res.sight_data.circle.cphi1, res.sight_data.circle.cphi2);
  //}

  res.tag = sight::circle;
  res.static_data.circle = circ;
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
  res.static_data.line = line;
  return true;
}

/* XXX: this function assumes that sights overlap */
static inline bool
_is_circle_before_circle(const mw::pt2d_d &source, const mw::sight& a,
    const mw::sight &b) noexcept
{
  const double r12 = mag2(a.static_data.circle.center - source);
  const double r22 = mag2(b.static_data.circle.center - source);
  return r12 < r22;
}

/* XXX: this function assumes that sights overlap */
static bool
_is_circle_before_line(const mw::pt2d_d &source, const mw::sight& a,
    const mw::sight &b) noexcept
{
  using namespace mw;

  const line_segment &line = b.static_data.line;
  const pt2d_d p1 = line.origin + b.sight_data.line.t1*line.direction;
  const pt2d_d p2 = line.origin + b.sight_data.line.t2*line.direction;
  const double p1r2 = mag2(p1 - source);
  const double p2r2 = mag2(p2 - source);
  const pt2d_d r1 = a.static_data.circle(a.sight_data.circle.cphi1);
  const pt2d_d r2 = a.static_data.circle(a.sight_data.circle.cphi2);
  const double r12 = mag2(r1 - source);
  const double r22 = mag2(r2 - source);
  if (contains_inc({a.phi1, a.phi2}, b.phi1))
  {
    const line_segment ray {
      source,
      b.static_data.line(b.sight_data.line.t1) - source
    };
    double _;
    return intersect(ray, a.static_data.circle, _) > 0;
  }
  else if (contains_inc({a.phi1, a.phi2}, b.phi2))
  {
    const line_segment ray {
      source,
      b.static_data.line(b.sight_data.line.t2) - source
    };
    double _;
    return intersect(ray, a.static_data.circle, _) > 0;
  }
  else
  {
    const line_segment ray {source, a.static_data.circle.center - source};
    double _;
    return not (intersect(ray, b.static_data.line, _, _) > 0);
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
      b.static_data.line(b.sight_data.line.t1)-source
    };
    double _;
    return intersect(ray, a.static_data.line, _, _) > 0;
  }
  else if (contains({a.phi1, a.phi2}, b.phi2))
  { // draw a line from source to the second point on `b` and check if it
    // intersects `a`.
    const line_segment ray {
      source,
      b.static_data.line(b.sight_data.line.t2)-source
    };
    double _;
    return intersect(ray, a.static_data.line, _, _) > 0;
  }
  else if (a.phi1 == b.phi1 and a.phi2 == b.phi2)
  {
    const double tbcenter = (b.sight_data.line.t1 + b.sight_data.line.t2)/2;
    const pt2d_d bcenter = b.static_data.line(tbcenter);
    const line_segment ray {source, bcenter - source};
    double t1, t2;
    const int ret = intersect(ray, a.static_data.line, t1, t2);
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
    intersect(ray, s.static_data.line, rayt, linet);
    s.phi1 = phi1;
    s.sight_data.line.t1 = linet;
  }

  if (phi2 != s.phi2)
  {
    const line_segment ray {source, rotated(vec2d_d(1, 0), phi2)};
    double rayt, linet;
    // we know there MUST be intersection
    intersect(ray, s.static_data.line, rayt, linet);
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
    intersect(ray, s.static_data.circle, t);
    s.phi1 = phi1;
    s.sight_data.circle.cphi1 = dirangle(ray(t) - s.static_data.circle.center);
  }

  if (phi2 != s.phi2)
  {
    const line_segment ray {source, rotated(vec2d_d(1, 0), phi2)};
    double t;
    // we know there MUST be intersection
    intersect(ray, s.static_data.circle, t);
    s.phi2 = phi2;
    s.sight_data.circle.cphi2 = dirangle(ray(t) - s.static_data.circle.center);
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


template <
  typename AIter,
  typename TIter,
  typename TEraser,
  typename BackInserter>
void
_process(const mw::circle &source,
         AIter abegin, AIter aend,
         TIter tbegin, TIter tend,
         TEraser teraser,
         BackInserter newsights)
{
  using namespace mw;

  for (TIter tit = tbegin; tit != tend; ++tit)
  {
    sight &ts = *tit;

    for (AIter ait = abegin; ait != aend; ++ait)
    {
      const sight &as = *ait;
      if (as.is_transparent) continue;
      if (same_identity(as, ts)) continue;
      if (not overlaps({ts.phi1, ts.phi2}, {as.phi1, as.phi2})) continue;
      if (not is_before(source.center, as, ts)) continue;

      double newphi1 = ts.phi1;
      double newphi2 = ts.phi2;
      switch (substract(newphi1, newphi2, {as.phi1, as.phi2}))
      {
        case empty:
          std::tie(tit, tend) = teraser(tit);
          --tit;
          ait = aend;
          --ait;
          break;

        case adjust:
          adjust_sight(source, ts, newphi1, newphi2);
          break;

        case split: {
          sight s1copy = ts;
          adjust_sight(source, ts, ts.phi1, newphi1);
          adjust_sight(source, s1copy, newphi2, s1copy.phi2);
          *newsights = s1copy;
          break;
        }
      }
    }
  }
}

template <typename Container>
class swapback_eraser {
  public:
  explicit
  swapback_eraser(Container &container)
  : m_container {container}
  { }

  std::pair<typename Container::iterator, typename Container::iterator>
  operator () (const typename Container::iterator &it)
  {
    std::swap(*it, m_container.back());
    m_container.pop_back();
    return {it, m_container.end()};
  }

  private:
  Container &m_container;
};

void
mw::vision_processor::process()
{
  std::vector<sight> tsights = m_sights;
  std::vector<sight> nsights;
  std::vector<sight> osights;

  nsights.reserve(m_sights.size());
  osights.reserve(m_sights.size() * 2);
  while (not tsights.empty())
  {
    // process target sights (against initial sights)
    _process(get_source(),
             m_sights.begin(), m_sights.end(),
             tsights.begin(), tsights.end(),
             swapback_eraser {tsights},
             std::back_inserter(nsights));

    // write processed sights into output
    osights.insert(osights.end(), tsights.begin(), tsights.end());

    // new sights are the new target
    std::swap(tsights, nsights);
    nsights.clear();
  }

  m_sights = std::move(osights);
}

void
mw::vision_processor::apply(bool foreignsource, sight_container &tgtsights)
  const
{
  const circle &source = get_source();

  // fix dir-angles of foreign sights so that they conform with `this` processor
  if (foreignsource)
  {
    for (sight &s : tgtsights)
    {
      switch (s.tag)
      {
        case sight::circle:
        {
          const circle &circ = s.static_data.circle;
          const pt2d_d a = circ(s.sight_data.circle.cphi1);
          const pt2d_d b = circ(s.sight_data.circle.cphi2);
          s.phi1 = dirangle(a - source.center);
          s.phi2 = dirangle(b - source.center);
          // XXX cheating
          if (interval_size({s.phi1, s.phi2}) > M_PI)
          {
            std::swap(s.phi1, s.phi2);
            std::swap(s.sight_data.circle.cphi1, s.sight_data.circle.cphi2);
          }
          break;
        }
        case sight::line:
        {
          const line_segment &line = s.static_data.line;
          const pt2d_d a = line(s.sight_data.line.t1);
          const pt2d_d b = line(s.sight_data.line.t2);
          s.phi1 = dirangle(a - source.center);
          s.phi2 = dirangle(b - source.center);
          // XXX cheating
          if (interval_size({s.phi1, s.phi2}) > M_PI)
          {
            std::swap(s.phi1, s.phi2);
            std::swap(s.sight_data.line.t1, s.sight_data.line.t2);
          }
          break;
        }
      }
    }
  }

  std::vector<sight> tsights {tgtsights.begin(), tgtsights.end()};
  std::vector<sight> nsights;
  std::vector<sight> osights;

  nsights.reserve(tsights.size());
  osights.reserve(tsights.size() * 2);
  while (not tsights.empty())
  {
    // process target sights (against initial sights)
    _process(get_source(),
             m_sights.begin(), m_sights.end(),
             tsights.begin(), tsights.end(),
             swapback_eraser {tsights},
             std::back_inserter(nsights));

    // write processed sights into output
    osights.insert(osights.end(), tsights.begin(), tsights.end());

    // new sights are the new target
    std::swap(tsights, nsights);
    nsights.clear();
  }

  tgtsights = std::move(osights);
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
    // FIXME XXX causing vision bug with bullet glow being visible when it shouldnt
    //if (not boxinfo.is_source_inside and
        //not overlaps_inc({boxinfo.phi1, boxinfo.phi2}, {s.phi1, s.phi2}))
      //continue;

    line_segment line;
    double t1, t2;
    switch (s.tag)
    {
      case sight::line:
        line = s.static_data.line;
        t1 = s.sight_data.line.t1;
        t2 = s.sight_data.line.t2;
        break;

      case sight::circle:
        {
          const circle &circ = s.static_data.circle;
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

    int16_t xs[pts.size()], ys[pts.size()];
    for (size_t i = 0; i < pts.size(); ++i)
      std::tie(xs[i], ys[i]) = pt2d<int16_t>(world_to_target(pts[i]));
    filledPolygonColor(rend, xs, ys, pts.size(), color);
  }
  SDL_SetRenderDrawBlendMode(rend, oldblendmode);
}

