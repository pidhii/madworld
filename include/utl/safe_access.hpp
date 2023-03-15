#ifndef UTL_SAFE_ACCESS_HPP
#define UTL_SAFE_ACCESS_HPP

#include "logging.h"

#include <memory>


namespace mw {
inline namespace utl {

template <typename T>
class safe_access;

template <typename T>
class safe_pointer {
  safe_pointer(const std::shared_ptr<T*> &ptrptr): m_ptrptr {ptrptr} { }

  public:
  safe_pointer() = default;
  safe_pointer(const safe_pointer &) = default;
  safe_pointer(safe_pointer &&) = default;
  safe_pointer& operator = (const safe_pointer &) = default;
  safe_pointer& operator = (safe_pointer &&) = default;

  operator bool () const noexcept { return m_ptrptr and *m_ptrptr != nullptr; }
  T* get() const { return *m_ptrptr.get(); }
  T* operator -> () const { return get(); }
  T& operator * () const { return *get(); }

  private:
  std::shared_ptr<T*> m_ptrptr;

  friend safe_access<T>;
}; // struct mw::utl::safe_pointer

template <typename T>
class safe_access {
  public:
  safe_access(T* ptr): m_ptrptr {std::make_shared<T*>(ptr)} { }
  virtual ~safe_access() { *m_ptrptr = nullptr; }

  safe_pointer<T>
  get_safe_pointer() const noexcept
  { return {m_ptrptr}; }

  private:
  std::shared_ptr<T*> m_ptrptr;
}; // class mw::utl::safe_access

} // namespace mw::utl
} // namespace mw


#endif
