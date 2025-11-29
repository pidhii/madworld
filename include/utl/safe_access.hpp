#ifndef UTL_SAFE_ACCESS_HPP
#define UTL_SAFE_ACCESS_HPP


#include <memory>


namespace mw {
inline namespace utl {

template <typename T> class safe_access;
template <typename T> class const_safe_pointer;

template <typename T>
class safe_pointer {
  safe_pointer(T *ptr, std::shared_ptr<bool> mark)
  : m_ptr {ptr}, m_alive_mark {mark}
  { }

  public:
  safe_pointer(T *ptr)
  : m_ptr {ptr}, m_alive_mark {ptr->get_safe_pointer().m_alive_mark}
  { }

  safe_pointer() = default;
  safe_pointer(const safe_pointer &) = default;
  safe_pointer(safe_pointer &&) = default;
  safe_pointer& operator = (const safe_pointer &) = default;
  safe_pointer& operator = (safe_pointer &&) = default;

  operator bool () const noexcept { return m_alive_mark and *m_alive_mark; }
  T* get() const { return m_ptr; }
  T* operator -> () const { return get(); }
  T& operator * () const { return *get(); }

  private:
  T* m_ptr;
  std::shared_ptr<bool> m_alive_mark;

  friend safe_access<T>;
}; // struct mw::utl::safe_pointer


template <typename T>
class const_safe_pointer {
  const_safe_pointer(const T *ptr, std::shared_ptr<bool> mark)
  : m_ptr {ptr}, m_alive_mark {mark}
  { }

  public:
  const_safe_pointer(const safe_pointer<T> &safeptr)
  { *this = safeptr->get_safe_pointer(); }

  const_safe_pointer() = default;
  const_safe_pointer(const const_safe_pointer &) = default;
  const_safe_pointer(const_safe_pointer &&) = default;
  const_safe_pointer& operator = (const const_safe_pointer &) = default;
  const_safe_pointer& operator = (const_safe_pointer &&) = default;

  operator bool () const noexcept { return m_alive_mark and *m_alive_mark; }
  const T* get() const { return m_ptr; }
  const T* operator -> () const { return get(); }
  const T& operator * () const { return *get(); }

  private:
  const T* m_ptr;
  std::shared_ptr<bool> m_alive_mark;

  friend safe_access<T>;
}; // struct mw::utl::safe_pointer


template <typename T, typename U>
safe_pointer<T>
const_safe_pointer_cast(const const_safe_pointer<U> &safeptr)
{ return const_cast<T*>(safeptr.get())->get_safe_pointer(); }


template <typename T>
class safe_access {
  public:
  safe_access(): m_alive_mark {std::make_shared<bool>(true)} { }
  virtual ~safe_access() { *m_alive_mark = false; }

  const_safe_pointer<T>
  get_safe_pointer() const noexcept
  { return {static_cast<const T*>(this), m_alive_mark}; }

  safe_pointer<T>
  get_safe_pointer() noexcept
  { return {static_cast<T*>(this), m_alive_mark}; }

  private:
  std::shared_ptr<bool> m_alive_mark;
}; // class mw::utl::safe_access

} // namespace mw::utl
} // namespace mw


#endif
