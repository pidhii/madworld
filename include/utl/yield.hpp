#ifndef UTL_YIELD_HPP
#define UTL_YIELD_HPP

#include <list>
#include <vector>
#include <set>
#include <unordered_set>


namespace mw {
inline namespace utl {

template <typename Container>
class yield_via_emplace_back {
  public:
  yield_via_emplace_back(Container &container)
  : m_container {container}
  { }

  template <typename Arg> void
  operator () (Arg&& arg)
  { m_container.emplace_back(std::forward<Arg>(arg)); }

  private:
  Container &m_container;
}; // class mw::utl::yield_via_emplace_back

template <typename Container>
class yield_via_emplace {
  public:
  yield_via_emplace(Container &container)
  : m_container {container}
  { }

  template <typename Arg> void
  operator () (Arg&& arg)
  { m_container.emplace(std::forward<Arg>(arg)); }

  private:
  Container &m_container;
}; // class mw::utl::yield_via_emplace

template <typename T> yield_via_emplace_back<std::list<T>>
yield_into(std::list<T> &container)
{ return {container}; }

template <typename T> yield_via_emplace_back<std::vector<T>>
yield_into(std::vector<T> &container)
{ return {container}; }

template <typename T> yield_via_emplace<std::set<T>>
yield_into(std::set<T> &container)
{ return {container}; }

template <typename T> yield_via_emplace<std::unordered_set<T>>
yield_into(std::unordered_set<T> &container)
{ return {container}; }

} // namespace mw::utl
} // namespace mw

#endif
