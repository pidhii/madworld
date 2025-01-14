#include "color_manager.hpp"
#include "central_config.hpp"

#include <ether/sandbox.hpp>

#include <sstream>
#include <endian.h>


mw::color_manager&
mw::color_manager::instance()
{
  static color_manager self;
  return self;
}

mw::color_manager::color_manager()
{
  const eth::value conf = mw::central_config::instance().get_color_config();
  for (auto colors = eth::list(conf); not colors.is_nil(); colors = colors.cdr())
  {
    const eth::value entry = colors.car();
    m_colors.emplace(entry[0].str(), be32toh(entry[1]));
  }
}

