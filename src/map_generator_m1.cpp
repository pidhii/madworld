#include "map_generators/map_generator_m1.hpp"

#include <random>
#include <algorithm>
#include <chrono>
#include <set>
#include <limits>
#include <unordered_set>
#include <assert.h>


void
mw::map_generator_m1::find_vertices()
{
  // collect intersection vertices
  m_vertices.resize(m_vlines.size() * m_hlines.size());
  std::vector<mw::line_segment> vsegms, hsegms;
  std::transform(m_vlines.begin(), m_vlines.end(), std::back_inserter(vsegms),
                 [] (double x) { return mw::line_segment {{x, 0}, {0, 1}}; });
  std::transform(m_hlines.begin(), m_hlines.end(), std::back_inserter(hsegms),
                 [] (double y) { return mw::line_segment {{0, y}, {1, 0}}; });
  for (size_t iv = 0; iv < m_vlines.size(); ++iv)
  {
    const mw::line_segment &v = vsegms[iv];
    for (size_t ih = 0; ih < m_hlines.size(); ++ih)
    {
      const mw::line_segment &h = hsegms[ih];
      double tv = 0, th = 0;
      intersect(v, h, tv, th);
      m_vertices[vertex_index(iv, ih)] = v(tv);
    }
  }
}


void
mw::map_generator_m1::find_rooms()
{ // NOTE: Below we operate with quadruplets of adjacent vertices (blocks),
  // which naturally constitute primitive building blocks of rooms (whatever
  // is the actual shape of the room, it can be represented as a set of these
  // blocks). However, these "blocks" are not stored anywhere explicity.
  // However, each block is identified by pair of indices (row and column) as
  // all blocks naturally form a "matrix", and can be reconstructed on the
  // fly given the block indices.
  const size_t nblocks = (m_vlines.size() - 1) * (m_hlines.size() - 1);
  m_roomsdsf.resize(nblocks);

  // join() all pairs of adjacent blocks without a wall in-between
  for (size_t iv = 0; iv < m_vlines.size() - 1; ++iv)
  {
    for (size_t ih = 0; ih < m_hlines.size() - 1; ++ih)
    {
      const size_t ablockidx = block_index(iv, ih);
      // horizontally adjacent blocks
      if (iv + 1 < m_vlines.size() - 1)
      {
        const size_t bblockidx = block_index(iv + 1, ih);
        const size_t wallidx = wall_index(vertex_index(iv + 1, ih),
                                          vertex_index(iv + 1, ih + 1));
        if (not m_walls[wallidx])
          m_roomsdsf.join(ablockidx, bblockidx);
      }
      // verically adjacent blocks
      if (ih + 1 < m_hlines.size() - 1)
      {
        const size_t bblockidx = block_index(iv, ih + 1);
        const size_t wallidx = wall_index(vertex_index(iv, ih + 1),
                                          vertex_index(iv + 1, ih + 1));
        if (not m_walls[wallidx])
          m_roomsdsf.join(ablockidx, bblockidx);
      }
    }
  }

  // extract rooms from the m_roomsdsf
  m_rooms.clear();
  for (size_t iv = 0; iv < m_vlines.size() - 1; ++iv)
  {
    for (size_t ih = 0; ih < m_hlines.size() - 1; ++ih)
    {
      const size_t blockidx = block_index(iv, ih);
      m_rooms[m_roomsdsf.find(blockidx)].emplace(iv, ih);
    }
  }
}

void
mw::map_generator_m1::block_contour(size_t iv, size_t ih,
                                    std::vector<size_t> &walls) const
{
  // a -- b
  // |    |
  // d -- c
  const size_t a = vertex_index(iv, ih),
               b = vertex_index(iv + 1, ih),
               c = vertex_index(iv + 1, ih + 1),
               d = vertex_index(iv, ih + 1);
  const size_t ab = wall_index(a, b),
               bc = wall_index(b, c),
               cd = wall_index(c, d),
               da = wall_index(d, a);
  walls = {ab, bc, cd, da};
}

