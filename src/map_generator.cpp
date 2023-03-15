#include "map_generation.hpp"
#include "exceptions.hpp"

#include <bitset>


bool
mw::map_generator::is_within(const carbon_grid &cg) const noexcept
{
  int xbegin, xend, ybegin, yend;
  cg.get_x_range(xbegin, xend);
  cg.get_y_range(ybegin, yend);

  return
    xbegin >= 0 and xend <= m_w and
    ybegin >= 0 and yend <= m_h;
}

void
mw::map_generator::blind_write(const carbon_grid &cg)
{
  if (not is_within(cg))
    throw exception {"room outside the main grid"};

  int xbegin, xend, ybegin, yend;
  cg.get_x_range(xbegin, xend);
  cg.get_y_range(ybegin, yend);
  for (int x = xbegin; x < xend; ++x)
  {
    for (int y = ybegin; y < yend; ++y)
      m_mg.set({x, y}, cg.get({x, y}));
  }
}

static uint8_t
_overlay_cells(uint8_t dst, uint8_t src,
    mw::map_generator::overlay_data &ovdata)
{
  using namespace mw;

  const uint8_t dstor = dst & mw::orientation_mask;
  const uint8_t srcor = src & mw::orientation_mask;
  const uint8_t dstval = dst & mw::value_mask;
  const uint8_t srcval = src & mw::value_mask;

  static uint8_t table[4][4] = {
  /*              nothing  issue   wall  doorway */
  /* nothing */ { nothing, issue,  wall, doorway },
  /* issue   */ {   issue, issue, issue,   issue },
  /* wall    */ {    wall, issue,  wall,    wall },
  /* doorway */ { doorway, issue,  wall, doorway },
  };

  const uint8_t resval = table[dstval >> 4][srcval >> 4];
  uint8_t resor;
  switch (resval)
  {
    case nothing:
      resor = 0;
      break;

    case issue:
      ovdata.n_errors += 1;
      return mw::issue;

    case wall:
      if (dstval == nothing or srcval == nothing)
      {
        resor = dstor | srcor;
        ovdata.n_new_walls += 1;
      }
      else // dstval == wall and srcval == wall
      {
        if (dstor == srcor)
          resor = dstor;
        else
        {
          ovdata.n_errors += 1;
          return mw::issue;
        }
      }
      break;

    case doorway:
      resor = 0;
      if (dstval == doorway and srcval == doorway)
        ovdata.n_ovl_doorways += 1;
      break;
  }
  return resval | resor;
}

static uint8_t
_overlay_cells(uint8_t dst, uint8_t src)
{
  mw::map_generator::overlay_data ovdata;
  return _overlay_cells(dst, src, ovdata);
}

void
mw::map_generator::overlay(const carbon_grid &cg, overlay_data &ovdata)
{
  int xbegin, xend, ybegin, yend;
  cg.get_x_range(xbegin, xend);
  cg.get_y_range(ybegin, yend);
  for (int x = xbegin; x < xend; ++x)
  {
    for (int y = ybegin; y < yend; ++y)
      _overlay_cells(m_mg.get({x, y}), cg.get({x, y}), ovdata);
  }
}

void
mw::map_generator::write(const carbon_grid &cg)
{
  overlay_data ovdata;
  overlay(cg, ovdata);

  if (ovdata.n_errors > 0)
    throw exception {"failed to overlay a room over the main grid"};

  int xbegin, xend, ybegin, yend;
  cg.get_x_range(xbegin, xend);
  cg.get_y_range(ybegin, yend);
  for (int x = xbegin; x < xend; ++x)
  {
    for (int y = ybegin; y < yend; ++y)
    {
      const uint8_t c = _overlay_cells(m_mg.get({x, y}), cg.get({x, y}));
      m_mg.set({x, y}, c);
    }
  }
}

