#ifndef MAP_GENERATION_HPP
#define MAP_GENERATION_HPP

#include "map_generators/map_generator_m1.hpp"

namespace mw {

static void
generate_map(map_generator_m1 &mapgen, size_t seed /* 0 => random */,
             int map_width, int map_height,
             int n_vlines = 0, int n_hlines = 0,
             double wall_density = 0.7)
{
  seed = seed ? seed : std::random_device()();

  n_vlines = n_vlines ? n_vlines : map_width / 5;
  n_hlines = n_hlines ? n_hlines : map_height / 5;

  const double mindx = 1.5 / map_width;
  const double mindy = 1.5 / map_height;
  const double corridorw = 3. / map_width;
  const double corridorh = 3. / map_height;

  std::mt19937 gen {seed};
  mapgen.generate_lines(gen, n_vlines, n_hlines, mindx, mindy);
  mapgen.find_vertices();
  mapgen.generate_walls(gen, wall_density);
  mapgen.find_rooms();
  mapgen.remove_walls_inside_rooms();
  mapgen.extend_corridors(corridorw, corridorh);
  mapgen.find_rooms();
  mapgen.remove_walls_inside_rooms();

  while (mapgen.m_rooms.size() > 1)
  {
    mapgen.pinch_rooms(gen);
    mapgen.find_rooms();
  }

  mapgen.build_frame();
}

} // namespace mw

#endif
