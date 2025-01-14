#ifndef GUI_UTILITIES_HPP
#define GUI_UTILITIES_HPP

#include "geometry.hpp"
#include "sdl_environment.hpp"
#include "common.hpp"
#include "gui/sdl_string.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <list>
#include <optional>
#include <stdexcept>

namespace mw {
inline namespace guiutl {

//struct placement {
  //public:
  //static placement
  //absolute(const vec2d_d &offs)
  //{
    //placement ret;
    //ret.m_type = placement_type::offset;
    //ret.m_x = offs.x;
    //ret.m_y = offs.y;
    //return ret;
  //}

  //static placement
  //relative(double x, double y)
  //{
    //placement ret;
    //ret.m_type = placement_type::ratio;
    //ret.m_x = x;
    //ret.m_y = y;
    //return ret;
  //}

  //pt2d_i
  //into_point(const rectangle &box) const
  //{
    //switch (m_type)
    //{
      //case placement_type::offset:
        //return pt2d_i(box.offset + vec2d_d(m_x, m_y));

      //case placement_type::ratio:
        //return pt2d_i(box.offset.x + box.width*m_x,
                      //box.offset.y + box.height*m_y);
    //}
    //abort();
  //}

  //private:
  //placement() = default;

  //private:
  //enum class placement_type { offset, ratio };
  //placement_type m_type;
  //double m_x, m_y;
//};


} // namespcae mw::guiutl
} // namespace mw

#endif