size_t
mw::map_generator_m1::common_wall(size_t ablockidx, size_t bblockidx) const
{
  if (ablockidx == bblockidx)
    throw std::runtime_error {"common_wall(X, X)"};
  std::vector<size_t> awalls, bwalls;
  block_contour(ablockidx, awalls);
  block_contour(bblockidx, bwalls);
  for (const size_t a : awalls)
  {
    for (const size_t b : bwalls)
    {
      if (a == b)
        return a;
    }
  }
  throw std::runtime_error {"common_wall(X, Y) where X not adj to Y"};
}

void
mw::map_generator_m1::remove_walls_inside_rooms()
{
  std::unordered_set<size_t> mem;
  std::vector<size_t> blockwalls;
  for (const auto &[_, blocks] : m_rooms)
  {
    mem.clear();
    mem.reserve(blocks.size()*3);
    for (const auto &[iv, ih] : blocks)
    {
      block_contour(iv, ih, blockwalls);
      for (size_t wallidx : blockwalls)
      {
        const auto [_, isnew] = mem.emplace(wallidx);
        if (not isnew)
          m_walls[wallidx] = false;
      }
    }
  }
}

std::vector<std::vector<size_t>>
mw::map_generator_m1::room_contours(size_t roomid) const
{
  std::vector<size_t> buf;

  std::multiset<size_t> memwalls;

  // collect walls of every single block constituting the room
  for (const auto &[iv, ih] : m_rooms.at(roomid))
  {
    block_contour(iv, ih, buf);
    for (const size_t wallidx : buf)
    {
      //if (m_walls[wallidx])
        memwalls.insert(wallidx);
    }
  }

  // trace connected vertices on the contours
  static thread_local mw::forest<size_t, forest_traits> vertexdsf;
  vertexdsf.resize(m_vertices.size()); // XXX FIXME total overkill
  for (const size_t wallidx : memwalls)
  {
    // contour walls can only appear once, the rest is internal to the room
    if (memwalls.count(wallidx) == 1)
    {
      const auto [a, b] = wall_vertices(wallidx);
      vertexdsf.join(a, b);
    }
  }

  // extract (fragmented) contours: walls without particular order
  std::map<size_t, std::vector<size_t>> fragmcontours;
  for (const size_t wallidx : memwalls)
  {
    if (memwalls.count(wallidx) == 1)
    {
      const auto [a, _] = wall_vertices(wallidx);
      fragmcontours[vertexdsf.find(a)].push_back(wallidx);
    }
  }

  // extract ordered contour vertices
  std::vector<std::vector<size_t>> contours;
  for (auto &[contourid, framgs] : fragmcontours)
  {
    std::vector<size_t> &contour = contours.emplace_back();
    // reorder fragments s.t. adjacent walls are adjacent in the arrays
    // 1. start with some fragment, A;
    // 2. find another fragment, B, sharing a vertex with A;
    // 3. put fragment B next to A;
    // 4. repeat from setp (2) w.r.t. fragment B and ALTERNATIVE vertex
    // (saving vertices in the order of processing will ensure correct vertex
    // ordering on the contour)
    const auto [firstvtx, secondvtx] = wall_vertices(framgs.front());
    contour.push_back(firstvtx);
    size_t curvtx = secondvtx;
    for (auto ait = framgs.begin(); ait != framgs.end() - 1; ++ait)
    {
      // save vertex
      contour.push_back(curvtx);
      // ... find B
      const auto bit = std::find_if(ait + 1, framgs.end(), [&] (size_t wallidx) {
          const auto [ba, bb] = wall_vertices(wallidx);
          if (curvtx == ba)
          {
            curvtx = bb;
            return true;
          }
          if (curvtx == bb)
          {
            curvtx = ba;
            return true;
          }
          return false;
      });
      // ... put B next to A
      std::swap(*(ait + 1), *bit);
    }
  }

  // filter contours: keep only corners
  for (std::vector<size_t> &contour : contours)
  {
    for (size_t i = 0; i < contour.size();)
    {
      // filter it based on adjacent triplets
      const auto [av, ah] = vertex_location(contour[i]);
      const auto [bv, bh] = vertex_location(contour[(i+1) % contour.size()]);
      const auto [cv, ch] = vertex_location(contour[(i+2) % contour.size()]);
      if ((av == bv and bv == cv) or (ah == bh and bh == ch))
      {
        contour.erase(contour.begin() + (i+1) % contour.size());
        // adjust `i` when erasing with a flip over the end
        if (i + 1 == contour.size())
          i -= 1;
      }
      else
        i += 1;
    }
  }

  return contours;
}

