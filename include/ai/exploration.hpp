/**
 * @file ai/exploration.hpp
 * @brief Algorithms modeling exploration of the area
 * @author Ivan Pidhurskyi <ivanpidhurskyi1997@gmail.com>
 */
#ifndef AI_EXPLORATION_HPP
#define AI_EXPLORATION_HPP

#include "area_map.hpp"
#include "vision.hpp"
#include "utl/grid.hpp"
#include "geometry.hpp"


namespace mw {
inline namespace ai {

/**
 * @brief Exploration alogorithm based on heat/Dijkstra maps.
 *
 * Algorithm
 * =========
 *
 * The algorithm is based on a heat/djekstra map. It tries to move towards a
 * direction that would yield higher gain of "information".
 *
 * Information is modeled via weights assigned to each cell. If weight of a cell
 * A is higher than that of a cell B, i.e. we know more about cell A than about
 * cell B, then it is more beneficial to move towards the cell B than to the
 * cell A. "Memeory" of a subject is then simply a weight-contents of a grid.
 * Loss of memeory is modeled via incremental decay of weights in cells outside
 * the immediate field of view. Weights of cells within the immedaite field of
 * view increase. It is important to note that sets of cells used for the steps
 * 1) "decide where to go" and 2) "mark what you see" must not be the same. And,
 * in fact, having the later one significantly lower than the former appears to
 * be crucial to achieve a plesant behaviour from the algorithm.
 *
 * In particular, direction to move is calculated as follows:
 * 1) for each cell in view assign a vector pointing from the subject to the
 *    cell with a magnitude equal to the weight of the cell;
 * 2) direction is a sum all the vectors from cells in view.
 * Note that the magnitude of this vector should not be interpreted or impact
 * any logics. The maxumum value of the magnitude is grid-dependent, and in a
 * limit of a grid with infinetesimal cells it approaches zero.
 *
 * @todo TODO further details
 * @todo TODO seed the grid with random noise
 */
class explorer {
  public:
  /**
   * @brief Initialize the algorithm for exploration of the @p map.
   * @param map A map to explore.
   * @param vision_radius Radius of an area impacting the "where to go".
   * @param mark_radius Radius of an area impacting the "what to mark". If lower
   *   than zero, the value will default to @p vision_radius/2.
   */
  explorer(const area_map &map, double vision_radius, double mark_radius = -1);

  double get_vision_radius() const noexcept { return m_vision_radius; }
  double get_mark_radius() const noexcept { return m_mark_radius; }

  /**
   * @brief Explore surroundings and get direction for further exploration.
   *
   * This is literally a "tick" of an algorithm described above.
   *
   * @param view Immediate field of view of the subject. Its radius must be
   * greater or equal to the `vision_radius` provided to the
   * @ref mw::ai::explorer::explorer() (exact radius of @p view doesn't matter).
   */
  vec2d_d
  operator () (const vision_processor &view);

  void
  draw_heatmap(SDL_Renderer *rend, const mapping &viewport) const;

  private:
  const area_map &m_map;
  grid<std::pair<bool, double>> m_heatmap;
  double m_vision_radius, m_mark_radius;
  double m_decay_factor;
  double m_mark_weight_base, m_mark_weight_extra;
}; // class mw::ai::exploration

} // inline namespace mw::ai
} // namespace mw

#endif
