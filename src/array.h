/*
 * Copyright (c) 2004-2008  Daniel Elstner  <daniel.kitta@gmail.com>
 *
 * This file is part of Somato.
 *
 * Somato is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Somato is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Somato; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SOMATO_ARRAY_H_INCLUDED
#define SOMATO_ARRAY_H_INCLUDED

#include <cstddef>
#include <algorithm>
#include <iterator>

namespace Util
{

template <class T, std::size_t N>
class Array
{
private:
  T data_[N];

public:
  typedef T                                     value_type;
  typedef T&                                    reference;
  typedef const T&                              const_reference;
  typedef T*                                    iterator;
  typedef const T*                              const_iterator;
  typedef std::reverse_iterator<iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::size_t                           size_type;
  typedef std::ptrdiff_t                        difference_type;

  size_type size()  const { return N; }
  bool      empty() const { return (N == 0); }

  reference       operator[](size_type i)       { return data_[i]; }
  const_reference operator[](size_type i) const { return data_[i]; }

  reference       front()       { return data_[0]; }
  const_reference front() const { return data_[0]; }
  reference       back()        { return data_[N - 1]; }
  const_reference back()  const { return data_[N - 1]; }

  iterator       begin()       { return &data_[0]; }
  const_iterator begin() const { return &data_[0]; }
  iterator       end()         { return &data_[N]; }
  const_iterator end()   const { return &data_[N]; }

  reverse_iterator       rbegin()       { return reverse_iterator(&data_[N]); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(&data_[N]); }
  reverse_iterator       rend()         { return reverse_iterator(&data_[0]); }
  const_reverse_iterator rend()   const { return const_reverse_iterator(&data_[0]); }
};

template <class T>
class MemChunk
{
private:
  T* mbegin_;
  T* mend_;

  // noncopyable
  MemChunk(const MemChunk<T>&);
  MemChunk<T>& operator=(const MemChunk<T>&);

public:
  typedef T                                     value_type;
  typedef T&                                    reference;
  typedef const T&                              const_reference;
  typedef T*                                    iterator;
  typedef const T*                              const_iterator;
  typedef std::reverse_iterator<iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::size_t                           size_type;
  typedef std::ptrdiff_t                        difference_type;

  explicit MemChunk(size_type s)
    : mbegin_ (static_cast<T*>(operator new(sizeof(T) * s))), mend_ (mbegin_ + s) {}
  MemChunk() : mbegin_ (0), mend_ (0) {}
  ~MemChunk() { operator delete(mbegin_); }

  void swap(MemChunk<T>& b)
    { std::swap(mbegin_, b.mbegin_); std::swap(mend_, b.mend_); }

  size_type bytes() const { return size_type(mend_ - mbegin_) * sizeof(T); }
  size_type size()  const { return mend_ - mbegin_; }
  bool      empty() const { return (mbegin_ == mend_); }

  reference       operator[](size_type i)       { return mbegin_[i]; }
  const_reference operator[](size_type i) const { return mbegin_[i]; }

  reference       front()       { return mbegin_[0]; }
  const_reference front() const { return mbegin_[0]; }
  reference       back()        { return mend_[-1]; }
  const_reference back()  const { return mend_[-1]; }

  iterator       begin()       { return mbegin_; }
  const_iterator begin() const { return mbegin_; }
  iterator       end()         { return mend_; }
  const_iterator end()   const { return mend_; }

  reverse_iterator       rbegin()       { return reverse_iterator(mend_); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(mend_); }
  reverse_iterator       rend()         { return reverse_iterator(mbegin_); }
  const_reverse_iterator rend()   const { return const_reverse_iterator(mbegin_); }
};

template <class T> inline
void swap(MemChunk<T>& a, MemChunk<T>& b) { a.swap(b); }

template <class T>
class Delete
{
public:
  typedef T    argument_type;
  typedef void result_type;

  void operator()(T ptr) const { delete ptr; }
};

} // namespace Util

#endif /* SOMATO_ARRAY_H_INCLUDED */
