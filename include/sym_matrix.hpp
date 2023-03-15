#ifndef SYM_MATRIX_HPP
#define SYM_MATRIX_HPP

#include <cstdlib>
#include <algorithm>
#include <stdexcept>

namespace utl {

template<typename T>
class sym_matrix {
  public:
  sym_matrix(size_t n)
  : m_dsize {(1 + n)*n / 2},
    m_data {new T [m_dsize]}
  {}

  ~sym_matrix()
  { delete [] m_data; }

  const T&
  operator () (size_t i, size_t j) const
  {
    if (i > j)
      std::swap(i, j);

    if (j >= m_n)
      throw std::out_of_range {"from utl::sym_matrix::operator() const"};

    // lenght of `i`th row = n - i
    // => offset of `i`th row = n + (n-1) + (n-2) + ... (n-(i-1))
    //                        = (n + n-i+1)*i / 2
    //                        = (2n - i + 1)*i / 2
    size_t rowoffs = (2*m_n - i + 1)*i / 2;
    size_t eltoffs = j - i;
    return *(m_data + rowoffs + eltoffs);
  }

  T&
  operator () (size_t i, size_t j)
  {
    if (i > j)
      std::swap(i, j);

    if (j >= m_n)
      throw std::out_of_range {"from utl::sym_matrix::operator()"};

    // lenght of `i`th row = n - i
    // => offset of `i`th row = n + (n-1) + (n-2) + ... (n-(i-1))
    //                        = (n + n-i+1)*i / 2
    //                        = (2n - i + 1)*i / 2
    size_t rowoffs = (2*m_n - i + 1)*i / 2;
    size_t eltoffs = j - i;
    return *(m_data + rowoffs + eltoffs);
  }

  void
  fill(const T& z)
  { std::fill(m_data, m_data+m_dsize, z); }

  size_t
  n_dim() const noexcept
  { return m_n; }

  private:
  size_t m_n;
  size_t m_dsize;
  T* m_data;

}; // class utl::sym_matrix<T>

} // namespace utl

#endif