template <typename TestRoom /* bool(contour) */,
          typename Seek /* std::pair<size_t, size_t>(size_t iv, size_t ih) */>
void
mw::map_generator_m1::_extend_corridors(TestRoom testroom, Seek seek)
{
  for (const auto &[roomid, roomblocks] : m_rooms)
  {
    if (not testroom(roomid))
      continue;

    assert(roomblocks.size() > 0);
    auto [kv, kh] = *roomblocks.begin();
    size_t seekid = roomid;
    size_t oldblock = block_index(kv, kh),
           newblock = block_index(kv, kh);

    assert(m_roomsdsf.find(block_index(kv, kh)) == roomid);
    do {
      std::tie(kv, kh) = seek(kv, kh);
      // stop seek if went out of boundaries
      if (kv >= m_vlines.size() - 1 or kh >= m_hlines.size() - 1)
        break;

      oldblock = newblock;
      newblock = block_index(kv, kh);
      seekid = m_roomsdsf.find(newblock);
    } while (seekid == roomid);

    if (seekid == roomid)
      // havent found anything
      continue;

    if (testroom(seekid))
      m_walls[common_wall(oldblock, newblock)] = false;
  }
}

void
mw::map_generator_m1::extend_corridors(double maxw, double maxh)
{
  const auto dimentions = [&] (const std::vector<size_t> &contour)
    -> std::pair<double, double> {
    double w = 0, h = 0;
    for (int i = 0; i < 3; ++i)
    {
      const mw::pt2d_d a = m_vertices[contour[i]];
      const mw::pt2d_d b = m_vertices[contour[i+1]];
      if (w == 0) w = std::abs(a.x - b.x);
      if (h == 0) h = std::abs(a.y - b.y);
    }
    return {w, h};
  };

  // horizontal corridors
  _extend_corridors(
    [&] (size_t roomid) -> bool {
      const std::vector<std::vector<size_t>> contours = room_contours(roomid);
      return contours.size() == 1 and contours[0].size() == 4
         and dimentions(contours[0]).second < maxh;
    },
    [&] (size_t v, size_t h) -> std::pair<size_t, size_t> {
      return {v + 1, h};
    }
  );

  // vertical corridors
  _extend_corridors(
    [&] (size_t roomid) -> bool {
      const std::vector<std::vector<size_t>> contours = room_contours(roomid);
      return contours.size() == 1 and contours[0].size() == 4
         and dimentions(contours[0]).first < maxw;
    },
    [&] (size_t v, size_t h) -> std::pair<size_t, size_t> {
      return {v, h + 1};
    }
  );
}

void
mw::map_generator_m1::build_frame()
{
  // clse edges with walls
  for (size_t iv = 0; iv < m_vlines.size()-1; ++iv)
  {
    {
      const size_t aidx = vertex_index(iv, 0);
      const size_t bidx = vertex_index(iv+1, 0);
      m_walls[wall_index(aidx, bidx)] = true;
    }
    {
      const size_t aidx = vertex_index(iv, m_hlines.size()-1);
      const size_t bidx = vertex_index(iv+1, m_hlines.size()-1);
      m_walls[wall_index(aidx, bidx)] = true;
    }
  }
  for (size_t ih = 0; ih < m_hlines.size()-1; ++ih)
  {
    {
      const size_t aidx = vertex_index(0, ih);
      const size_t bidx = vertex_index(0, ih+1);
      m_walls[wall_index(aidx, bidx)] = true;
    }
    {
      const size_t aidx = vertex_index(m_vlines.size()-1, ih);
      const size_t bidx = vertex_index(m_vlines.size()-1, ih+1);
      m_walls[wall_index(aidx, bidx)] = true;
    }
  }
}

