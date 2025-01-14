#ifndef VIDEO_MANAGER_HPP
#define VIDEO_MANAGER_HPP

#include "sdl_environment.hpp"
#include "common.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <ether/ether.hpp>

#include <optional>
#include <string>


namespace mw {

inline namespace gui {
  class basic_menu;
}

class video_config {
  public:
  struct config {
    struct { int w, h; } window_size;
    struct { int x, y; } resolution;
    bool fullscreen;
    bool hardware_acceleration;
    bool vsync;
    int fps_limit;
    struct { std::string path; int point_size; } font;
  };

  static video_config&
  instance();

  static eth::value
  dump_config(const config &cfg);

  const config&
  get_config() const noexcept
  { return m_config; }

  video_config(const video_config&) = delete;
  video_config(video_config&&) = delete;
  video_config& operator = (const video_config&) = delete;
  video_config& operator = (video_config&&) = delete;

  private:
  video_config();

  private:
  config m_config;

  friend class video_manager;
};

class fps_guardian_type {
  public:
  static inline fps_guardian_type&
  instance() noexcept
  {
    static fps_guardian_type self;
    return self;
  }

  bool
  operator () (time_t now) const
  { return now - m_prev_present_time >= _get_min_dt(); }

  void
  notify() noexcept;

  int
  get_fps() const noexcept
  { return m_last_fps.value_or(0); }

  double
  get_average_fps() const noexcept
  { return m_avg_fps.value_or(0); }

  void
  update_fps_limit() noexcept
  { m_fps_limit = std::nullopt; }

  fps_guardian_type(const fps_guardian_type&) = delete;
  fps_guardian_type(fps_guardian_type&&) = delete;
  fps_guardian_type& operator = (const fps_guardian_type&) = delete;
  fps_guardian_type& operator = (fps_guardian_type&&) = delete;

  private:
  fps_guardian_type()
  : m_prev_present_time {0},
    m_cur_second {0},
    m_frame_count {0}
  { }

  time_t
  _get_min_dt() const noexcept
  {
    if (not m_fps_limit.has_value())
    {
      m_fps_limit = video_config::instance().get_config().fps_limit;
      return ceil(1000./m_fps_limit.value());
    }
    else
      return ceil(1000./m_fps_limit.value());
  }

  mutable std::optional<int> m_fps_limit;
  time_t m_prev_present_time;
  time_t m_cur_second;
  int m_frame_count;
  std::optional<double> m_avg_fps;
  std::optional<int> m_last_fps;
}; // class mw:;detail::fps_guardian_type
extern fps_guardian_type &fps_guardian;


class video_manager {
  public:
  static video_manager&
  instance();

  void
  init();

  sdl_environment&
  get_sdl();

  TTF_Font*
  get_font() const noexcept { return m_font; }

  TTF_Font*
  get_small_font() const noexcept { return m_small_font; }

  gui::basic_menu*
  make_new_settings();

  video_manager(const video_manager&) = delete;
  video_manager(video_manager&&) = delete;
  video_manager& operator = (const video_manager&) = delete;
  video_manager& operator = (video_manager&&) = delete;

  private:
  video_manager(): m_sdl { } { }
  ~video_manager();

  sdl_environment m_sdl;
  TTF_Font *m_font, *m_small_font;
};


} // namespace mw

#endif
