/*
 * Copyright (c) 2004-2006  Daniel Elstner  <daniel.kitta@gmail.com>
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

#if SOMATO_VECTOR_USE_SSE

#include <glib.h>
#include <cmath>
#include <cstdlib>
#include <new>

#if (G_MEM_ALIGN < 16)
# define SOMATO_CUSTOM_ALLOC 1
#endif

namespace
{

static inline
__m128 vector4_dot(__m128 a, __m128 b)
{
  __m128 c = _mm_mul_ps(a, b);
  __m128 d = _mm_shuffle_ps(c, c, _MM_SHUFFLE(2,3,0,1));

  c = _mm_add_ps(c, d);
  d = _mm_movehl_ps(d, c);

  return _mm_add_ss(c, d);
}

static inline
__m128 vector3_mag(__m128 v)
{
  __m128 c = _mm_mul_ps(v, v);
  __m128 d = _mm_shuffle_ps(c, c, _MM_SHUFFLE(2,3,0,1));

  d = _mm_add_ss(d, c);
  c = _mm_movehl_ps(c, c);

  return _mm_sqrt_ss(_mm_add_ss(c, d));
}

#if SOMATO_CUSTOM_ALLOC

static void* v4_align_alloc(std::size_t size) throw() G_GNUC_MALLOC;

# if SOMATO_HAVE_POSIX_MEMALIGN

static
void* v4_align_alloc(std::size_t size) throw()
{
  if (size == 0)
    return std::malloc(1);

  // According to the language rules, using the sizeof operator on an
  // aggregate type always returns a multiple of the largest alignment
  // requirement of any aggregate member.  In other words, if the size
  // of an allocation request is not a multiple of 16, it cannot have
  // an alignment requirement of 16 bytes.  Checking for this catches
  // the majority of cases where the special alignment is unnecessary.

  if (size % sizeof(__m128) != 0)
    return std::malloc(size);

  void* p = 0;

  if (posix_memalign(&p, sizeof(__m128), size) == 0)
    return p;

  return 0;
}

static inline
void v4_align_free(void* p) throw()
{
  std::free(p);
}

# else /* !SOMATO_HAVE_POSIX_MEMALIGN */

static
void* v4_align_alloc(std::size_t size) throw()
{
  if (size == 0)
    size = 1;

  const std::size_t alignment = (size % sizeof(__m128) == 0) ? sizeof(__m128) : sizeof(__m64);

  return _mm_malloc(size, alignment);
}

static inline
void v4_align_free(void* p) throw()
{
  _mm_free(p);
}

# endif /* !SOMATO_HAVE_POSIX_MEMALIGN */
#endif /* SOMATO_CUSTOM_ALLOC */

} // anonymous namespace

/*
 * Replace the global new and delete operators to ensure that dynamically
 * allocated memory meets the SSE alignment requirements.  This is of course
 * a somewhat heavy-handed approach, but if not done globally, every class
 * or struct that has a data member of type Vector4/Matrix4/Quat would have
 * to provide its own memory allocation operators.  Worse yet, this applies
 * even to indirectly contained data members, and inheritance as a possible
 * solution does not apply there!
 *
 * Obviously this is all too easy to get wrong, thus I opted for the global
 * solution.  This implies that the current SSE vector implementation should
 * not be included in a library.
 *
 * Note that the GNU C library already uses a minimum alignment of 16 bytes
 * on 64-bit systems, thus the explicit alignment is actually not necessary
 * on these systems.  Unfortunately, there does not seem to be a reliable way
 * to detect the alignment used by malloc at compile time.  Detecting a glibc
 * system running on a 64-bit architecture at configure time is probably too
 * weak an indicator to justify relying on behavior outside the requirements
 * of the C standard.  On top of that, it would be perfectly valid for a C++
 * implementation to implement its own memory allocation scheme instead of
 * just wrapping the C library interface.
 */
