#include "walls.hpp"


void
mw::basic_wall_impl::_draw_walls(const area_map &map,
    const std::vector<pt2d_d> &vertices, color_t color)
{
  SDL_Renderer *rend = map.get_sdl().get_renderer();
  for (size_t j = 1; j < vertices.size(); ++j)
  {
    const pt2d_i start = map.point_to_pixels(vertices[j-1]);
    const pt2d_i end = map.point_to_pixels(vertices[j]);
    aalineColor(rend, start.x, start.y, end.x, end.y, color);
  }
}
