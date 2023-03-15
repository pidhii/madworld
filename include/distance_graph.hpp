#ifndef DISTANCE_GRAPH_H
#define DISTANCE_GRAPH_H

#include "bit_array.hpp"
#include "sym_matrix.hpp"

class distance_graph {
  public:
  explicit
  distance_graph(size_t grid_width, size_t grid_height);

  //void
  //clear()
  //{ ... }

  void
  build(utl::bit_array terrain);

  private:
  size_t m_gwidth;
  size_t m_gheight;
  utl::sym_matrix<float> m_graph;
}; // class distance_graph

#endif
