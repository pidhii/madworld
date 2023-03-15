#include "distance_graph.hpp"


distance_graph::distance_graph(size_t grid_width, size_t grid_height)
: m_gwidth {grid_width},
  m_gheight {grid_height},
  m_graph {m_gwidth*m_gheight}
{ }

/*
static void
fill_distance_for_cell(struct distance_graph *dg, bit_array_t terrain, short i,
    short j)
{
  int width = dg->width;
  int height = dg->height;
  int ncells = width*height;

  int cellidx = i*width + j;
  float *celldata = dg->data + (cellidx*ncells);

  celldata[cellidx] = 0; // distance to itself = 0

  long *stack = malloc(sizeof(long)*0x1000);
  long *sp = stack;
  *sp = cellidx;

  while (TRUE)
  {
  }
}

void
fill_distance_graph(struct distance_graph *dg, bit_array_t terrain)
{

  for (short i = 0; i < dg->height; ++i)
  {
    for (short j = 0; j < dg->width; ++j)
    {
      fill_distance_for_cell(dg, terrain, i, j);
    }
  }
}

*/
