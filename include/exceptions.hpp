#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <stdexcept>
#include <typeinfo>


namespace mw {

struct exception: public std::exception {
  exception(const std::string &what): m_what {what} { }
  exception(const char *what): m_what {what} { }

  virtual const char*
  what() const noexcept override
  { return m_what.c_str(); }

  protected:
  std::string m_what;
}; // struct exception

template <const char* source>
struct scoped_exception: public exception {
  scoped_exception(const std::string &what): exception(what) { }

  const char*
  what() const noexcept override
  {
    if (not m_where.empty())
      m_big_what = "["+std::string(source)+"] in "+m_where+": "+m_what;
    else
      m_big_what = "["+std::string(source)+"] "+m_what;
    return m_big_what.c_str();
  }

  scoped_exception&
  in(const char *where) noexcept
  { m_where = where; return *this; }

  private:
  std::string m_where;
  mutable std::string m_big_what;
}; // struct scoped_exception

} // namespace mw

#endif
