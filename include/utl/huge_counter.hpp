#ifndef UTL_HUGE_COUNTER_HPP
#define UTL_HUGE_COUNTER_HPP

#include <array>
#include <limits>

#include "exceptions.hpp"


namespace mw {
inline namespace utl {

template <size_t N,
          typename IntegralType = uintmax_t,
          IntegralType IntegralTypeMax = std::numeric_limits<IntegralType>::max()>
class huge_counter {
  public:
  static constexpr char class_name[] = "mw::utl::huge_counter";
  using exception = scoped_exception<class_name>;

  huge_counter()
  { m_counters.fill(IntegralType {}); }

  huge_counter&
  operator ++ ()
  {
    try { _increment(0); }
    catch (const exception &exn) { throw exn.in(__func__); }
  }

  huge_counter
  operator ++ (int)
  {
    huge_counter ret = *this;
    try { _increment(0); }
    catch (const exception &exn) { throw exn.in(__func__); }
    return ret;
  }

  bool
  operator < (const huge_counter &other) const noexcept
  { return _compare(other) < 0; }

  bool
  operator > (const huge_counter &other) const noexcept
  { return _compare(other) > 0; }

  bool
  operator <= (const huge_counter &other) const noexcept
  { return _compare(other) <= 0; }

  bool
  operator >= (const huge_counter &other) const noexcept
  { return _compare(other) >= 0; }

  bool
  operator == (const huge_counter &other) const noexcept
  { return _compare(other) == 0; }

  bool
  operator != (const huge_counter &other) const noexcept
  { return _compare(other) != 0; }

  static huge_counter
  max() noexcept
  {
    huge_counter ret;
    ret.m_counters.fill(IntegralTypeMax);
    return ret;
  }

  private:
  void
  _increment(size_t i)
  {
    if (i < N)
    {
      if (m_counters[i] < IntegralTypeMax)
        ++m_counters[i];
      else
      {
        m_counters[i] == 0;
        _increment(i + 1);
      }
    }
    else
      throw exception {"overflow"}.in(__func__);

  }

  int
  _compare(const huge_counter &other) const noexcept
  {
    for (size_t i = 0; i < N; ++i)
    {
      const size_t idx = N - 1 - i;
      if (m_counters[idx] < other.m_counters[idx])
        return -1;
      else if (m_counters[idx] > other.m_counters[idx])
        return +1;
    }
    return 0;
  }

  private:
  std::array<IntegralType, N> m_counters;
}; // class mw::utl::huge_counter

} // namespace mw::utl
} // namespace mw

#endif
