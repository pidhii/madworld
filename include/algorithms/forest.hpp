#ifndef ALG_FOREST_HPP
#define ALG_FOREST_HPP

#include <cstddef>
#include <utility>


namespace std { template <class T, class Allocator> class vector; }

namespace mw
{
inline namespace alg
{

namespace find_method
{
  struct naive {};
  struct path_compression {};
  struct path_halving {};
  struct path_splitting {};
};

namespace union_method
{
  struct naive {};
  struct by_rank {};
  struct by_size {};
};

template <typename FindMethod, typename UnionMethod>
struct forest_traits {
  typedef FindMethod find_type;
  typedef UnionMethod union_type;
};


namespace detail {

// Basic fields for element with non-void identifier.
template <typename Key>
struct element_base {
  element_base(size_t id, const Key& key)
  : parent {id}, key {key}
  { }

  template <typename ...Args>
  element_base(size_t id, Args&& ...args)
  : parent {id}, key {std::forward<Args>(args)...}
  { }

  size_t parent;
  Key key;
};

// Specialization for elements without additional identifier.
template <>
struct element_base<void> {
  size_t parent;
  element_base(size_t id): parent {id} { }
};

template <typename>
struct element_union_part;
// Naive union doesn't need any additional fields.
template <>
struct element_union_part<union_method::naive> { };
// Union by rank or by size needs additional field.
template <>
struct element_union_part<union_method::by_rank> { size_t rank = 0; };
template <>
struct element_union_part<union_method::by_size> { size_t size = 1; };

template <typename Key, typename UnionMethod>
struct element: element_base<Key>, element_union_part<UnionMethod> {
  using element_base<Key>::element_base;
};

template <typename Key, typename Traits,
         template <typename...> typename Container>
class forest {
  public:
  using value_type = element<Key, typename Traits::union_type>;
  using container_type = Container<value_type>;

  explicit
  forest() = default;
  forest(const forest& other): m_els {other.m_els} { }
  forest(forest&& other): m_els {std::move(other.m_els)} { }

  template <typename ...Args>
  size_t
  make_set(Args&& ...args)
  {
    const size_t id = m_els.size();
    m_els.emplace_back(id, std::forward<Args>(args)...);
    return id;
  }

  size_t
  make_set()
  {
    const size_t id = m_els.size();
    m_els.emplace_back(id);
    return id;
  }

  size_t
  parent(size_t id) const
  { return m_els[id].parent; }

  size_t
  find(size_t id);

  void
  join(size_t x, size_t y);

  void
  reserve(typename container_type::size_type size)
  { m_els.reserve(size); }

  protected:
  Container<value_type> m_els;

  private:
  size_t
  _set_parent(size_t id, size_t p)
  { return m_els[id].parent = p; }

  template <typename Key_, typename Traits_, template <typename...> typename C>
  friend size_t
  _find(forest<Key_, Traits_, C>& f, size_t id, find_method::naive);
  template <typename Key_, typename Traits_, template <typename...> typename C>
  friend size_t
  _find(forest<Key_, Traits_, C>& f, size_t id, find_method::path_compression);
  template <typename Key_, typename Traits_, template <typename...> typename C>
  friend size_t
  _find(forest<Key_, Traits_, C>& f, size_t id, find_method::path_halving);
  template <typename Key_, typename Traits_, template <typename...> typename C>
  friend size_t
  _find(forest<Key_, Traits_, C>& f, size_t id, find_method::path_splitting);

  template <typename Key_, typename Traits_, template <typename...> typename C>
  friend void
  _join(forest<Key_, Traits_, C>& f, size_t x, size_t y, union_method::naive);
  template <typename Key_, typename Traits_, template <typename...> typename C>
  friend void
  _join(forest<Key_, Traits_, C>& f, size_t x, size_t y, union_method::by_rank);
  template <typename Key_, typename Traits_, template <typename...> typename C>
  friend void
  _join(forest<Key_, Traits_, C>& f, size_t x, size_t y, union_method::by_size);
};

template <typename Key, typename Traits, template <typename...> typename C>
size_t
_find(forest<Key, Traits, C>& f, size_t id, find_method::naive)
{
  while (id != f.parent(id))
    id = f.parent(id);
  return id;
}

template <typename Key, typename Traits, template <typename...> typename C>
size_t
_find(forest<Key, Traits, C>& f, size_t id, find_method::path_compression)
{ return f._set_parent(id, _find(f, id, find_method::naive { })); }

template <typename Key, typename Traits, template <typename...> typename C>
size_t
_find(forest<Key, Traits, C>& f, size_t id, find_method::path_halving)
{
  while (id != f.parent(id))
  {
    f._set_parent(id, f.parent(f.parent(id)));
    id = f.parent(id);
  }
  return id;
}

template <typename Key, typename Traits, template <typename...> typename C>
size_t
_find(forest<Key, Traits, C>& f, size_t id, find_method::path_splitting)
{
  size_t tmp;
  while (id != f.parent(id))
  {
    tmp = f.parent(id);
    f._set_parent(id, f.parent(tmp));
    id = tmp;
  }
  return id;
}

template <typename Key, typename Traits, template <typename...> typename C>
size_t
forest<Key, Traits, C>::find(size_t x)
{ return _find(*this, x, typename Traits::find_type { }); }


template <typename Key, typename Traits, template <typename...> typename C>
void
_join(forest<Key, Traits, C>& f, size_t x, size_t y, union_method::naive)
{
  typename Traits::find_type find_method;
  auto x_root = _find<Key, Traits>(f, x, find_method);
  auto y_root = _find<Key, Traits>(f, y, find_method);
  f.m_els[y_root].parent = x_root;
}

template <typename Key, typename Traits, template <typename...> typename C>
void
_join(forest<Key, Traits, C>& f, size_t x, size_t y, union_method::by_rank)
{
  typename Traits::find_type find_method;
  auto x_root = _find<Key, Traits>(f, x, find_method);
  auto y_root = _find<Key, Traits>(f, y, find_method);

  if (x_root == y_root)
    return;

  // if same ranks, then rank of resulting tree will be one larger
  if (f.m_els[x_root].rank == f.m_els[y_root].rank)
  {
    f.m_els[y_root].parent = x_root;
    f.m_els[x_root].rank ++;
    return;
  }

  // if ranks are different, then join tree with smaller rank
  // to the tree with larger rank.
  if (f.m_els[x_root].rank > f.m_els[y_root].rank)
    f.m_els[y_root].parent = x_root;
  else
    f.m_els[x_root].parent = y_root;

  return;
}

template <typename Key, typename Traits, template <typename...> typename C>
void
_join(forest<Key, Traits, C>& f, size_t x, size_t y, union_method::by_size)
{
  typename Traits::find_type find_method;
  auto x_root = _find<Key, Traits>(f, x, find_method);
  auto y_root = _find<Key, Traits>(f, y, find_method);

  if (x_root == y_root)
    return;

  // x_root will be the new root, y_root is attached root
  //
  // swap roots according to size
  if (f.m_els[x_root].size < f.m_els[y_root].size)
  {
    size_t tmp = x_root;
    x_root = y_root;
    y_root = tmp;
  }

  // attach tree with smaller size to one with bigger size
  f.m_els[y_root].parent = x_root;
  f.m_els[x_root].size += f.m_els[y_root].size;
}

template <typename Key, typename Traits, template <typename...> typename C>
void
forest<Key, Traits, C>::join(size_t x, size_t y)
{ _join(*this, x, y, typename Traits::union_type { }); }

} // namespace mw::alg::detail

template <
  typename Key = void,
  typename Traits = forest_traits<find_method::path_halving,
                                  union_method::by_size>,
  template <typename...> typename Container = std::vector>
struct forest: detail::forest<Key, Traits, Container> {
  using value_type =
    typename detail::forest<Key, Traits, Container>::value_type;
  using container_type =
    typename detail::forest<Key, Traits, Container>::container_type;

  using detail::forest<Key, Traits, Container>::forest;
};

} // inline namespace mw::alg
} // namespace mw

#endif
