#include "ai/exploration.hpp"


struct _decayer {
  const double decay_factor;

  _decayer(double decay_factor)
  : decay_factor {decay_factor}
  { }

  void
  operator () (mw::grid_view<std::pair<bool, double>> &gw) const
  {
    if (gw.is_leaf())
    {
      auto [iswall, weight] = gw.get_value();
      const double newweight = weight * decay_factor;
      gw.set_value({iswall, newweight});
    }
    else
      gw.scan(*this);
  }
}; // struct _decayer


struct _scanner {
  struct box_sight_data {
    box_sight_data(const mw::rectangle &box, double &weight)
    : is_processed {false}, box {box}, weight {weight}
    { }

    bool is_processed;
    mw::rectangle box;
    double &weight;
  };

  const mw::vision_processor &visproc;
  const double r_scan, r_update, decay_factor, base, extra;
  mw::vec2d_d pull;
  std::list<box_sight_data> bsds;
  std::list<mw::sight> sights;

  _scanner(const mw::vision_processor &visproc, double r_scan,
      double r_update, double decay_factor, double base,
      double extra)
  : visproc {visproc},
    r_scan {r_scan},
    r_update {r_update},
    decay_factor {decay_factor},
    base {base},
    extra {extra},
    pull {0, 0}
  { }

  void
  scan(mw::grid<std::pair<bool, double>> &g)
  {
    g.scan(*this);

    const mw::pt2d_d source = visproc.get_source().center;

    visproc.apply(source, sights);
    for (mw::sight &s : sights)
    {
      box_sight_data &bsd =
        *const_cast<box_sight_data*>(
            reinterpret_cast<const box_sight_data*>(s.static_data->obs));

      if (bsd.is_processed)
        continue;
      bsd.is_processed = true;

      const double memweight = bsd.weight * decay_factor;
      const double visweight =
        base + extra*(1 - mw::mag(source - bsd.box.center())/r_scan);
      const double newweight = std::max(visweight, memweight);

      const mw::vec2d_d dir =
        normalized(bsd.box.center() - source);
      const double pullmag = (base + extra - bsd.weight)*bsd.box.width*bsd.box.height;
      pull = pull + dir*pullmag;

      if (overlap_box_circle(bsd.box, {source, r_update}))
        bsd.weight = newweight;
    }

    pull = normalized(pull);
  }

  void
  operator () (mw::grid_view<std::pair<bool, double>> &gw)
  {
    if (not overlap_box_circle(gw.get_box(), {visproc.get_source().center, r_scan}))
    {
      gw.scan(_decayer {decay_factor});
      return;
    }

    if (gw.is_leaf())
    {
      auto [iswall, weight] = gw.get_value();
      const mw::rectangle &box = gw.get_box();

      if (iswall)
        return;

      const double reprradius = std::min(box.width/2, box.height/2) / 10;
      const mw::circle boxrepr {box.center(), reprradius};
      mw::sight s;
      s.is_transparent = true;
      if (not mw::cast_sight(visproc.get_source(), boxrepr, s))
      { // decay
        const double memweight = weight * decay_factor;
        gw.set_value({iswall, memweight});
        return;
      }

      bsds.emplace_back(gw.get_box(), gw.get_value_ref().second);
      s.static_data->obs = reinterpret_cast<mw::vis_obstacle*>(&bsds.back());
      sights.emplace_back(s);
    }
    else
      gw.scan(*this);
  }
}; // struct _scanner


static std::pair<bool, double>
_into_heatmap(bool iswall, const mw::rectangle &box)
{ return {iswall, 0}; }

static void
_draw_heatmap(SDL_Renderer *rend, const mw::mapping &viewport,
    const mw::grid<std::pair<bool, double>> &heatmap, double wmax, double wmin)
{
  SDL_BlendMode oldblend;
  SDL_GetRenderDrawBlendMode(rend, &oldblend);
  SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
  heatmap.for_each([&] (const std::pair<bool, double> &v, const mw::rectangle &box) {
      SDL_Rect pixbox = viewport(box);
      if (v.first)
      {
        SDL_SetRenderDrawColor(rend, 0xAA, 0x33, 0x33, 0x70);
        SDL_RenderDrawRect(rend, &pixbox);
      }
      else if (v.second >= 0)
      {
      const double alpha = std::max(wmax - v.second, wmin)/(wmax - wmin);
      const uint8_t r = 0xFF*alpha;
      const uint8_t g = 0xFF*(1 - alpha);
      SDL_SetRenderDrawColor(rend, r, g, 0x00, 0x33);
      SDL_RenderFillRect(rend, &pixbox);
      }
      else
      {
        //SDL_SetRenderDrawColor(rend, 0x00, 0xFF, 0x00, 0x10);
        //SDL_RenderDrawRect(rend, &pixbox);
      }
  });
  SDL_SetRenderDrawBlendMode(rend, oldblend);
}

mw::ai::explorer::explorer(const area_map &map, double vision_radius,
    double mark_radius)
: m_map {map},
  m_heatmap {map.get_grid().map<std::pair<bool, double>>(_into_heatmap)},
  m_vision_radius {vision_radius},
  m_mark_radius {mark_radius < 0 ? vision_radius/2 : mark_radius},
  m_decay_factor {0.9999},
  m_mark_weight_base {100},
  m_mark_weight_extra {20}
{ }

mw::vec2d_d
mw::ai::explorer::operator()(const vision_processor &view)
{
  _scanner sc {view, m_vision_radius, m_mark_radius, m_decay_factor,
    m_mark_weight_base, m_mark_weight_extra};
  sc.scan(m_heatmap);
  return sc.pull;
}

void
mw::ai::explorer::draw_heatmap(SDL_Renderer *rend, const mapping &viewport)
  const
{
  const double max_weight = m_mark_weight_base + m_mark_weight_extra;
  const double min_weight = 0;
  _draw_heatmap(rend, viewport, m_heatmap, max_weight, min_weight);
}
