#ifndef GUI_UTILITIES_HPP
#define GUI_UTILITIES_HPP

#include "common.hpp"

#include <SDL2/SDL.h>

#include <string_view>


#define MM_PER_INCH 25.4
#define CM_PER_INCH (MM_PER_INCH / 10)
#define PT_PER_INCH 72


namespace mw {
inline namespace gui {

std::pair<float, float>
unit_to_pix(float n_units, float units_per_inch);

static inline std::pair<float, float>
inch_to_pix(float inch)
{ return unit_to_pix(inch, 1); }

static inline std::pair<float, float>
mm_to_pix(float mm)
{ return unit_to_pix(mm, MM_PER_INCH); }

static inline std::pair<float, float>
cm_to_pix(float mm)
{ return unit_to_pix(mm, CM_PER_INCH); }

static inline std::pair<float, float>
pt_to_pix(float pt)
{ return unit_to_pix(pt, PT_PER_INCH); }

int
parse_size(std::string_view str);

mw::color_t
parse_color(std::string_view str);


} // namespcae mw::gui
} // namespace mw

#endif
