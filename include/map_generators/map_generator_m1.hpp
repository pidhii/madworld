#ifndef MW_MAP_GENERATORS_MAP_GENERATOR_M1_HPP
#define MW_MAP_GENERATORS_MAP_GENERATOR_M1_HPP

#include "geometry.hpp"
#include "video_manager.hpp"
#include "algorithms/forest.hpp"
#include "area_map.hpp"
#include "walls.hpp"

#include <random>
#include <map>
#include <set>
#include <algorithm>


namespace mw {

class map_generator_m1 {
  using forest_traits = mw::forest_traits<mw::find_method::path_compression,
                                          mw::union_method::by_rank>;
  public:
  size_t
  vertex_index(size_t kv, size_t kh) const noexcept;

  std::pair<size_t, size_t>
  vertex_location(size_t vertexidx) const noexcept;

  size_t
  block_index(size_t kv, size_t kh) const noexcept;

  std::pair<size_t, size_t>
  block_location(size_t blockidx) const noexcept;

  size_t
  wall_index(size_t avtxidx, size_t bvtxidx) const noexcept;

  std::pair<size_t, size_t>
  wall_vertices(size_t wallidx) const noexcept;

  void
  block_contour(size_t iv, size_t ih, std::vector<size_t> &walls) const;

  void
  block_contour(size_t blockidx, std::vector<size_t> &walls) const;

  size_t
  common_wall(size_t ablockidx, size_t bblockidx) const;

  std::vector<std::vector<size_t>>
  room_contours(size_t roomid) const;

  template <typename Generator>
  void
  generate_lines(Generator &gen, int nvlines, int nhlines, double mindx,
                 double mindy);

  void
  find_vertices();

  template <typename Generator>
  void
  generate_walls(Generator &gen, double walldens);

  void
  build_frame();

  void
  find_rooms();

  void
  remove_walls_inside_rooms();

  private:
  template <typename TestRoom /* bool(contour) */,
            typename Seek /* std::pair<size_t, size_t>(size_t iv, size_t ih) */>
  void
  _extend_corridors(TestRoom testroom, Seek seek);

  public:
  void
  extend_corridors(double maxw, double maxh);

  template <typename Generator>
  void
  pinch_rooms(Generator &gen)
  {
    std::vector<std::pair<size_t, size_t>> blockbuf;
    std::vector<size_t> wallbuf;
    for (const auto &[roomid, roomblocks] : m_rooms)
    {
      blockbuf.clear();
      std::copy(roomblocks.begin(), roomblocks.end(),
                std::back_inserter(blockbuf));
      std::shuffle(blockbuf.begin(), blockbuf.end(), gen);
      for (const auto &[iv, ih] : blockbuf)
      {
        block_contour(iv, ih, wallbuf);
        std::shuffle(wallbuf.begin(), wallbuf.end(), gen);
        for (const size_t wallidx : wallbuf)
        {
          if (m_walls[wallidx])
          {
            m_walls[wallidx] = false;
            goto end_room_iteration;
          }
        }
      }

end_room_iteration:;
    }
  }

  void
  apply(area_map &map) const
  {
    const mw::mapping viewport {{0., 0.}, map.get_width(), map.get_height()};
    for (size_t i = 0; i < m_vertices.size(); ++i)
    {
      for (size_t j = i + 1; j < m_vertices.size(); ++j)
      {
        if (not m_walls[wall_index(i, j)])
          continue;

        const pt2d_d a = viewport(m_vertices[i]),
                     b = viewport(m_vertices[j]);
        basic_wall<> *wall = new basic_wall<> {{a, b}};
        object_id obsid = map.add_static_object(wall);
        map.register_phys_obstacle(obsid);
        map.register_vis_obstacle(obsid);
      }
    }
  }

  //protected:
  std::vector<double> m_vlines, m_hlines;
  std::vector<mw::pt2d_d> m_vertices;
  std::vector<bool> m_walls;
  std::map<size_t, std::set<std::pair<size_t, size_t>>> m_rooms;
  mw::forest<size_t, forest_traits> m_roomsdsf;

