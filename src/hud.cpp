#include "hud.hpp"

#include <vector>


bool
mw::hud_footprint::contains(const pt2d_i pt, const_safe_pointer<hud_component> &hud)
  const noexcept
{
  for (const auto &[box, boxhud] : m_boxes)
  {
    if (box.contains(pt2d_d(pt)))
    {
      hud = boxhud;
      return true;
    }
  }
  return false;
}


mw::heads_up_display::~heads_up_display()
{
  for (hud_component *hud : m_huds)
    delete hud;
}

mw::hud_id
mw::heads_up_display::add_component(hud_component *hud) noexcept
{
  m_huds.push_back(hud);
  const hud_id id {--m_huds.end()};
  m_huds.back()->set_id(id);
  return id;
}

void
mw::heads_up_display::remove_component(hud_id id)
{
  (*id.get())->destroy();
  m_huds.erase(id.get()); }

void
mw::heads_up_display::update()
{
  std::vector<safe_pointer<hud_component>> weakrefs;
  weakrefs.reserve(m_huds.size());
  for (hud_component *hud : m_huds)
    weakrefs.emplace_back(hud->get_safe_pointer());

  for (safe_pointer<hud_component> safeptr : weakrefs)
  {
    if (safeptr)
      safeptr.get()->update(*this);
  }
}

void
mw::heads_up_display::draw(hud_footprint &area) const
{
  for (const hud_component *hud : m_huds)
  {
    rectangle hudbox;
    hud->draw(hudbox);
    area.add(hudbox, hud->get_safe_pointer());
  }
}

