#ifndef UTL_FREQUENCY_LIMITER_HPP
#define UTL_FREQUENCY_LIMITER_HPP

#include <chrono>


namespace mw {
inline namespace utl {

class frequency_limiter {
  using time_units = std::chrono::milliseconds;
  using clock = std::chrono::high_resolution_clock;
  using time_point = std::chrono::time_point<clock>;

  public:
  template <typename Rep, typename Period>
  frequency_limiter(const std::chrono::duration<Rep, Period> &mindt)
  : m_mindt {std::chrono::duration_cast<time_units>(mindt)},
    m_start {clock::now()}
  { }

  template <typename Rep, typename Period>
  bool
  operator () (std::chrono::duration<Rep, Period> &duration) noexcept
  {
    using duration_type = std::chrono::duration<Rep, Period>;

    const time_point now = clock::now();
    if (now - m_start >= m_mindt)
    {
      duration = std::chrono::duration_cast<duration_type>(now - m_start);
      m_start = now;
      return true;
    }
    else
    {
      duration = std::chrono::duration_cast<duration_type>(m_mindt - (now - m_start));
      return false;
    }
  }

  bool
  operator () () noexcept
  {
    std::chrono::milliseconds duration;
    return this->operator()(duration);
  }

  private:
  const time_units m_mindt;
  time_point m_start;
}; // class mw::utl::frequency_limiter

} // namespace mw::utl
} // namespace mw

#endif
