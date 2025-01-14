#ifndef UTL_DYNAMIC_GRID_HPP
#define UTL_DYNAMIC_GRID_HPP

#include "utl/huge_counter.hpp"
#include "exceptions.hpp"

#include "boost/format.hpp"


namespace mw {
inline namespace utl {

template <typename T, typename Timestamp = huge_counter<10, uintmax_t>>
class dynamic_grid {
  public:
  static constexpr char class_name[] = "mw::utl::dynamic_grid";
  using exception = scoped_exception<class_name>;

  using timestamp_type = Timestamp;
  using value_type = T;
  using values_collection = std::vector<value_type>;

  private:
  struct cell
  {
    timestamp_type timestamp;
    // The layout of values is the following:
    // [ <static#1>, <static#2>, ..., <statics#n_statics>, <dynamic#1>, ... ]
    values_collection values;
    size_t n_statics;

    void
    tick(const timestamp_type &newts)
    {
      if (timestamp != newts)
      {
        timestamp = newts;
        values.resize(n_statics);
      }
    }

    template <typename ...Args> void
    put_dynamic(Args&& ...args)
    { values.emplace_back(std::forward<Args>(args)...); }

    template <typename ...Args> void
    put_static(Args&& ...args)
    {
      values.emplace(values.begin(), std::forward<Args>(args)...);
      n_statics += 1;
    }
  }; // (sub)class mw::utl::dynamic_grid::cell

  public:
  dynamic_grid(size_t nx, size_t ny)
  : m_nx {nx}, m_ny {ny}
  { m_cells.resize(nx * ny); }

  std::pair<size_t, size_t>
  get_dimentions() const noexcept
  { return {m_nx, m_ny}; }

  void
  tick()
  { ++m_current_time; }

  class cell_view {
    cell_view(const cell &mycell)
    : m_cell {mycell}
    { }

    public:
    using const_iterator = typename values_collection::const_iterator;
    using iterator = const_iterator;

    using const_reverse_iterator =
      typename values_collection::const_reverse_iterator;
    using reverse_iterator = const_reverse_iterator;

    iterator
    begin() const noexcept
    { return m_cell.values.begin(); }

    iterator
    end() const noexcept
    { return m_cell.values.end(); }

    reverse_iterator
    rbegin() const noexcept
    { return m_cell.values.rbegin(); }

    reverse_iterator
    rend() const noexcept
    { return m_cell.values.rend(); }

    private:
    const cell &m_cell;

    friend class dynamic_grid;
  };

  cell_view
  at(size_t ix, size_t iy) const
  { return cell_view {m_cells[_get_cell_index(ix, iy)]}; }

  template <typename ...Args> void
  put(size_t ix, size_t iy, Args&& ...args)
  {
    cell &c = m_cells[_get_cell_index(ix, iy)];
    c.tick(m_current_time);
    c.put_dynamic(std::forward<Args>(args)...);
  }

  template <typename ...Args> void
  put_static(size_t ix, size_t iy, Args&& ...args)
  {
    cell &c = m_cells[_get_cell_index(ix, iy)];
    c.tick(m_current_time);
    c.put_static(std::forward<Args>(args)...);
  }

  private:
  const values_collection&
  _get_values(size_t ix, size_t iy) const
  {
    cell &c = m_cells[_get_cell_index(ix, iy)];
    c.tick(m_current_time);
    return c.values;
  }

  size_t
  _get_cell_index(size_t ix, size_t iy) const
  {
    if (ix >= m_nx or iy >= m_ny)
    {
      throw exception {
        (boost::format("cell indices are out of bounds: ix=%d/%d, iy=%d/%d")
         % ix % m_nx % iy % m_ny
      ).str()}.in(__func__);
    }
    return ix + m_nx*iy;
  }

  private:
  const size_t m_nx, m_ny;
  mutable std::vector<cell> m_cells;
  timestamp_type m_current_time;
}; // class mw::utl::dynamic_grid

} // namespace mw::utl
} // namespace mw

#endif