#if SOMATO_CUSTOM_ALLOC

// MSVC++ is broken and wants to tell us about it.  No shame.
# ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4290)
# endif

void* operator new(std::size_t size) throw(std::bad_alloc)
{
  if (void *const p = v4_align_alloc(size))
    return p;
  else
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) throw(std::bad_alloc)
{
  if (void *const p = v4_align_alloc(size))
    return p;
  else
    throw std::bad_alloc();
}

# ifdef _MSC_VER
#  pragma warning(pop)
# endif

void operator delete(void* p) throw()
{
  v4_align_free(p);
}

void operator delete[](void* p) throw()
{
  v4_align_free(p);
}

void* operator new(std::size_t size, const std::nothrow_t&) throw()
{
  return v4_align_alloc(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) throw()
{
  return v4_align_alloc(size);
}

void operator delete(void* p, const std::nothrow_t&) throw()
{
  v4_align_free(p);
}

void operator delete[](void* p, const std::nothrow_t&) throw()
{
  v4_align_free(p);
}

#endif /* SOMATO_CUSTOM_ALLOC */

namespace Math
{

// static
__m128 Vector4::mag_(__m128 v)
{
  return _mm_sqrt_ss(vector4_dot(v, v));
}

// static
__m128 Vector4::dot_(__m128 a, __m128 b)
{
  return vector4_dot(a, b);
}

#if !SOMATO_VECTOR_USE_SSE2
// static
__m128 Vector4::rint_(__m128 v)
{
  __m128 u = _mm_movehl_ps(v, v);

  // Note: causes transition to MMX state
  const __m64 i0 = _mm_cvtps_pi32(v);
  const __m64 i1 = _mm_cvtps_pi32(u);

  v = _mm_cvtpi32_ps(v, i0);
  u = _mm_cvtpi32_ps(u, i1);

  _mm_empty(); // reset FP state

  return _mm_movelh_ps(v, u);
}
#endif /* !SOMATO_VECTOR_USE_SSE2 */

const Matrix4::array_type Matrix4::identity =
{
  { 1.0f, 0.0f, 0.0f, 0.0f },
  { 0.0f, 1.0f, 0.0f, 0.0f },
  { 0.0f, 0.0f, 1.0f, 0.0f },
  { 0.0f, 0.0f, 0.0f, 1.0f }
};

void Matrix4::assign(const Matrix4::column_type* b)
{
  m_[0] = b[0];
  m_[1] = b[1];
  m_[2] = b[2];
  m_[3] = b[3];
}

void Matrix4::assign(const Matrix4::value_type b[][4])
{
  m_[0] = _mm_loadu_ps(b[0]);
  m_[1] = _mm_loadu_ps(b[1]);
  m_[2] = _mm_loadu_ps(b[2]);
  m_[3] = _mm_loadu_ps(b[3]);
}

void Matrix4::transpose()
{
  const __m128 c0 = m_[0];
  const __m128 c1 = m_[1];
  const __m128 c2 = m_[2];
  const __m128 c3 = m_[3];

  const __m128 t0 = _mm_unpacklo_ps(c0, c1);
  const __m128 t1 = _mm_unpacklo_ps(c2, c3);
  const __m128 t2 = _mm_unpackhi_ps(c0, c1);
  const __m128 t3 = _mm_unpackhi_ps(c2, c3);

  m_[0] = _mm_movelh_ps(t0, t1);
  m_[1] = _mm_movehl_ps(t1, t0);
  m_[2] = _mm_movelh_ps(t2, t3);
  m_[3] = _mm_movehl_ps(t3, t2);
}

// static
__m128 Matrix4::mul_(const __m128* a, __m128 b)
{
  const __m128 c0 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(0,0,0,0)), a[0]);
  const __m128 c1 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(1,1,1,1)), a[1]);
  const __m128 c2 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(2,2,2,2)), a[2]);
  const __m128 c3 = _mm_mul_ps(_mm_shuffle_ps(b, b, _MM_SHUFFLE(3,3,3,3)), a[3]);

  return _mm_add_ps(_mm_add_ps(_mm_add_ps(c0, c1), c2), c3);
}

