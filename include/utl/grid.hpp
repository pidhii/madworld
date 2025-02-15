#ifndef UTL_GRID_HPP
#define UTL_GRID_HPP

#include "geometry.hpp"
#include "exceptions.hpp"

#include "./_grid_details.hpp"


namespace mw {
inline namespace utl {

template <typename T>
struct grid;

template <typename T, typename Root>
class grid_base {
  public:
  static constexpr char class_name[] = "mw::utl::grid_base";
  using exception = scoped_exception<class_name>;

  using data_type = T;
  using value_type = data_type;

  protected:
  using cell_ptr_type = detail::_grid_cell_ptr<data_type>;
  using cell_type = detail::_grid_cell<data_type>;
  using root_type = Root;
  using view_type = grid_base<T, detail::_grid_cell_ptr<T>&>;

  public:
  grid_base(const rectangle &grid_box, const root_type &root)
  : m_box {grid_box}, m_root {root}
  { }

  grid_base(const grid_base &) = delete;
  grid_base& operator = (const grid_base &) = delete;

  const rectangle&
  get_box() const noexcept
  { return m_box; }

  const T&
  operator [] (const pt2d_d &p) const
  { return _get_bottom_cell(p).data(); }

  T&
  operator [] (const pt2d_d &p)
  { return _get_bottom_cell(p).data(); }

  template <typename ...Args>
  void
  refine(const pt2d_d &p, size_t nx, size_t ny, Args&& ...args)
  {
    _get_bottom_cell(p) =
      cell_type::make_grid_cell(nx, ny, std::forward<Args>(args)...);
  }

  bool
  is_leaf() const noexcept
  { return m_root->is_data(); }

  const T&
  get_value() const
  {
    if (is_leaf())
      return m_root->data();
    else
      throw exception {"get_value() on non-leaf node"}.in(__func__);
  }

  T&
  get_value_ref()
  {
    if (is_leaf())
      return m_root->data();
    else
      throw exception {"get_value_ref() on non-leaf node"}.in(__func__);
  }

  void
  set_value(const data_type &v)
  {
    if (is_leaf())
      m_root->data() = v;
    else
      throw exception {"set_value() on non-leaf node"}.in(__func__);
  }

  template <typename ...Args>
  void
  divide(size_t nx, size_t ny, Args&& ...args)
  {
    if (m_root->is_data())
      m_root = cell_type::make_grid_cell(nx, ny, std::forward<Args>(args)...);
    else
      throw exception {"divide() on non-leaf node"}.in(__func__);
  }

  template <typename Callback>
  void
  for_each(Callback cb) const
  {
    detail::_for_each(
      m_root, m_box.offset.x, m_box.offset.y, m_box.width, m_box.height,
      [&] (const cell_ptr_type &c, double x, double y, double w, double h) {
        cb(c->data(), rectangle {{x, y}, w, h});
      }
    );
  }

  template <typename Callback>
  void
  for_each(Callback cb)
  {
    detail::_for_each(
      m_root, m_box.offset.x, m_box.offset.y, m_box.width, m_box.height,
      [&] (cell_ptr_type &c, double x, double y, double w, double h) {
        cb(c->data(), rectangle {{x, y}, w, h});
      }
    );
  }

  template <typename Callback>
  void
  scan(Callback &cb) const
  {
    detail::_scan(
      m_root, m_box.offset.x, m_box.offset.y, m_box.width, m_box.height,
      [&] (cell_ptr_type &c, double x, double y, double w, double h) {
        view_type view {rectangle {{x, y}, w, h}, c};
        cb(view, rectangle {{x, y}, w, h});
      }
    );
  }

  template <typename Callback>
  void
  scan(Callback &cb)
  {
    detail::_scan(
      m_root, m_box.offset.x, m_box.offset.y, m_box.width, m_box.height,
      [&] (cell_ptr_type &c, double x, double y, double w, double h) {
        view_type view {rectangle {{x, y}, w, h}, c};
        cb(view);
      }
    );
  }


  template <typename Callback>
  void
  scan(const Callback &cb) const
  {
    detail::_scan(
      m_root, m_box.offset.x, m_box.offset.y, m_box.width, m_box.height,
      [&] (cell_ptr_type &c, double x, double y, double w, double h) {
        view_type view {rectangle {{x, y}, w, h}, c};
        cb(view, rectangle {{x, y}, w, h});
      }
    );
  }

  template <typename Callback>
  void
  scan(const Callback &cb)
  {
    detail::_scan(
      m_root, m_box.offset.x, m_box.offset.y, m_box.width, m_box.height,
      [&] (cell_ptr_type &c, double x, double y, double w, double h) {
        view_type view {rectangle {{x, y}, w, h}, c};
        cb(view);
      }
    );
  }

  template <typename U, typename F>
  grid<U>
  map(F cb) const;

  private:
  const cell_ptr_type&
  _get_bottom_cell(const pt2d_d &p) const
  {
    if (not m_box.contains(p))
      throw exception {"values outside grid"}.in(__func__);

    double w = m_box.width;
    double h = m_box.height;
    double x = p.x - m_box.offset.x;
    double y = p.y - m_box.offset.y;
    return _find_cell(m_root, w, h, x, y);
  }

  cell_ptr_type&
  _get_bottom_cell(const pt2d_d &p)
  {
    if (not m_box.contains(p))
      throw exception {"values outside grid"}.in(__func__);

    double w = m_box.width;
    double h = m_box.height;
    double x = p.x - m_box.offset.x;
    double y = p.y - m_box.offset.y;
    return _find_cell(m_root, w, h, x, y);
  }

  protected:
  rectangle m_box;
  root_type m_root;
};


template <typename T>
using grid_view = grid_base<T, detail::_grid_cell_ptr<T>&>;


template <typename T>
struct grid: grid_base<T, detail::_grid_cell_ptr<T>> {
  private:
  using base = grid_base<T, detail::_grid_cell_ptr<T>>;

  public:
  grid(const rectangle &box)
  : base::grid_base(box, base::cell_type::make_data_cell())
  { }

  grid(const rectangle &box, const detail::_grid_cell_ptr<T> cp)
  : base::grid_base(box, cp)
  { }
};

template <typename T, typename Root>
template <typename U, typename F>
grid<U>
grid_base<T, Root>::map(F cb) const
{
  detail::_grid_cell_ptr<U> newroot = detail::_map<U>(
      m_root, m_box.offset.x, m_box.offset.y, m_box.width, m_box.height,
      [&] (const T& v, double x, double y, double w, double h) {
        return cb(v, rectangle {{x, y}, w, h});
      }
  );
  return grid {m_box, newroot};
}

} // namespace mw::utl
} // namespace mw

#endif
