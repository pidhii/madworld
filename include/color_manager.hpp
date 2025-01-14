#ifndef COLOR_MANAGER_HPP
#define COLOR_MANAGER_HPP

#include "common.hpp"

#include <unordered_map>
#include <string>

namespace mw {


class color_manager {
  public:
  static color_manager&
  instance();

  color_t
  get_color(const std::string &key) const noexcept
  {
    auto it = m_colors.find(key);
    return it == m_colors.end() ? 0xFFFFFFFF : it->second;
  }

  color_t
  operator [] (const std::string &key) const noexcept
  { return get_color(key); }

  color_manager(const color_manager&) = delete;
  color_manager(color_manager&&) = delete;
  color_manager& operator = (const color_manager&) = delete;
  color_manager& operator = (color_manager&&) = delete;

  private:
  color_manager();

  std::unordered_map<std::string, color_t> m_colors;
}; // class mw::color_manager


} // namespace mw

#endif
