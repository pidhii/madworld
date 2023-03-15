#include "color_manager.hpp"

#include <ether/sandbox.hpp>

#include <sstream>
#include <endian.h>


mw::color_manager&
mw::color_manager::instance()
{
  static bool is_first_time = true;
  static color_manager self;
  if (is_first_time)
  {
    self.load_config("./config.eth");
    is_first_time = false;
  }
  return self;
}

void
mw::color_manager::load_config(const std::string &path)
{
  eth::sandbox ether;

  std::ostringstream cmd;
  cmd << "first $ load '" << path << "'";
  const eth::value conf = ether(cmd.str());

  for (auto colors = conf["colors"]; not colors.is_nil(); colors = colors.cdr())
  {
    const eth::value entry = colors.car();
    m_colors.emplace(entry[0].str(), be32toh(entry[1]));
  }
}

