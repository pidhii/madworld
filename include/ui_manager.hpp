#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include "logging.h"
#include "sdl_environment.hpp"
#include "ui_layer.hpp"
#include "utl/frequency_limiter.hpp"
#include "utl/scheduler.hpp"
#include "video_manager.hpp"

#include <SDL2/SDL.h>

#include <list>
#include <stdexcept>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>


namespace mw {


class ui_manager: public scheduler<std::chrono::steady_clock, std::recursive_mutex> {
  public:
  ui_manager(sdl_environment &sdl): scheduler {m_run_lock}, m_sdl {sdl} { }
  ui_manager(): ui_manager {video_manager::instance().get_sdl()} { }

  std::recursive_mutex&
  get_run_lock() noexcept
  { return m_run_lock; }

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
  run(std::optional<mw::frequency_limiter> freqlimiter = std::nullopt)
  {
    while (not m_layers.empty())
    {
      if (freqlimiter.has_value())
      {
        std::chrono::milliseconds tsleep;
        if (not freqlimiter.value()(tsleep))
          std::this_thread::sleep_for(tsleep);
      }

      std::lock_guard _ {m_run_lock};
      m_layers.front()->run_layer(*this);
      run_single_task(*this);
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
  std::recursive_mutex m_run_lock;
  std::list<std::shared_ptr<ui_layer>> m_layers;
  std::list<std::shared_ptr<ui_float>> m_floats;
  sdl_environment &m_sdl;
}; // class mw::ui_manager


class temp_ui_layer {
  public:
  template <bool AddLayer = true>
  [[nodiscard]]
  temp_ui_layer(ui_manager &ui, const std::shared_ptr<ui_layer> &layer)
  : m_ui {ui}, m_layer {layer}
  { if constexpr (AddLayer) ui.add_layer(layer); }

  ~temp_ui_layer()
  { m_ui.remove_layer(m_layer->get_id()); }

  private:
  ui_manager &m_ui;
  std::shared_ptr<ui_layer> m_layer;
};

} // namespace mw

#endif
