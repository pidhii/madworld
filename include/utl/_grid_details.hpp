#ifndef UTL__GRID_DETAILS_HPP
#define UTL__GRID_DETAILS_HPP

#include <memory>
#include <new>


namespace mw {
inline namespace utl {

namespace detail {

template <typename T>
class _grid_cell;

template <typename T>
using _grid_cell_ptr = std::shared_ptr<_grid_cell<T>>;

template <typename T>
class _grid_cell {
  private:
  enum class _tag_e { data, grid };
  struct _grid_type { size_t nx, ny; _grid_cell_ptr<T> *data; };

  public:
  _grid_cell(const _grid_cell &) = delete;
  _grid_cell(_grid_cell &&) = delete;
  _grid_cell& operator = (const _grid_cell &) = delete;
  _grid_cell& operator = (_grid_cell &&) = delete;

  _grid_cell() { }

  ~_grid_cell()
  {
    switch (m_tag)
    {
      case _tag_e::data:
        m_data.~T();
        break;

      case _tag_e::grid:
      {
        const size_t n = m_grid.nx * m_grid.ny;
        for (size_t i = 0; i < n; ++i)
          m_grid.data[i] = nullptr;
        delete[] m_grid.data;
        break;
      }
    }
  }

  bool
  is_data() const noexcept
  { return m_tag == _tag_e::data; }

  bool
  is_grid() const noexcept
  { return m_tag == _tag_e::grid; }

  const T&
  data() const noexcept
  { return m_data; }

  T&
  data() noexcept
  { return m_data; }

  size_t
  n_x() const noexcept
  { return m_grid.nx; }

  size_t
  n_y() const noexcept
  { return m_grid.ny; }

  const _grid_cell_ptr<T>&
  at(size_t i, size_t j) const noexcept
  { return m_grid.data[_ij_to_idx(i, j)]; }

  _grid_cell_ptr<T>&
  at(size_t i, size_t j) noexcept
  { return m_grid.data[_ij_to_idx(i, j)]; }

  private:
  size_t
  _ij_to_idx(size_t i, size_t j) const noexcept
  { return j + i*m_grid.ny; }

  public:
  template <typename ...Args>
  static _grid_cell_ptr<T>
  make_data_cell(Args&& ...args)
  {
    _grid_cell_ptr<T> cp = std::make_shared<_grid_cell>();
    cp->m_tag = _tag_e::data;

    new (&cp->m_data) T {std::forward<Args>(args)...};

    return cp;
  }

  template <typename ...Args>
  static _grid_cell_ptr<T>
  make_grid_cell(size_t nx, size_t ny, Args&& ...args)
  {
    _grid_cell_ptr<T> cp = std::make_shared<_grid_cell>();
    cp->m_tag = _tag_e::grid;

    cp->m_grid.nx = nx;
    cp->m_grid.ny = ny;
    const size_t n = nx*ny;
    cp->m_grid.data = new _grid_cell_ptr<T>[n];
    for (size_t i = 0; i < n; ++i)
      cp->m_grid.data[i] = make_data_cell(std::forward<Args>(args)...);

    return cp;
  }

  private:
  _tag_e m_tag;

  union { _grid_type m_grid; T m_data; };
}; // class mw::utl::grid

template <typename T>
_grid_cell_ptr<T>&
_zoom(_grid_cell_ptr<T> &cp, double &w, double &h, double &x, double &y)
{
  w = w / cp->n_x();
  h = h / cp->n_y();
  const int qx = x / w;
  const int qy = y / h;
  x = x - qx*w;
  y = y - qy*h;
  return cp->at(qx, qy);
}

template <typename T>
const _grid_cell_ptr<T>&
_zoom(const _grid_cell_ptr<T> &cp, double &w, double &h, double &x, double &y)
{
  w = w / cp->n_x();
  h = h / cp->n_y();
  const int qx = x / w;
  const int qy = y / h;
  x = x - qx*w;
  y = y - qy*h;
  return cp->at(qx, qy);
}

template <typename T>
_grid_cell_ptr<T>&
_find_cell(_grid_cell_ptr<T> &cp, double &w, double &h, double &x, double &y)
{
  if (cp->is_data())
    return cp;
  else
    return _find_cell(_zoom(cp, w, h, x, y), w, h, x, y);
}

template <typename T>
const _grid_cell_ptr<T>&
_find_cell(const _grid_cell_ptr<T> &cp, double &w, double &h, double &x, double &y)
{
  if (cp->is_data())
    return cp;
  else
    return _find_cell(_zoom(cp, w, h, x, y), w, h, x, y);
}

template <typename T, typename Callback>
void
_for_each(const _grid_cell_ptr<T> &cp, double x0, double y0, double w, double h,
    Callback cb)
{
  if (cp->is_data())
    cb(cp, x0, y0, w, h);
  else
  {
    const double dx = w / cp->n_x();
    const double dy = h / cp->n_y();
    for (size_t ix = 0; ix < cp->n_x(); ++ix)
    {
      for (size_t iy = 0; iy < cp->n_y(); ++iy)
        _for_each(cp->at(ix, iy), x0 + dx*ix, y0 + dy*iy, dx, dy, cb);
    }
  }
}

template <typename T, typename Callback>
void
_for_each(_grid_cell_ptr<T> &cp, double x0, double y0, double w, double h,
    Callback cb)
{
  if (cp->is_data())
    cb(cp, x0, y0, w, h);
  else
  {
    const double dx = w / cp->n_x();
    const double dy = h / cp->n_y();
    for (size_t ix = 0; ix < cp->n_x(); ++ix)
    {
      for (size_t iy = 0; iy < cp->n_y(); ++iy)
        _for_each(cp->at(ix, iy), x0 + dx*ix, y0 + dy*iy, dx, dy, cb);
    }
  }
}

template <typename T, typename Callback>
void
_scan(_grid_cell_ptr<T> &cp, double x0, double y0, double w, double h,
    Callback cb)
{
  if (cp->is_data())
  {
    cb(cp, x0, y0, w, h);
    if (cp->is_grid())
      _scan(cp, x0, y0, w, h, cb);
  }
  else
  {
    const double dx = w / cp->n_x();
    const double dy = h / cp->n_y();
    for (size_t ix = 0; ix < cp->n_x(); ++ix)
    {
      for (size_t iy = 0; iy < cp->n_y(); ++iy)
        _scan(cp->at(ix, iy), x0 + dx*ix, y0 + dy*iy, dx, dy, cb);
    }
  }
}

template <typename T, typename Callback>
void
_scan(const _grid_cell_ptr<T> &cp, double x0, double y0, double w, double h,
    Callback cb)
{
  if (cp->is_data())
  {
    cb(cp, x0, y0, w, h);
    if (cp->is_grid())
      _scan(cp, x0, y0, w, h, cb);
  }
  else
  {
    const double dx = w / cp->n_x();
    const double dy = h / cp->n_y();
    for (size_t ix = 0; ix < cp->n_x(); ++ix)
    {
      for (size_t iy = 0; iy < cp->n_y(); ++iy)
        _scan(cp->at(ix, iy), x0 + dx*ix, y0 + dy*iy, dx, dy, cb);
    }
  }
}

template <typename U, typename T, typename F>
_grid_cell_ptr<U>
_map(const _grid_cell_ptr<T> &cp, double x0, double y0, double w, double h,
    F cb)
{
  if (cp->is_data())
    return _grid_cell<U>::make_data_cell(cb(cp->data(), x0, y0, w, h));
  else
  {
    _grid_cell_ptr<U> newcp =
      _grid_cell<U>::make_grid_cell(cp->n_x(), cp->n_y());
    const double dx = w / cp->n_x();
    const double dy = h / cp->n_y();
    for (size_t ix = 0; ix < cp->n_x(); ++ix)
    {
      for (size_t iy = 0; iy < cp->n_y(); ++iy)
        newcp->at(ix, iy) =
          _map<U>(cp->at(ix, iy), x0 + dx*ix, y0 + dy*iy, dx, dy, cb);
    }
    return newcp;
  }
}

} // namespace mw::utl::detail
} // namespace mw::utl
} // namespace mw

#endif
