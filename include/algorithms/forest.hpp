#ifndef ALG_FOREST_HPP
#define ALG_FOREST_HPP

#include <cstddef>
#include <utility>
#include <vector>


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


namespace detail {

// Basic fields for element with non-void identifier.
template <typename Key>
struct element_base {
  element_base(const Key& parent)
  : parent {parent}
  { }

  template <typename ...Args>
  element_base(Args&& ...args)
  : parent {std::forward<Args>(args)...}
  { }

  Key parent;
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


template <typename T>
struct is_pair { static constexpr bool value = false; };

template <typename T, typename U>
struct is_pair<std::pair<T, U>> { static constexpr bool value = true; };

template <typename Key, typename Traits, typename Container>
class forest {
  static_assert(not std::is_void<Key>::value, "key can not be of void type");

  public:
  using key_type = Key;
  using value_type = element<Key, typename Traits::union_type>;
  using container_type = Container;

  explicit
  forest() = default;
  forest(const forest& other): m_els {other.m_els} { }
  forest(forest&& other): m_els {std::move(other.m_els)} { }


  void
  make_set(const key_type &key)
  {
    if constexpr (is_pair<typename container_type::value_type>::value)
      m_els.emplace(key, key);
    else
      m_els.emplace_back(key);
  }

  const key_type&
  parent(const key_type &id) const
  { return m_els.at(id).parent; }

  key_type
  find(const key_type &id);

  void
  join(const key_type &x, const key_type &y);

  // void
  // reserve(typename container_type::size_type size)
  // { m_els.reserve(size); }

  void
  resize(typename container_type::size_type size)
  {
    m_els.clear();
    m_els.reserve(size);
    for (typename container_type::size_type i = 0; i < size; ++i)
      make_set(i);
  }

  void
  clear()
  { m_els.clear(); }

  protected:
  container_type m_els;

  private:
  const key_type&
  _set_parent(const key_type &id, const key_type &p)
  { return m_els[id].parent = p; }

  template <typename Key_, typename Traits_, typename C>
  friend Key_
  _find(forest<Key_, Traits_, C>& f, const Key_ &id, find_method::naive);
  template <typename Key_, typename Traits_, typename C>
  friend Key_
  _find(forest<Key_, Traits_, C>& f, const Key_ &id, find_method::path_compression);
  template <typename Key_, typename Traits_, typename C>
  friend Key_
  _find(forest<Key_, Traits_, C>& f, const Key_ &id, find_method::path_halving);
  template <typename Key_, typename Traits_, typename C>
  friend Key_
  _find(forest<Key_, Traits_, C>& f, const Key_ &id, find_method::path_splitting);

  template <typename Key_, typename Traits_, typename C>
  friend void
  _join(forest<Key_, Traits_, C>& f, const Key_ &x, const Key_ &y, union_method::naive);
  template <typename Key_, typename Traits_, typename C>
  friend void
  _join(forest<Key_, Traits_, C>& f, const Key_ &x, const Key_ &y, union_method::by_rank);
  template <typename Key_, typename Traits_, typename C>
  friend void
  _join(forest<Key_, Traits_, C>& f, const Key_ &x, const Key_ &y, union_method::by_size);
};

template <typename Key, typename Traits, typename C>
Key
_find(forest<Key, Traits, C>& f, const Key &id, find_method::naive)
{
  Key tmp = id;
  while (tmp != f.parent(tmp))
    tmp = f.parent(tmp);
  return tmp;
}

template <typename Key, typename Traits, typename C>
Key
_find(forest<Key, Traits, C>& f, const Key &id, find_method::path_compression)
{ return f._set_parent(id, _find(f, id, find_method::naive { })); }

template <typename Key, typename Traits, typename C>
Key
_find(forest<Key, Traits, C>& f, const Key &id, find_method::path_halving)
{
  Key tmp = id;
  while (tmp != f.parent(tmp))
  {
    f._set_parent(tmp, f.parent(f.parent(tmp)));
    tmp = f.parent(tmp);
  }
  return tmp;
}

template <typename Key, typename Traits, typename C>
Key
_find(forest<Key, Traits, C>& f, const Key &id, find_method::path_splitting)
{
  Key idval = id;
  while (idval != f.parent(idval))
  {
    const Key tmp = f.parent(idval);
    f._set_parent(idval, f.parent(tmp));
    idval = tmp;
  }
  return id;
}

template <typename Key, typename Traits, typename C>
Key
forest<Key, Traits, C>::find(const Key &x)
{ return _find(*this, x, typename Traits::find_type { }); }


template <typename Key, typename Traits, typename C>
void
_join(forest<Key, Traits, C>& f, const Key &x, const Key &y, union_method::naive)
{
  typename Traits::find_type find_method;
  auto x_root = _find<Key, Traits>(f, x, find_method);
  auto y_root = _find<Key, Traits>(f, y, find_method);
  f.m_els[y_root].parent = x_root;
}

template <typename Key, typename Traits, typename C>
void
_join(forest<Key, Traits, C>& f, const Key &x, const Key &y, union_method::by_rank)
{
  typename Traits::find_type find_method;
  Key x_root = _find<Key, Traits>(f, x, find_method);
  Key y_root = _find<Key, Traits>(f, y, find_method);

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

template <typename Key, typename Traits, typename C>
void
_join(forest<Key, Traits, C>& f, const Key &x, const Key &y, union_method::by_size)
{
  typename Traits::find_type find_method;
  Key x_root = _find<Key, Traits>(f, x, find_method);
  Key y_root = _find<Key, Traits>(f, y, find_method);

  if (x_root == y_root)
    return;

  // x_root will be the new root, y_root is attached root
  //
  // swap roots according to size
  if (f.m_els[x_root].size < f.m_els[y_root].size)
    std::swap(x_root, y_root);

  // attach tree with smaller size to one with bigger size
  f.m_els[y_root].parent = x_root;
  f.m_els[x_root].size += f.m_els[y_root].size;
}

template <typename Key, typename Traits, typename C>
void
forest<Key, Traits, C>::join(const Key &x, const Key &y)
{ _join(*this, x, y, typename Traits::union_type { }); }

} // namespace mw::alg::detail


template <typename FindMethod, typename UnionMethod>
struct forest_traits {
  typedef FindMethod find_type;
  typedef UnionMethod union_type;

  template <typename Key>
  using element_type = detail::element<Key, union_type>;
};

template <typename Key = size_t,
          typename Traits =
              forest_traits<find_method::path_halving, union_method::by_size>,
          typename Container =
              std::vector<typename Traits::template element_type<Key>>>
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
