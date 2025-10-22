#include "central_config.hpp"

#include <sstream>

#include <ether/sandbox.hpp>


std::string
mw::central_config::m_path = "./config.eth";

mw::central_config&
mw::central_config::instance()
{
  static central_config self;
  return self;
}

mw::central_config::central_config()
{
  eth::sandbox ether;
  std::ostringstream cmd;
  cmd << "load '" << m_path << "' | first";
  const eth::value conf = ether(cmd.str());
  m_video = conf["video"];
  m_colors = conf["colors"];
  m_textures = conf["textures"];
  m_controls = conf["controls"];
}