// static
void Matrix4::mul_(const __m128* a, const __m128* b, __m128* result)
{
  const __m128 a0 = a[0];
  const __m128 a1 = a[1];
  const __m128 a2 = a[2];
  const __m128 a3 = a[3];

  for (int i = 0; i < 4; ++i)
  {
    // It is assumed that b[] and result[] either refer to the same location
    // in memory or are completely distinct, i.e. not partially overlapping.

    const __m128 bi = b[i];

    const __m128 c0 = _mm_mul_ps(_mm_shuffle_ps(bi, bi, _MM_SHUFFLE(0,0,0,0)), a0);
    const __m128 c1 = _mm_mul_ps(_mm_shuffle_ps(bi, bi, _MM_SHUFFLE(1,1,1,1)), a1);
    const __m128 c2 = _mm_mul_ps(_mm_shuffle_ps(bi, bi, _MM_SHUFFLE(2,2,2,2)), a2);
    const __m128 c3 = _mm_mul_ps(_mm_shuffle_ps(bi, bi, _MM_SHUFFLE(3,3,3,3)), a3);

    result[i] = _mm_add_ps(_mm_add_ps(_mm_add_ps(c0, c1), c2), c3);
  }
}

// static
const Quat::CastVec4 Quat::mask_xyz_ = { { -1, -1, -1, 0 } };

// static
__m128 Quat::from_axis_(const Vector4& a, __m128 phi)
{
  const float phi_2 = _mm_cvtss_f32(_mm_mul_ss(phi, _mm_set_ss(0.5f)));
  float sine, cosine;

#if SOMATO_HAVE_SINCOSF
  sincosf(phi_2, &sine, &cosine);
#else
  sine   = std::sin(phi_2);
  cosine = std::cos(phi_2);
#endif

  // Normalize the axis vector first.
  __m128 u = a.data();
  const __m128 mag = vector3_mag(u);
  u = _mm_div_ps(u, _mm_shuffle_ps(mag, mag, _MM_SHUFFLE(0,0,0,0)));

  __m128 s = _mm_set_ss(sine);
  __m128 c = _mm_set_ss(cosine);

  s = _mm_shuffle_ps(s, s, _MM_SHUFFLE(1,0,0,0));
  c = _mm_shuffle_ps(c, c, _MM_SHUFFLE(0,1,1,1));

  return _mm_or_ps(_mm_and_ps(_mm_mul_ps(u, s), mask_xyz_.v), c);
}

// static
void Quat::to_matrix_(__m128 quat, __m128* result)
{
  const __m128 mask = mask_xyz_.v;

  const __m128 xyz = _mm_and_ps(quat, mask);
  const __m128 www = _mm_and_ps(_mm_shuffle_ps(quat, quat, _MM_SHUFFLE(3,3,3,3)), mask);
  const __m128 yzx = _mm_shuffle_ps(xyz, xyz, _MM_SHUFFLE(3,0,2,1));

  const __m128 xy_yz_xz = _mm_mul_ps(xyz, yzx);
  const __m128 wy_wz_wx = _mm_mul_ps(www, yzx);
  const __m128 yy_zz_xx = _mm_mul_ps(yzx, yzx);

  const __m128 xz_xy_yz = _mm_shuffle_ps(xy_yz_xz, xy_yz_xz, _MM_SHUFFLE(3,1,0,2));
  const __m128 zz_xx_yy = _mm_shuffle_ps(yy_zz_xx, yy_zz_xx, _MM_SHUFFLE(3,0,2,1));
  const __m128 wz_wx_wy = _mm_shuffle_ps(wy_wz_wx, wy_wz_wx, _MM_SHUFFLE(3,0,2,1));

  const __m128 t0 = _mm_add_ps(yy_zz_xx, zz_xx_yy);
  const __m128 t1 = _mm_sub_ps(xy_yz_xz, wz_wx_wy);
  const __m128 t2 = _mm_add_ps(xz_xy_yz, wy_wz_wx);

  static const __m128 v1110 = { 1.0f, 1.0f, 1.0f, 0.0f };

  const __m128 c0 = _mm_sub_ps(v1110, _mm_add_ps(t0, t0));
  const __m128 c1 = _mm_add_ps(t1, t1);
  const __m128 c2 = _mm_add_ps(t2, t2);

  const __m128 v0001 = Matrix4::identity[3];

  result[0] = _mm_move_ss(_mm_shuffle_ps(c2, c1, _MM_SHUFFLE(3,2,1,0)), c0);
  result[1] = _mm_move_ss(_mm_shuffle_ps(c0, c2, _MM_SHUFFLE(3,2,1,0)), c1);
  result[2] = _mm_move_ss(_mm_shuffle_ps(c1, c0, _MM_SHUFFLE(3,2,1,0)), c2);
  result[3] = v0001;
}

