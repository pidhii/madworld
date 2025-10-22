#include "ability.hpp"
#include "door.hpp"
#include "player.hpp"


void
mw::open_door::activate(area_map &map, player &user, const pt2d_d &pt)
{
  info("scanning for doors");
  for (phys_obstacle *pobs : map.get_phys_obstacles())
  {
    if (door *d = dynamic_cast<door*>(pobs))
    {
      const double dr = mag(user.get_position() - d->sample_point());
      info("dr = %f", dr);
      if (dr < 2)
      {
        info("opening door");
        d->open();
      }
    }
  }
}


void
mw::close_door::activate(area_map &map, player &user, const pt2d_d &pt)
{
  info("scanning for doors");
  for (phys_obstacle *pobs : map.get_phys_obstacles())
  {
    if (door *d = dynamic_cast<door*>(pobs))
    {
      const double dr = mag(user.get_position() - d->sample_point());
      info("dr = %f", dr);
      if (dr < 2)
      {
        info("closing door");
        d->close();
      }
    }
  }
}
