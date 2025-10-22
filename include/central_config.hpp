#ifndef CENTRAL_CONFIG_HPP
#define CENTRAL_CONFIG_HPP

#include <ether/ether.hpp>

namespace mw {

class central_config {
  public:
  static void
  set_config_file_path(const std::string &path)
  { m_path = path; }

  static central_config&
  instance();

  const eth::value& get_video_config() const { return m_video; }
  const eth::value& get_color_config() const { return m_colors; }
  const eth::value& get_textures_config() const { return m_textures; }
  const eth::value& get_controls_config() const { return m_controls; }

  private:
  central_config();

  static std::string m_path;

  eth::value m_config;
  eth::value m_video;
  eth::value m_colors;
  eth::value m_textures;
  eth::value m_controls;
}; // class mw::central_config

} // namespace mw

#endif