// static
float Quat::angle_(__m128 quat)
{
  const float sine   = _mm_cvtss_f32(vector3_mag(quat));
  const float cosine = _mm_cvtss_f32(_mm_shuffle_ps(quat, quat, _MM_SHUFFLE(3,3,3,3)));

  return 2.0f * std::atan2(sine, cosine);
}

// static
__m128 Quat::renormalize_(__m128 quat, __m128 epsilon)
{
  static const union { int i; float f; } absmasku = { 0x7FFFFFFF };
  const __m128 absmask = _mm_load_ss(&absmasku.f);

  const __m128 norm  = vector4_dot(quat, quat);
  const __m128 error = _mm_and_ps(_mm_sub_ss(_mm_set_ss(1.0f), norm), absmask);

  if (_mm_ucomige_ss(error, epsilon))
  {
    const __m128 mag = _mm_sqrt_ss(norm);

    quat = _mm_div_ps(quat, _mm_shuffle_ps(mag, mag, _MM_SHUFFLE(0,0,0,0)));
  }

  return quat;
}

// static
__m128 Quat::mul_(__m128 a, __m128 b)
{
  // x = aw * bx + ax * bw + ay * bz - az * by
  // y = aw * by + ay * bw + az * bx - ax * bz
  // z = aw * bz + az * bw + ax * by - ay * bx
  // w = aw * bw - ax * bx - ay * by - az * bz

  const __m128 a0 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3,3,3,3));
  const __m128 a1 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(0,2,1,0));
  const __m128 a2 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(1,0,2,1));
  const __m128 a3 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2,1,0,2));

  const __m128 b1 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(0,3,3,3));
  const __m128 b2 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(1,1,0,2));
  const __m128 b3 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(2,0,2,1));

  const __m128 c0 = _mm_mul_ps(a0, b);
  const __m128 c1 = _mm_mul_ps(a1, b1);
  const __m128 c2 = _mm_mul_ps(a2, b2);
  const __m128 c3 = _mm_mul_ps(a3, b3);

  // Just invert the sign of one intermediate sum in order to
  // compute w along with x, y, z using only vertical operations:
  //
  // w = aw * bw + (-(ax * bx + ay * by)) - az * bz
  //
  // result = ((c1 + c2) ^ signbit3) + (c0 - c3)

  static const __m128 signbit3 = { 0.0f, 0.0f, 0.0f, -0.0f };

  const __m128 c12 = _mm_add_ps(c1, c2);
  const __m128 c03 = _mm_sub_ps(c0, c3);

  return _mm_add_ps(_mm_xor_ps(c12, signbit3), c03);
}

} // namespace Math

#endif /* SOMATO_VECTOR_USE_SSE */
