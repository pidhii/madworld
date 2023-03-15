#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>
#include <type_traits>
#include <optional>


#ifndef SCRIPTS_PATH
# define SCRIPTS_PATH ""
#endif

namespace mw {

typedef uint32_t color_t;

struct color_rgba {
  color_rgba(color_t color)
  : a {uint8_t((color & 0xFF000000) >> 22)},
    b {uint8_t((color & 0x00FF0000) >> 16)},
    g {uint8_t((color & 0x0000FF00) >>  8)},
    r {uint8_t((color & 0x000000FF) >>  0)}
  { }
  uint8_t a, b, g, r;
};

template <typename T>
struct remove_cvref {
  using type =
    typename std::remove_reference<typename std::remove_cv<T>::type>::type;
};

union qword {
  qword(): u64 {0} { }
  qword(int x): i {x} { }
  qword(unsigned x): u {x} { }
  qword(int64_t x): i64 {x} { }
  qword(uint64_t x): u64 {x} { }

  uint64_t u;
  uint64_t u64;

  int64_t i;
  int64_t i64;

  char c;

  bool
  operator == (qword other) const noexcept
  { return u64 == other.u64; }
};


template <typename T, class Owner>
class identifier {
  public:
  identifier() = default;
  identifier(const identifier&) = default;
  identifier(identifier&&) = default;

  identifier& operator = (const identifier&) noexcept = default;
  identifier& operator = (identifier&&) noexcept = default;

  operator bool () const noexcept { return m_val.has_value(); }

  private:
  identifier(const T &val): m_val {val} { }

  identifier& operator = (const T& val) noexcept { m_val = val; }
  identifier& operator = (T&& val) noexcept { m_val = std::move(val); }

  const T&
  get() const
  { return m_val.value(); }

  T&
  get()
  { return m_val.value(); }

  operator const T& () const
  { return get(); }

  operator T& ()
  { return get(); }

  private:
  std::optional<T> m_val;

  friend Owner;
};

template <typename T>
class identifier<T, void> {
  public:

  private:
  T m_value;
}; // class mw::identifier

} // namsepace mw

#endif
