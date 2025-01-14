#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include "ui_layer.hpp"
#include "sdl_environment.hpp"
#include "logging.h"

#include <SDL2/SDL.h>

#include <list>
#include <stdexcept>
#include <memory>

namespace mw {

class ui_manager {
  public:
  ui_manager(sdl_environment &sdl)
  : m_sdl {sdl}
  { }

  void
  add_layer(std::shared_ptr<ui_layer> layer)
  {
    m_layers.push_front(layer);
    layer->set_id(m_layers.begin());
  }

  void
  remove_layer(const ui_layer_id &id)
  {
    info("removing ui-layer");
    std::shared_ptr<ui_layer> layer = *id;
    m_layers.erase(id);
  }

  size_t
  get_n_layers() const noexcept
  { return m_layers.size(); }

  ui_layer_id
  get_top_layer_id() const
  {
    if (m_layers.empty())
      throw std::logic_error {"no layers available"};
    return m_layers.begin();
  }

  void
  add_float(std::shared_ptr<ui_float> flt)
  {
    m_floats.push_front(flt);
    flt->set_id(m_floats.begin());
  }

  void
  remove_float(const ui_float_id &id)
  {
    info("removing float");
    std::shared_ptr<ui_float> flt = *id;
    m_floats.erase(id);
  }

  void
  run()
  {
    while (not m_layers.empty())
    {
      m_layers.front()->run_layer(*this);
      for (const std::shared_ptr<ui_float> &flt : m_floats)
        flt->draw();
    }
  }

  void
  draw(const ui_layer_id &from) const
  {
    SDL_SetRenderDrawColor(m_sdl.get_renderer(), 0x00, 0x00, 0x00, 0);
    SDL_RenderClear(m_sdl.get_renderer());

    auto end = from;
    while (end != m_layers.end())
    {
      if ((*end)->get_size() == ui_layer::size::whole_screen)
      {
        ++end;
        break;
      }
      ++end;
    }

    for (auto it = --end; it != from; --it)
      (*it)->draw();
    (*from)->draw();
    for (const std::shared_ptr<ui_float> &flt : m_floats)
      flt->draw();
  }

  private:
  std::list<std::shared_ptr<ui_layer>> m_layers;
  std::list<std::shared_ptr<ui_float>> m_floats;
  sdl_environment &m_sdl;
}; // class mw::ui_manager

} // namespace mw

#endif