  friend void apply(const map_generator_m1&, mw::area_map&);
};



inline size_t
map_generator_m1::vertex_index(size_t kv, size_t kh) const noexcept
{ return kh + kv*m_hlines.size(); }

inline std::pair<size_t, size_t>
map_generator_m1::vertex_location(size_t vertexidx) const noexcept
{
  const lldiv_t qr = lldiv(vertexidx, m_hlines.size());
  return {qr.quot, qr.rem};
}

inline size_t
map_generator_m1::block_index(size_t kv, size_t kh) const noexcept
{ return kh + kv*(m_hlines.size() - 1); }

inline std::pair<size_t, size_t>
map_generator_m1::block_location(size_t blockidx) const noexcept
{
  const lldiv_t qr = lldiv(blockidx, m_hlines.size() - 1);
  return {qr.quot, qr.rem};
}

inline size_t
map_generator_m1::wall_index(size_t avtxidx, size_t bvtxidx) const noexcept
{
  if (avtxidx > bvtxidx)
    std::swap(avtxidx, bvtxidx);
  return avtxidx + bvtxidx*m_vertices.size();
}

inline std::pair<size_t, size_t>
map_generator_m1::wall_vertices(size_t wallidx) const noexcept
{
  const lldiv_t qr = lldiv(wallidx, m_vertices.size());
  return {qr.quot, qr.rem};
}

template <typename Generator>
void
map_generator_m1::generate_lines(Generator &gen, int nvlines, int nhlines,
                                 double mindx, double mindy)
{
  std::uniform_real_distribution<double> dist {0, 1};
  m_vlines.resize(nvlines);
  m_hlines.resize(nhlines);

  // generate horizontal and vertical lines at random offsets
  std::generate(m_vlines.begin(), m_vlines.end(), std::bind(dist, std::ref(gen)));
  std::generate(m_hlines.begin(), m_hlines.end(), std::bind(dist, std::ref(gen)));

  // sort so that adjacent indices are adjacent geometrically
  std::sort(m_vlines.begin(), m_vlines.end());
  std::sort(m_hlines.begin(), m_hlines.end());

  // remove lines that are too close geometrically
  m_vlines.erase(std::remove_if(m_vlines.begin() + 1, m_vlines.end(),
                 [mindx, prev=m_vlines.front()] (double x) mutable {
      if (x - prev < mindx)
        return true;
      prev = x;
      return false;
  }), m_vlines.end());
  m_hlines.erase(std::remove_if(m_hlines.begin() + 1, m_hlines.end(),
                 [mindy, prev=m_hlines.front()] (double x) mutable {
      if (x - prev < mindy)
        return true;
      prev = x;
      return false;
  }), m_hlines.end());
}

template <typename Generator>
void
map_generator_m1::generate_walls(Generator &gen, double walldens)
{
  std::uniform_real_distribution<double> dist {0, 1};

  // generate random walls
  m_walls.assign(m_vertices.size()*m_vertices.size(), false);
  int nwalls = 0;
  for (size_t iv = 0; iv < m_vlines.size(); ++iv)
  {
    for (size_t ih = 0; ih < m_hlines.size(); ++ih)
    {
      const size_t aidx = vertex_index(iv, ih);
      // vertical wall (vtx_a -> down)
      if (iv + 1 < m_vlines.size())
      {
        const size_t bidx = vertex_index(iv + 1, ih);
        if (dist(gen) < walldens)
        {
          m_walls[wall_index(aidx, bidx)] = true;
          nwalls += 1;
        }
      }
      // horizontal wall (vtx_a -> right)
      if (ih + 1 < m_hlines.size())
      {
        const size_t bidx = vertex_index(iv, ih + 1);
        if (dist(gen) < walldens)
        {
          m_walls[wall_index(aidx, bidx)] = true;
          nwalls += 1;
        }
      }
    }
  }
}

inline void
map_generator_m1::block_contour(size_t blockidx, std::vector<size_t> &walls)
  const
{
  const auto [v, h] = block_location(blockidx);
  block_contour(v, h, walls);
}

}

#endif
