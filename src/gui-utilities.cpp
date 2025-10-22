#include "gui/utilities.hpp"
#include "logging.h"

#include <stdexcept>
#include <algorithm>



std::pair<float, float>
mw::gui::unit_to_pix(float n_units, float units_per_inch)
{
  float hdpi, vdpi;
  if (SDL_GetDisplayDPI(0, nullptr, &hdpi, &vdpi) < 0)
  {
    error("%s", SDL_GetError());
    return {1, 1};
  }
  return {n_units * hdpi / units_per_inch, n_units * vdpi / units_per_inch};
}


int
mw::gui::parse_size(std::string_view str)
{
  const auto safe_isdigit = [] (char c) { return std::isdigit(unsigned(c)); };
  const auto it = std::find_if_not(str.begin(), str.end(), safe_isdigit);
  const auto prefix = str.substr(0, it - str.begin());
  const auto suffix = str.substr(it - str.begin());

  const float val = std::strtod(prefix.data(), nullptr);
  if (suffix == "" or suffix == "pix")
    return val;
  else if (suffix == "in")
    return inch_to_pix(val).first;
  else if (suffix == "pt")
    return pt_to_pix(val).first;
  else if (suffix == "cm")
    return cm_to_pix(val).first;
  else if (suffix == "mm")
    return mm_to_pix(val).first;

  error("invalid units: '%s'", suffix.data());
  throw std::runtime_error {"invalid units"};
}
