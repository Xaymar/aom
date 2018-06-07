/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <emmintrin.h>  // SSE2

#include "config/aom_dsp_rtcd.h"

#include "aom_dsp/x86/synonyms.h"
#include "aom_ports/mem.h"
#include "aom_ports/emmintrin_compat.h"

static INLINE __m128i abs_diff(__m128i a, __m128i b) {
  return _mm_or_si128(_mm_subs_epu8(a, b), _mm_subs_epu8(b, a));
}

static INLINE void transpose4x8_8x4_low_sse2(__m128i *x0, __m128i *x1,
                                             __m128i *x2, __m128i *x3,
                                             __m128i *d0, __m128i *d1,
                                             __m128i *d2, __m128i *d3) {
  // input
  // x0   00 01 02 03 04 05 06 07 xx xx xx xx xx xx xx xx
  // x1   10 11 12 13 14 15 16 17 xx xx xx xx xx xx xx xx
  // x2   20 21 22 23 24 25 26 27 xx xx xx xx xx xx xx xx
  // x3   30 31 32 33 34 35 36 37 xx xx xx xx xx xx xx xx
  // output
  // 00 10 20 30 xx xx xx xx xx xx xx xx xx xx xx xx
  // 01 11 21 31 xx xx xx xx xx xx xx xx xx xx xx xx
  // 02 12 22 32 xx xx xx xx xx xx xx xx xx xx xx xx
  // 03 13 23 33 xx xx xx xx xx xx xx xx xx xx xx xx

  __m128i w0, w1;

  w0 = _mm_unpacklo_epi8(
      *x0, *x1);  // 00 10 01 11 02 12 03 13 04 14 05 15 06 16 07 17
  w1 = _mm_unpacklo_epi8(
      *x2, *x3);  // 20 30 21 31 22 32 23 33 24 34 25 35 26 36 27 37

  *d0 = _mm_unpacklo_epi16(
      w0, w1);  // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33

  *d1 = _mm_srli_si128(*d0,
                       4);  // 01 11 21 31 xx xx xx xx xx xx xx xx xx xx xx xx
  *d2 = _mm_srli_si128(*d0,
                       8);  // 02 12 22 32 xx xx xx xx xx xx xx xx xx xx xx xx
  *d3 = _mm_srli_si128(*d0,
                       12);  // 03 13 23 33 xx xx xx xx xx xx xx xx xx xx xx xx
}

static INLINE void transpose4x8_8x4_sse2(__m128i *x0, __m128i *x1, __m128i *x2,
                                         __m128i *x3, __m128i *d0, __m128i *d1,
                                         __m128i *d2, __m128i *d3, __m128i *d4,
                                         __m128i *d5, __m128i *d6,
                                         __m128i *d7) {
  // input
  // x0   00 01 02 03 04 05 06 07 xx xx xx xx xx xx xx xx
  // x1   10 11 12 13 14 15 16 17 xx xx xx xx xx xx xx xx
  // x2   20 21 22 23 24 25 26 27 xx xx xx xx xx xx xx xx
  // x3   30 31 32 33 34 35 36 37 xx xx xx xx xx xx xx xx
  // output
  // 00 10 20 30 xx xx xx xx xx xx xx xx xx xx xx xx
  // 01 11 21 31 xx xx xx xx xx xx xx xx xx xx xx xx
  // 02 12 22 32 xx xx xx xx xx xx xx xx xx xx xx xx
  // 03 13 23 33 xx xx xx xx xx xx xx xx xx xx xx xx
  // 04 14 24 34 xx xx xx xx xx xx xx xx xx xx xx xx
  // 05 15 25 35 xx xx xx xx xx xx xx xx xx xx xx xx
  // 06 16 26 36 xx xx xx xx xx xx xx xx xx xx xx xx
  // 07 17 27 37 xx xx xx xx xx xx xx xx xx xx xx xx

  __m128i w0, w1, ww0, ww1;

  w0 = _mm_unpacklo_epi8(
      *x0, *x1);  // 00 10 01 11 02 12 03 13 04 14 05 15 06 16 07 17
  w1 = _mm_unpacklo_epi8(
      *x2, *x3);  // 20 30 21 31 22 32 23 33 24 34 25 35 26 36 27 37

  ww0 = _mm_unpacklo_epi16(
      w0, w1);  // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
  ww1 = _mm_unpackhi_epi16(
      w0, w1);  // 04 14 24 34 05 15 25 35 06 16 26 36 07 17 27 37

  *d0 = ww0;  // 00 10 20 30 xx xx xx xx xx xx xx xx xx xx xx xx
  *d1 = _mm_srli_si128(ww0,
                       4);  // 01 11 21 31 xx xx xx xx xx xx xx xx xx xx xx xx
  *d2 = _mm_srli_si128(ww0,
                       8);  // 02 12 22 32 xx xx xx xx xx xx xx xx xx xx xx xx
  *d3 = _mm_srli_si128(ww0,
                       12);  // 03 13 23 33 xx xx xx xx xx xx xx xx xx xx xx xx

  *d4 = ww1;  // 04 14 24 34 xx xx xx xx xx xx xx xx xx xx xx xx
  *d5 = _mm_srli_si128(ww1,
                       4);  // 05 15 25 35 xx xx xx xx xx xx xx xx xx xx xx xx
  *d6 = _mm_srli_si128(ww1,
                       8);  // 06 16 26 36 xx xx xx xx xx xx xx xx xx xx xx xx
  *d7 = _mm_srli_si128(ww1,
                       12);  // 07 17 27 37 xx xx xx xx xx xx xx xx xx xx xx xx
}

static INLINE void transpose6x6_sse2(__m128i *x0, __m128i *x1, __m128i *x2,
                                     __m128i *x3, __m128i *x4, __m128i *x5,
                                     __m128i *d0d1, __m128i *d2d3,
                                     __m128i *d4d5) {
  __m128i w0, w1, w2, w4, w5;
  // x0  00 01 02 03 04 05 xx xx
  // x1  10 11 12 13 14 15 xx xx

  w0 = _mm_unpacklo_epi8(*x0, *x1);
  // 00 10 01 11 02 12 03 13  04 14 05 15 xx xx  xx xx

  // x2 20 21 22 23 24 25
  // x3 30 31 32 33 34 35

  w1 = _mm_unpacklo_epi8(
      *x2, *x3);  // 20 30 21 31 22 32 23 33  24 34 25 35 xx xx  xx xx

  // x4 40 41 42 43 44 45
  // x5 50 51 52 53 54 55

  w2 = _mm_unpacklo_epi8(*x4, *x5);  // 40 50 41 51 42 52 43 53 44 54 45 55

  w4 = _mm_unpacklo_epi16(
      w0, w1);  // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
  w5 = _mm_unpacklo_epi16(
      w2, w0);  // 40 50 xx xx 41 51 xx xx 42 52 xx xx 43 53 xx xx

  *d0d1 = _mm_unpacklo_epi32(
      w4, w5);  // 00 10 20 30 40 50 xx xx 01 11 21 31 41 51 xx xx

  *d2d3 = _mm_unpackhi_epi32(
      w4, w5);  // 02 12 22 32 42 52 xx xx 03 13 23 33 43 53 xx xx

  w4 = _mm_unpackhi_epi16(
      w0, w1);  // 04 14 24 34 05 15 25 35 xx xx xx xx xx xx xx xx
  w5 = _mm_unpackhi_epi16(
      w2, *x3);  // 44 54 xx xx 45 55 xx xx xx xx xx xx xx xx xx xx
  *d4d5 = _mm_unpacklo_epi32(
      w4, w5);  // 04 14 24 34 44 54 xx xx 05 15 25 35 45 55 xx xx
}

static INLINE void transpose8x8_low_sse2(__m128i *x0, __m128i *x1, __m128i *x2,
                                         __m128i *x3, __m128i *x4, __m128i *x5,
                                         __m128i *x6, __m128i *x7, __m128i *d0,
                                         __m128i *d1, __m128i *d2,
                                         __m128i *d3) {
  // input
  // x0 00 01 02 03 04 05 06 07
  // x1 10 11 12 13 14 15 16 17
  // x2 20 21 22 23 24 25 26 27
  // x3 30 31 32 33 34 35 36 37
  // x4 40 41 42 43 44 45 46 47
  // x5  50 51 52 53 54 55 56 57
  // x6  60 61 62 63 64 65 66 67
  // x7 70 71 72 73 74 75 76 77
  // output
  // d0 00 10 20 30 40 50 60 70 xx xx xx xx xx xx xx
  // d1 01 11 21 31 41 51 61 71 xx xx xx xx xx xx xx xx
  // d2 02 12 22 32 42 52 62 72 xx xx xx xx xx xx xx xx
  // d3 03 13 23 33 43 53 63 73 xx xx xx xx xx xx xx xx

  __m128i w0, w1, w2, w3, w4, w5;

  w0 = _mm_unpacklo_epi8(
      *x0, *x1);  // 00 10 01 11 02 12 03 13 04 14 05 15 06 16 07 17

  w1 = _mm_unpacklo_epi8(
      *x2, *x3);  // 20 30 21 31 22 32 23 33 24 34 25 35 26 36 27 37

  w2 = _mm_unpacklo_epi8(
      *x4, *x5);  // 40 50 41 51 42 52 43 53 44 54 45 55 46 56 47 57

  w3 = _mm_unpacklo_epi8(
      *x6, *x7);  // 60 70 61 71 62 72 63 73 64 74 65 75 66 76 67 77

  w4 = _mm_unpacklo_epi16(
      w0, w1);  // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
  w5 = _mm_unpacklo_epi16(
      w2, w3);  // 40 50 60 70 41 51 61 71 42 52 62 72 43 53 63 73

  *d0 = _mm_unpacklo_epi32(
      w4, w5);  // 00 10 20 30 40 50 60 70 01 11 21 31 41 51 61 71
  *d1 = _mm_srli_si128(*d0, 8);
  *d2 = _mm_unpackhi_epi32(
      w4, w5);  // 02 12 22 32 42 52 62 72 03 13 23 33 43 53 63 73
  *d3 = _mm_srli_si128(*d2, 8);
}

static INLINE void transpose8x8_sse2(__m128i *x0, __m128i *x1, __m128i *x2,
                                     __m128i *x3, __m128i *x4, __m128i *x5,
                                     __m128i *x6, __m128i *x7, __m128i *d0d1,
                                     __m128i *d2d3, __m128i *d4d5,
                                     __m128i *d6d7) {
  __m128i w0, w1, w2, w3, w4, w5, w6, w7;
  // x0 00 01 02 03 04 05 06 07
  // x1 10 11 12 13 14 15 16 17
  w0 = _mm_unpacklo_epi8(
      *x0, *x1);  // 00 10 01 11 02 12 03 13 04 14 05 15 06 16 07 17

  // x2 20 21 22 23 24 25 26 27
  // x3 30 31 32 33 34 35 36 37
  w1 = _mm_unpacklo_epi8(
      *x2, *x3);  // 20 30 21 31 22 32 23 33 24 34 25 35 26 36 27 37

  // x4 40 41 42 43 44 45 46 47
  // x5  50 51 52 53 54 55 56 57
  w2 = _mm_unpacklo_epi8(
      *x4, *x5);  // 40 50 41 51 42 52 43 53 44 54 45 55 46 56 47 57

  // x6  60 61 62 63 64 65 66 67
  // x7 70 71 72 73 74 75 76 77
  w3 = _mm_unpacklo_epi8(
      *x6, *x7);  // 60 70 61 71 62 72 63 73 64 74 65 75 66 76 67 77

  w4 = _mm_unpacklo_epi16(
      w0, w1);  // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
  w5 = _mm_unpacklo_epi16(
      w2, w3);  // 40 50 60 70 41 51 61 71 42 52 62 72 43 53 63 73

  *d0d1 = _mm_unpacklo_epi32(
      w4, w5);  // 00 10 20 30 40 50 60 70 01 11 21 31 41 51 61 71
  *d2d3 = _mm_unpackhi_epi32(
      w4, w5);  // 02 12 22 32 42 52 62 72 03 13 23 33 43 53 63 73

  w6 = _mm_unpackhi_epi16(
      w0, w1);  // 04 14 24 34 05 15 25 35 06 16 26 36 07 17 27 37
  w7 = _mm_unpackhi_epi16(
      w2, w3);  // 44 54 64 74 45 55 65 75 46 56 66 76 47 57 67 77

  *d4d5 = _mm_unpacklo_epi32(
      w6, w7);  // 04 14 24 34 44 54 64 74 05 15 25 35 45 55 65 75
  *d6d7 = _mm_unpackhi_epi32(
      w6, w7);  // 06 16 26 36 46 56 66 76 07 17 27 37 47 57 67 77
}

static INLINE void transpose16x8_8x16_sse2(
    __m128i *x0, __m128i *x1, __m128i *x2, __m128i *x3, __m128i *x4,
    __m128i *x5, __m128i *x6, __m128i *x7, __m128i *x8, __m128i *x9,
    __m128i *x10, __m128i *x11, __m128i *x12, __m128i *x13, __m128i *x14,
    __m128i *x15, __m128i *d0, __m128i *d1, __m128i *d2, __m128i *d3,
    __m128i *d4, __m128i *d5, __m128i *d6, __m128i *d7) {
  __m128i w0, w1, w2, w3, w4, w5, w6, w7, w8, w9;
  __m128i w10, w11, w12, w13, w14, w15;

  w0 = _mm_unpacklo_epi8(*x0, *x1);
  w1 = _mm_unpacklo_epi8(*x2, *x3);
  w2 = _mm_unpacklo_epi8(*x4, *x5);
  w3 = _mm_unpacklo_epi8(*x6, *x7);

  w8 = _mm_unpacklo_epi8(*x8, *x9);
  w9 = _mm_unpacklo_epi8(*x10, *x11);
  w10 = _mm_unpacklo_epi8(*x12, *x13);
  w11 = _mm_unpacklo_epi8(*x14, *x15);

  w4 = _mm_unpacklo_epi16(w0, w1);
  w5 = _mm_unpacklo_epi16(w2, w3);
  w12 = _mm_unpacklo_epi16(w8, w9);
  w13 = _mm_unpacklo_epi16(w10, w11);

  w6 = _mm_unpacklo_epi32(w4, w5);
  w7 = _mm_unpackhi_epi32(w4, w5);
  w14 = _mm_unpacklo_epi32(w12, w13);
  w15 = _mm_unpackhi_epi32(w12, w13);

  // Store first 4-line result
  *d0 = _mm_unpacklo_epi64(w6, w14);
  *d1 = _mm_unpackhi_epi64(w6, w14);
  *d2 = _mm_unpacklo_epi64(w7, w15);
  *d3 = _mm_unpackhi_epi64(w7, w15);

  w4 = _mm_unpackhi_epi16(w0, w1);
  w5 = _mm_unpackhi_epi16(w2, w3);
  w12 = _mm_unpackhi_epi16(w8, w9);
  w13 = _mm_unpackhi_epi16(w10, w11);

  w6 = _mm_unpacklo_epi32(w4, w5);
  w7 = _mm_unpackhi_epi32(w4, w5);
  w14 = _mm_unpacklo_epi32(w12, w13);
  w15 = _mm_unpackhi_epi32(w12, w13);

  // Store second 4-line result
  *d4 = _mm_unpacklo_epi64(w6, w14);
  *d5 = _mm_unpackhi_epi64(w6, w14);
  *d6 = _mm_unpacklo_epi64(w7, w15);
  *d7 = _mm_unpackhi_epi64(w7, w15);
}

static INLINE void transpose8x16_16x8_sse2(
    __m128i *x0, __m128i *x1, __m128i *x2, __m128i *x3, __m128i *x4,
    __m128i *x5, __m128i *x6, __m128i *x7, __m128i *d0d1, __m128i *d2d3,
    __m128i *d4d5, __m128i *d6d7, __m128i *d8d9, __m128i *d10d11,
    __m128i *d12d13, __m128i *d14d15) {
  __m128i w0, w1, w2, w3, w4, w5, w6, w7, w8, w9;
  __m128i w10, w11, w12, w13, w14, w15;

  w0 = _mm_unpacklo_epi8(*x0, *x1);
  w1 = _mm_unpacklo_epi8(*x2, *x3);
  w2 = _mm_unpacklo_epi8(*x4, *x5);
  w3 = _mm_unpacklo_epi8(*x6, *x7);

  w8 = _mm_unpackhi_epi8(*x0, *x1);
  w9 = _mm_unpackhi_epi8(*x2, *x3);
  w10 = _mm_unpackhi_epi8(*x4, *x5);
  w11 = _mm_unpackhi_epi8(*x6, *x7);

  w4 = _mm_unpacklo_epi16(w0, w1);
  w5 = _mm_unpacklo_epi16(w2, w3);
  w12 = _mm_unpacklo_epi16(w8, w9);
  w13 = _mm_unpacklo_epi16(w10, w11);

  w6 = _mm_unpacklo_epi32(w4, w5);
  w7 = _mm_unpackhi_epi32(w4, w5);
  w14 = _mm_unpacklo_epi32(w12, w13);
  w15 = _mm_unpackhi_epi32(w12, w13);

  // Store first 4-line result
  *d0d1 = _mm_unpacklo_epi64(w6, w14);
  *d2d3 = _mm_unpackhi_epi64(w6, w14);
  *d4d5 = _mm_unpacklo_epi64(w7, w15);
  *d6d7 = _mm_unpackhi_epi64(w7, w15);

  w4 = _mm_unpackhi_epi16(w0, w1);
  w5 = _mm_unpackhi_epi16(w2, w3);
  w12 = _mm_unpackhi_epi16(w8, w9);
  w13 = _mm_unpackhi_epi16(w10, w11);

  w6 = _mm_unpacklo_epi32(w4, w5);
  w7 = _mm_unpackhi_epi32(w4, w5);
  w14 = _mm_unpacklo_epi32(w12, w13);
  w15 = _mm_unpackhi_epi32(w12, w13);

  // Store second 4-line result
  *d8d9 = _mm_unpacklo_epi64(w6, w14);
  *d10d11 = _mm_unpackhi_epi64(w6, w14);
  *d12d13 = _mm_unpacklo_epi64(w7, w15);
  *d14d15 = _mm_unpackhi_epi64(w7, w15);
}

static AOM_FORCE_INLINE void filter4_sse2(__m128i *p1p0, __m128i *q1q0,
                                          __m128i *hev, __m128i *mask,
                                          __m128i *qs1qs0, __m128i *ps1ps0) {
  const __m128i t3t4 =
      _mm_set_epi8(3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4);
  const __m128i t80 = _mm_set1_epi8(0x80);
  __m128i filter, filter2filter1, work;
  __m128i ps1ps0_work, qs1qs0_work;
  __m128i hev1;
  const __m128i ff = _mm_cmpeq_epi8(t80, t80);

  ps1ps0_work = _mm_xor_si128(*p1p0, t80); /* ^ 0x80 */
  qs1qs0_work = _mm_xor_si128(*q1q0, t80);

  /* int8_t filter = signed_char_clamp(ps1 - qs1) & hev; */
  work = _mm_subs_epi8(ps1ps0_work, qs1qs0_work);
  filter = _mm_and_si128(_mm_srli_si128(work, 8), *hev);
  /* filter = signed_char_clamp(filter + 3 * (qs0 - ps0)) & mask; */
  filter = _mm_subs_epi8(filter, work);
  filter = _mm_subs_epi8(filter, work);
  filter = _mm_subs_epi8(filter, work);  /* + 3 * (qs0 - ps0) */
  filter = _mm_and_si128(filter, *mask); /* & mask */
  filter = _mm_unpacklo_epi64(filter, filter);

  /* filter1 = signed_char_clamp(filter + 4) >> 3; */
  /* filter2 = signed_char_clamp(filter + 3) >> 3; */
  filter2filter1 = _mm_adds_epi8(filter, t3t4); /* signed_char_clamp */
  filter = _mm_unpackhi_epi8(filter2filter1, filter2filter1);
  filter2filter1 = _mm_unpacklo_epi8(filter2filter1, filter2filter1);
  filter2filter1 = _mm_srai_epi16(filter2filter1, 11); /* >> 3 */
  filter = _mm_srai_epi16(filter, 11);                 /* >> 3 */
  filter2filter1 = _mm_packs_epi16(filter2filter1, filter);

  /* filter = ROUND_POWER_OF_TWO(filter1, 1) & ~hev; */
  filter = _mm_subs_epi8(filter2filter1, ff); /* + 1 */
  filter = _mm_unpacklo_epi8(filter, filter);
  filter = _mm_srai_epi16(filter, 9); /* round */
  filter = _mm_packs_epi16(filter, filter);
  filter = _mm_andnot_si128(*hev, filter);

  hev1 = _mm_unpackhi_epi64(filter2filter1, filter);
  filter2filter1 = _mm_unpacklo_epi64(filter2filter1, filter);

  /* signed_char_clamp(qs1 - filter), signed_char_clamp(qs0 - filter1) */
  qs1qs0_work = _mm_subs_epi8(qs1qs0_work, filter2filter1);
  /* signed_char_clamp(ps1 + filter), signed_char_clamp(ps0 + filter2) */
  ps1ps0_work = _mm_adds_epi8(ps1ps0_work, hev1);
  *qs1qs0 = _mm_xor_si128(qs1qs0_work, t80); /* ^ 0x80 */
  *ps1ps0 = _mm_xor_si128(ps1ps0_work, t80); /* ^ 0x80 */
}

static AOM_FORCE_INLINE void lpf_internal_4_sse2(
    __m128i *p1, __m128i *p0, __m128i *q0, __m128i *q1, __m128i *limit,
    __m128i *thresh, __m128i *q1q0_out, __m128i *p1p0_out) {
  __m128i q1p1, q0p0, p1p0, q1q0;
  __m128i abs_p0q0, abs_p1q1;
  __m128i mask, hev;
  const __m128i zero = _mm_setzero_si128();

  q1p1 = _mm_unpacklo_epi64(*p1, *q1);
  q0p0 = _mm_unpacklo_epi64(*p0, *q0);

  p1p0 = _mm_unpacklo_epi64(q0p0, q1p1);
  q1q0 = _mm_unpackhi_epi64(q0p0, q1p1);

  /* (abs(q1 - q0), abs(p1 - p0) */
  __m128i flat = abs_diff(q1p1, q0p0);
  /* abs(p1 - q1), abs(p0 - q0) */
  const __m128i abs_p1q1p0q0 = abs_diff(p1p0, q1q0);

  /* const uint8_t hev = hev_mask(thresh, *op1, *op0, *oq0, *oq1); */
  flat = _mm_max_epu8(flat, _mm_srli_si128(flat, 8));
  hev = _mm_unpacklo_epi8(flat, zero);

  hev = _mm_cmpgt_epi16(hev, *thresh);
  hev = _mm_packs_epi16(hev, hev);

  /* const int8_t mask = filter_mask2(*limit, *blimit, */
  /*                                  p1, p0, q0, q1); */
  abs_p0q0 = _mm_adds_epu8(abs_p1q1p0q0, abs_p1q1p0q0); /* abs(p0 - q0) * 2 */
  abs_p1q1 = _mm_unpackhi_epi8(abs_p1q1p0q0, abs_p1q1p0q0); /* abs(p1 - q1) */
  abs_p1q1 = _mm_srli_epi16(abs_p1q1, 9);
  abs_p1q1 = _mm_packs_epi16(abs_p1q1, abs_p1q1); /* abs(p1 - q1) / 2 */
  /* abs(p0 - q0) * 2 + abs(p1 - q1) / 2 */
  mask = _mm_adds_epu8(abs_p0q0, abs_p1q1);
  mask = _mm_unpacklo_epi64(mask, flat);
  mask = _mm_subs_epu8(mask, *limit);
  mask = _mm_cmpeq_epi8(mask, zero);
  mask = _mm_and_si128(mask, _mm_srli_si128(mask, 8));

  filter4_sse2(&p1p0, &q1q0, &hev, &mask, q1q0_out, p1p0_out);
}

void aom_lpf_horizontal_4_sse2(uint8_t *s, int p /* pitch */,
                               const uint8_t *_blimit, const uint8_t *_limit,
                               const uint8_t *_thresh) {
  const __m128i zero = _mm_setzero_si128();
  __m128i limit = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i *)_blimit),
                                     _mm_loadl_epi64((const __m128i *)_limit));
  __m128i thresh =
      _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *)_thresh), zero);

  __m128i qs1qs0, ps1ps0;
  __m128i p1, p0, q0, q1;

  p1 = _mm_cvtsi32_si128(*(int *)(s - 2 * p));
  p0 = _mm_cvtsi32_si128(*(int *)(s - 1 * p));
  q0 = _mm_cvtsi32_si128(*(int *)(s + 0 * p));
  q1 = _mm_cvtsi32_si128(*(int *)(s + 1 * p));

  lpf_internal_4_sse2(&p1, &p0, &q0, &q1, &limit, &thresh, &qs1qs0, &ps1ps0);

  xx_storel_32(s - 1 * p, ps1ps0);
  xx_storel_32(s - 2 * p, _mm_srli_si128(ps1ps0, 8));
  xx_storel_32(s + 0 * p, qs1qs0);
  xx_storel_32(s + 1 * p, _mm_srli_si128(qs1qs0, 8));
}

void aom_lpf_vertical_4_sse2(uint8_t *s, int p /* pitch */,
                             const uint8_t *_blimit, const uint8_t *_limit,
                             const uint8_t *_thresh) {
  __m128i p1p0, q1q0;
  __m128i p1, p0, q0, q1;

  const __m128i zero = _mm_setzero_si128();
  __m128i limit = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i *)_blimit),
                                     _mm_loadl_epi64((const __m128i *)_limit));
  __m128i thresh =
      _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *)_thresh), zero);

  __m128i x0, x1, x2, x3;
  __m128i d0, d1, d2, d3;
  x0 = _mm_loadl_epi64((__m128i *)(s - 2 + 0 * p));
  x1 = _mm_loadl_epi64((__m128i *)(s - 2 + 1 * p));
  x2 = _mm_loadl_epi64((__m128i *)(s - 2 + 2 * p));
  x3 = _mm_loadl_epi64((__m128i *)(s - 2 + 3 * p));

  transpose4x8_8x4_low_sse2(&x0, &x1, &x2, &x3, &p1, &p0, &q0, &q1);

  lpf_internal_4_sse2(&p1, &p0, &q0, &q1, &limit, &thresh, &q1q0, &p1p0);

  // Transpose 8x4 to 4x8
  p1 = _mm_srli_si128(p1p0, 8);
  q1 = _mm_srli_si128(q1q0, 8);

  transpose4x8_8x4_low_sse2(&p1, &p1p0, &q1q0, &q1, &d0, &d1, &d2, &d3);

  xx_storel_32(s + 0 * p - 2, d0);
  xx_storel_32(s + 1 * p - 2, d1);
  xx_storel_32(s + 2 * p - 2, d2);
  xx_storel_32(s + 3 * p - 2, d3);
}

static INLINE void store_buffer_horz_8(__m128i x, int p, int num, uint8_t *s) {
  xx_storel_32(s - (num + 1) * p, x);
  xx_storel_32(s + num * p, _mm_srli_si128(x, 8));
}

static AOM_FORCE_INLINE void lpf_internal_14_sse2(
    __m128i *q6p6, __m128i *q5p5, __m128i *q4p4, __m128i *q3p3, __m128i *q2p2,
    __m128i *q1p1, __m128i *q0p0, __m128i *blimit, __m128i *limit,
    __m128i *thresh) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i one = _mm_set1_epi8(1);
  __m128i mask, hev, flat, flat2;
  __m128i qs0ps0, qs1ps1;
  __m128i p1p0, q1q0, qs1qs0, ps1ps0;
  __m128i abs_p1p0;

  p1p0 = _mm_unpacklo_epi64(*q0p0, *q1p1);
  q1q0 = _mm_unpackhi_epi64(*q0p0, *q1p1);

  {
    __m128i abs_p1q1, abs_p0q0, abs_q1q0;
    __m128i fe, ff, work;
    abs_p1p0 = abs_diff(*q1p1, *q0p0);
    abs_q1q0 = _mm_srli_si128(abs_p1p0, 8);
    fe = _mm_set1_epi8(0xfe);
    ff = _mm_cmpeq_epi8(abs_p1p0, abs_p1p0);
    abs_p0q0 = abs_diff(p1p0, q1q0);
    abs_p1q1 = _mm_srli_si128(abs_p0q0, 8);
    abs_p0q0 = _mm_unpacklo_epi64(abs_p0q0, zero);

    flat = _mm_max_epu8(abs_p1p0, abs_q1q0);
    hev = _mm_subs_epu8(flat, *thresh);
    hev = _mm_xor_si128(_mm_cmpeq_epi8(hev, zero), ff);
    // replicate for the further "merged variables" usage
    hev = _mm_unpacklo_epi64(hev, hev);

    abs_p0q0 = _mm_adds_epu8(abs_p0q0, abs_p0q0);
    abs_p1q1 = _mm_srli_epi16(_mm_and_si128(abs_p1q1, fe), 1);
    mask = _mm_subs_epu8(_mm_adds_epu8(abs_p0q0, abs_p1q1), *blimit);
    mask = _mm_xor_si128(_mm_cmpeq_epi8(mask, zero), ff);
    // mask |= (abs(p0 - q0) * 2 + abs(p1 - q1) / 2  > blimit) * -1;
    mask = _mm_max_epu8(abs_p1p0, mask);
    // mask |= (abs(p1 - p0) > limit) * -1;
    // mask |= (abs(q1 - q0) > limit) * -1;

    work = _mm_max_epu8(abs_diff(*q2p2, *q1p1), abs_diff(*q3p3, *q2p2));
    mask = _mm_max_epu8(work, mask);
    mask = _mm_max_epu8(mask, _mm_srli_si128(mask, 8));
    mask = _mm_subs_epu8(mask, *limit);
    mask = _mm_cmpeq_epi8(mask, zero);
    // replicate for the further "merged variables" usage
    mask = _mm_unpacklo_epi64(mask, mask);
  }

  // lp filter - the same for 6, 8 and 14 versions
  filter4_sse2(&p1p0, &q1q0, &hev, &mask, &qs1qs0, &ps1ps0);
  qs0ps0 = _mm_unpacklo_epi64(ps1ps0, qs1qs0);
  qs1ps1 = _mm_unpackhi_epi64(ps1ps0, qs1qs0);
  // loopfilter done

  __m128i flat2_q5p5, flat2_q4p4, flat2_q3p3, flat2_q2p2;
  __m128i flat2_q1p1, flat2_q0p0, flat_q2p2, flat_q1p1, flat_q0p0;
  {
    __m128i work;
    flat = _mm_max_epu8(abs_diff(*q2p2, *q0p0), abs_diff(*q3p3, *q0p0));
    flat = _mm_max_epu8(abs_p1p0, flat);
    flat = _mm_max_epu8(flat, _mm_srli_si128(flat, 8));
    flat = _mm_subs_epu8(flat, one);
    flat = _mm_cmpeq_epi8(flat, zero);
    flat = _mm_and_si128(flat, mask);

    flat2 = _mm_max_epu8(abs_diff(*q4p4, *q0p0), abs_diff(*q5p5, *q0p0));
    work = abs_diff(*q6p6, *q0p0);
    flat2 = _mm_max_epu8(work, flat2);
    flat2 = _mm_max_epu8(flat2, _mm_srli_si128(flat2, 8));
    flat2 = _mm_subs_epu8(flat2, one);
    flat2 = _mm_cmpeq_epi8(flat2, zero);
    flat2 = _mm_and_si128(flat2, flat);  // flat2 & flat & mask
  }
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // flat and wide flat calculations
  {
    const __m128i eight = _mm_set1_epi16(8);
    const __m128i four = _mm_set1_epi16(4);
    __m128i p6_16, p5_16, p4_16, p3_16, p2_16, p1_16, p0_16;
    __m128i q6_16, q5_16, q4_16, q3_16, q2_16, q1_16, q0_16;
    __m128i pixelFilter_p, pixelFilter_q;
    __m128i pixetFilter_p2p1p0, pixetFilter_q2q1q0;
    __m128i sum_p6, sum_q6;
    __m128i sum_p3, sum_q3, res_p, res_q;

    p6_16 = _mm_unpacklo_epi8(*q6p6, zero);
    p5_16 = _mm_unpacklo_epi8(*q5p5, zero);
    p4_16 = _mm_unpacklo_epi8(*q4p4, zero);
    p3_16 = _mm_unpacklo_epi8(*q3p3, zero);
    p2_16 = _mm_unpacklo_epi8(*q2p2, zero);
    p1_16 = _mm_unpacklo_epi8(*q1p1, zero);
    p0_16 = _mm_unpacklo_epi8(*q0p0, zero);
    q0_16 = _mm_unpackhi_epi8(*q0p0, zero);
    q1_16 = _mm_unpackhi_epi8(*q1p1, zero);
    q2_16 = _mm_unpackhi_epi8(*q2p2, zero);
    q3_16 = _mm_unpackhi_epi8(*q3p3, zero);
    q4_16 = _mm_unpackhi_epi8(*q4p4, zero);
    q5_16 = _mm_unpackhi_epi8(*q5p5, zero);
    q6_16 = _mm_unpackhi_epi8(*q6p6, zero);
    pixelFilter_p = _mm_add_epi16(p5_16, _mm_add_epi16(p4_16, p3_16));
    pixelFilter_q = _mm_add_epi16(q5_16, _mm_add_epi16(q4_16, q3_16));

    pixetFilter_p2p1p0 = _mm_add_epi16(p0_16, _mm_add_epi16(p2_16, p1_16));
    pixelFilter_p = _mm_add_epi16(pixelFilter_p, pixetFilter_p2p1p0);

    pixetFilter_q2q1q0 = _mm_add_epi16(q0_16, _mm_add_epi16(q2_16, q1_16));
    pixelFilter_q = _mm_add_epi16(pixelFilter_q, pixetFilter_q2q1q0);
    pixelFilter_p =
        _mm_add_epi16(eight, _mm_add_epi16(pixelFilter_p, pixelFilter_q));
    pixetFilter_p2p1p0 = _mm_add_epi16(
        four, _mm_add_epi16(pixetFilter_p2p1p0, pixetFilter_q2q1q0));
    res_p = _mm_srli_epi16(
        _mm_add_epi16(pixelFilter_p,
                      _mm_add_epi16(_mm_add_epi16(p6_16, p0_16),
                                    _mm_add_epi16(p1_16, q0_16))),
        4);
    res_q = _mm_srli_epi16(
        _mm_add_epi16(pixelFilter_p,
                      _mm_add_epi16(_mm_add_epi16(q6_16, q0_16),
                                    _mm_add_epi16(p0_16, q1_16))),
        4);
    flat2_q0p0 = _mm_packus_epi16(res_p, res_q);

    res_p = _mm_srli_epi16(
        _mm_add_epi16(pixetFilter_p2p1p0, _mm_add_epi16(p3_16, p0_16)), 3);
    res_q = _mm_srli_epi16(
        _mm_add_epi16(pixetFilter_p2p1p0, _mm_add_epi16(q3_16, q0_16)), 3);

    flat_q0p0 = _mm_packus_epi16(res_p, res_q);

    sum_p6 = _mm_add_epi16(p6_16, p6_16);
    sum_q6 = _mm_add_epi16(q6_16, q6_16);
    sum_p3 = _mm_add_epi16(p3_16, p3_16);
    sum_q3 = _mm_add_epi16(q3_16, q3_16);

    pixelFilter_q = _mm_sub_epi16(pixelFilter_p, p5_16);
    pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q5_16);

    res_p = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_p,
            _mm_add_epi16(sum_p6,
                          _mm_add_epi16(p1_16, _mm_add_epi16(p2_16, p0_16)))),
        4);
    res_q = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_q,
            _mm_add_epi16(sum_q6,
                          _mm_add_epi16(q1_16, _mm_add_epi16(q0_16, q2_16)))),
        4);
    flat2_q1p1 = _mm_packus_epi16(res_p, res_q);

    pixetFilter_q2q1q0 = _mm_sub_epi16(pixetFilter_p2p1p0, p2_16);
    pixetFilter_p2p1p0 = _mm_sub_epi16(pixetFilter_p2p1p0, q2_16);
    res_p = _mm_srli_epi16(
        _mm_add_epi16(pixetFilter_p2p1p0, _mm_add_epi16(sum_p3, p1_16)), 3);
    res_q = _mm_srli_epi16(
        _mm_add_epi16(pixetFilter_q2q1q0, _mm_add_epi16(sum_q3, q1_16)), 3);
    flat_q1p1 = _mm_packus_epi16(res_p, res_q);

    sum_p6 = _mm_add_epi16(sum_p6, p6_16);
    sum_q6 = _mm_add_epi16(sum_q6, q6_16);
    sum_p3 = _mm_add_epi16(sum_p3, p3_16);
    sum_q3 = _mm_add_epi16(sum_q3, q3_16);

    pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q4_16);
    pixelFilter_q = _mm_sub_epi16(pixelFilter_q, p4_16);

    res_p = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_p,
            _mm_add_epi16(sum_p6,
                          _mm_add_epi16(p2_16, _mm_add_epi16(p3_16, p1_16)))),
        4);
    res_q = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_q,
            _mm_add_epi16(sum_q6,
                          _mm_add_epi16(q2_16, _mm_add_epi16(q1_16, q3_16)))),
        4);
    flat2_q2p2 = _mm_packus_epi16(res_p, res_q);

    pixetFilter_p2p1p0 = _mm_sub_epi16(pixetFilter_p2p1p0, q1_16);
    pixetFilter_q2q1q0 = _mm_sub_epi16(pixetFilter_q2q1q0, p1_16);

    res_p = _mm_srli_epi16(
        _mm_add_epi16(pixetFilter_p2p1p0, _mm_add_epi16(sum_p3, p2_16)), 3);
    res_q = _mm_srli_epi16(
        _mm_add_epi16(pixetFilter_q2q1q0, _mm_add_epi16(sum_q3, q2_16)), 3);
    flat_q2p2 = _mm_packus_epi16(res_p, res_q);

    sum_p6 = _mm_add_epi16(sum_p6, p6_16);
    sum_q6 = _mm_add_epi16(sum_q6, q6_16);

    pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q3_16);
    pixelFilter_q = _mm_sub_epi16(pixelFilter_q, p3_16);

    res_p = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_p,
            _mm_add_epi16(sum_p6,
                          _mm_add_epi16(p3_16, _mm_add_epi16(p4_16, p2_16)))),
        4);
    res_q = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_q,
            _mm_add_epi16(sum_q6,
                          _mm_add_epi16(q3_16, _mm_add_epi16(q2_16, q4_16)))),
        4);
    flat2_q3p3 = _mm_packus_epi16(res_p, res_q);

    sum_p6 = _mm_add_epi16(sum_p6, p6_16);
    sum_q6 = _mm_add_epi16(sum_q6, q6_16);

    pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q2_16);
    pixelFilter_q = _mm_sub_epi16(pixelFilter_q, p2_16);

    res_p = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_p,
            _mm_add_epi16(sum_p6,
                          _mm_add_epi16(p4_16, _mm_add_epi16(p5_16, p3_16)))),
        4);
    res_q = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_q,
            _mm_add_epi16(sum_q6,
                          _mm_add_epi16(q4_16, _mm_add_epi16(q3_16, q5_16)))),
        4);
    flat2_q4p4 = _mm_packus_epi16(res_p, res_q);

    sum_p6 = _mm_add_epi16(sum_p6, p6_16);
    sum_q6 = _mm_add_epi16(sum_q6, q6_16);
    pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q1_16);
    pixelFilter_q = _mm_sub_epi16(pixelFilter_q, p1_16);

    res_p = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_p,
            _mm_add_epi16(sum_p6,
                          _mm_add_epi16(p5_16, _mm_add_epi16(p6_16, p4_16)))),
        4);
    res_q = _mm_srli_epi16(
        _mm_add_epi16(
            pixelFilter_q,
            _mm_add_epi16(sum_q6,
                          _mm_add_epi16(q5_16, _mm_add_epi16(q6_16, q4_16)))),
        4);
    flat2_q5p5 = _mm_packus_epi16(res_p, res_q);
  }
  // wide flat
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  flat = _mm_shuffle_epi32(flat, 68);
  flat2 = _mm_shuffle_epi32(flat2, 68);

  *q2p2 = _mm_andnot_si128(flat, *q2p2);
  flat_q2p2 = _mm_and_si128(flat, flat_q2p2);
  *q2p2 = _mm_or_si128(*q2p2, flat_q2p2);

  qs1ps1 = _mm_andnot_si128(flat, qs1ps1);
  flat_q1p1 = _mm_and_si128(flat, flat_q1p1);
  *q1p1 = _mm_or_si128(qs1ps1, flat_q1p1);

  qs0ps0 = _mm_andnot_si128(flat, qs0ps0);
  flat_q0p0 = _mm_and_si128(flat, flat_q0p0);
  *q0p0 = _mm_or_si128(qs0ps0, flat_q0p0);

  *q5p5 = _mm_andnot_si128(flat2, *q5p5);
  flat2_q5p5 = _mm_and_si128(flat2, flat2_q5p5);
  *q5p5 = _mm_or_si128(*q5p5, flat2_q5p5);

  *q4p4 = _mm_andnot_si128(flat2, *q4p4);
  flat2_q4p4 = _mm_and_si128(flat2, flat2_q4p4);
  *q4p4 = _mm_or_si128(*q4p4, flat2_q4p4);

  *q3p3 = _mm_andnot_si128(flat2, *q3p3);
  flat2_q3p3 = _mm_and_si128(flat2, flat2_q3p3);
  *q3p3 = _mm_or_si128(*q3p3, flat2_q3p3);

  *q2p2 = _mm_andnot_si128(flat2, *q2p2);
  flat2_q2p2 = _mm_and_si128(flat2, flat2_q2p2);
  *q2p2 = _mm_or_si128(*q2p2, flat2_q2p2);

  *q1p1 = _mm_andnot_si128(flat2, *q1p1);
  flat2_q1p1 = _mm_and_si128(flat2, flat2_q1p1);
  *q1p1 = _mm_or_si128(*q1p1, flat2_q1p1);

  *q0p0 = _mm_andnot_si128(flat2, *q0p0);
  flat2_q0p0 = _mm_and_si128(flat2, flat2_q0p0);
  *q0p0 = _mm_or_si128(*q0p0, flat2_q0p0);
}

void aom_lpf_horizontal_14_sse2(unsigned char *s, int p,
                                const unsigned char *_blimit,
                                const unsigned char *_limit,
                                const unsigned char *_thresh) {
  __m128i q6p6, q5p5, q4p4, q3p3, q2p2, q1p1, q0p0;
  __m128i blimit = _mm_load_si128((const __m128i *)_blimit);
  __m128i limit = _mm_load_si128((const __m128i *)_limit);
  __m128i thresh = _mm_load_si128((const __m128i *)_thresh);

  q4p4 = _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(int *)(s - 5 * p)),
                            _mm_cvtsi32_si128(*(int *)(s + 4 * p)));
  q3p3 = _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(int *)(s - 4 * p)),
                            _mm_cvtsi32_si128(*(int *)(s + 3 * p)));
  q2p2 = _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(int *)(s - 3 * p)),
                            _mm_cvtsi32_si128(*(int *)(s + 2 * p)));
  q1p1 = _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(int *)(s - 2 * p)),
                            _mm_cvtsi32_si128(*(int *)(s + 1 * p)));

  q0p0 = _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(int *)(s - 1 * p)),
                            _mm_cvtsi32_si128(*(int *)(s - 0 * p)));

  q5p5 = _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(int *)(s - 6 * p)),
                            _mm_cvtsi32_si128(*(int *)(s + 5 * p)));

  q6p6 = _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(int *)(s - 7 * p)),
                            _mm_cvtsi32_si128(*(int *)(s + 6 * p)));

  lpf_internal_14_sse2(&q6p6, &q5p5, &q4p4, &q3p3, &q2p2, &q1p1, &q0p0, &blimit,
                       &limit, &thresh);

  store_buffer_horz_8(q0p0, p, 0, s);
  store_buffer_horz_8(q1p1, p, 1, s);
  store_buffer_horz_8(q2p2, p, 2, s);
  store_buffer_horz_8(q3p3, p, 3, s);
  store_buffer_horz_8(q4p4, p, 4, s);
  store_buffer_horz_8(q5p5, p, 5, s);
}

static AOM_FORCE_INLINE void lpf_internal_6_sse2(
    __m128i *p2, __m128i *q2, __m128i *p1, __m128i *q1, __m128i *p0,
    __m128i *q0, __m128i *q1q0, __m128i *p1p0, const unsigned char *_blimit,
    const unsigned char *_limit, const unsigned char *_thresh) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i blimit = _mm_load_si128((const __m128i *)_blimit);
  const __m128i limit = _mm_load_si128((const __m128i *)_limit);
  const __m128i thresh = _mm_load_si128((const __m128i *)_thresh);
  __m128i mask, hev, flat;
  __m128i q2p2, q1p1, q0p0, flat_p1p0, flat_q0q1;
  __m128i p2_16, q2_16, p1_16, q1_16, p0_16, q0_16;
  __m128i ps1ps0, qs1qs0;

  q2p2 = _mm_unpacklo_epi64(*p2, *q2);
  q1p1 = _mm_unpacklo_epi64(*p1, *q1);
  q0p0 = _mm_unpacklo_epi64(*p0, *q0);

  *p1p0 = _mm_unpacklo_epi64(q0p0, q1p1);
  *q1q0 = _mm_unpackhi_epi64(q0p0, q1p1);

  const __m128i one = _mm_set1_epi8(1);
  const __m128i fe = _mm_set1_epi8(0xfe);
  const __m128i ff = _mm_cmpeq_epi8(fe, fe);
  {
    // filter_mask and hev_mask
    __m128i abs_p1q1, abs_p0q0, abs_q1q0, abs_p1p0, work;
    abs_p1p0 = abs_diff(q1p1, q0p0);
    abs_q1q0 = _mm_srli_si128(abs_p1p0, 8);

    abs_p0q0 = abs_diff(*p1p0, *q1q0);
    abs_p1q1 = _mm_srli_si128(abs_p0q0, 8);
    abs_p0q0 = _mm_unpacklo_epi64(abs_p0q0, zero);

    // considering sse doesn't have unsigned elements comparison the idea is
    // to find at least one case when X > limit, it means the corresponding
    // mask bit is set.
    // to achieve that we find global max value of all inputs of abs(x-y) or
    // (abs(p0 - q0) * 2 + abs(p1 - q1) / 2 If it is > limit the mask is set
    // otherwise - not

    flat = _mm_max_epu8(abs_p1p0, abs_q1q0);
    hev = _mm_subs_epu8(flat, thresh);
    hev = _mm_xor_si128(_mm_cmpeq_epi8(hev, zero), ff);
    // replicate for the further "merged variables" usage
    hev = _mm_unpacklo_epi64(hev, hev);

    abs_p0q0 = _mm_adds_epu8(abs_p0q0, abs_p0q0);
    abs_p1q1 = _mm_srli_epi16(_mm_and_si128(abs_p1q1, fe), 1);
    mask = _mm_subs_epu8(_mm_adds_epu8(abs_p0q0, abs_p1q1), blimit);
    mask = _mm_xor_si128(_mm_cmpeq_epi8(mask, zero), ff);
    // mask |= (abs(p0 - q0) * 2 + abs(p1 - q1) / 2  > blimit) * -1;
    mask = _mm_max_epu8(abs_p1p0, mask);
    // mask |= (abs(p1 - p0) > limit) * -1;
    // mask |= (abs(q1 - q0) > limit) * -1;

    work = abs_diff(q2p2, q1p1);
    mask = _mm_max_epu8(work, mask);
    mask = _mm_max_epu8(mask, _mm_srli_si128(mask, 8));
    mask = _mm_subs_epu8(mask, limit);
    mask = _mm_cmpeq_epi8(mask, zero);
    // replicate for the further "merged variables" usage
    mask = _mm_unpacklo_epi64(mask, mask);

    // flat_mask
    flat = _mm_max_epu8(abs_diff(q2p2, q0p0), abs_p1p0);
    flat = _mm_max_epu8(flat, _mm_srli_si128(flat, 8));
    flat = _mm_subs_epu8(flat, one);
    flat = _mm_cmpeq_epi8(flat, zero);
    flat = _mm_and_si128(flat, mask);
    // replicate for the further "merged variables" usage
    flat = _mm_unpacklo_epi64(flat, flat);
  }

  // 5 tap filter
  {
    const __m128i four = _mm_set1_epi16(4);

    __m128i workp_a, workp_b, workp_shft0, workp_shft1;
    p2_16 = _mm_unpacklo_epi8(*p2, zero);
    p1_16 = _mm_unpacklo_epi8(*p1, zero);
    p0_16 = _mm_unpacklo_epi8(*p0, zero);
    q0_16 = _mm_unpacklo_epi8(*q0, zero);
    q1_16 = _mm_unpacklo_epi8(*q1, zero);
    q2_16 = _mm_unpacklo_epi8(*q2, zero);

    // op1
    workp_a = _mm_add_epi16(_mm_add_epi16(p0_16, p0_16),
                            _mm_add_epi16(p1_16, p1_16));  // p0 *2 + p1 * 2
    workp_a = _mm_add_epi16(_mm_add_epi16(workp_a, four),
                            p2_16);  // p2 + p0 * 2 + p1 * 2 + 4

    workp_b = _mm_add_epi16(_mm_add_epi16(p2_16, p2_16), q0_16);
    workp_shft0 = _mm_srli_epi16(_mm_add_epi16(workp_a, workp_b),
                                 3);  // p2 * 3 + p1 * 2 + p0 * 2 + q0 + 4

    // op0
    workp_b = _mm_add_epi16(_mm_add_epi16(q0_16, q0_16), q1_16);  // q0 * 2 + q1
    workp_a = _mm_add_epi16(workp_a,
                            workp_b);  // p2 + p0 * 2 + p1 * 2 + q0 * 2 + q1 + 4
    workp_shft1 = _mm_srli_epi16(workp_a, 3);

    flat_p1p0 = _mm_packus_epi16(workp_shft1, workp_shft0);

    // oq0
    workp_a = _mm_sub_epi16(_mm_sub_epi16(workp_a, p2_16),
                            p1_16);  // p0 * 2 + p1  + q0 * 2 + q1 + 4
    workp_b = _mm_add_epi16(q1_16, q2_16);
    workp_a = _mm_add_epi16(
        workp_a, workp_b);  // p0 * 2 + p1  + q0 * 2 + q1 * 2 + q2 + 4
    workp_shft0 = _mm_srli_epi16(workp_a, 3);

    // oq1
    workp_a = _mm_sub_epi16(_mm_sub_epi16(workp_a, p1_16),
                            p0_16);  // p0   + q0 * 2 + q1 * 2 + q2 + 4
    workp_b = _mm_add_epi16(q2_16, q2_16);
    workp_shft1 = _mm_srli_epi16(_mm_add_epi16(workp_a, workp_b),
                                 3);  // p0  + q0 * 2 + q1 * 2 + q2 * 3 + 4

    flat_q0q1 = _mm_packus_epi16(workp_shft0, workp_shft1);
  }

  // lp filter - the same for 6, 8 and 14 versions
  filter4_sse2(p1p0, q1q0, &hev, &mask, &qs1qs0, &ps1ps0);

  qs1qs0 = _mm_andnot_si128(flat, qs1qs0);
  *q1q0 = _mm_and_si128(flat, flat_q0q1);
  *q1q0 = _mm_or_si128(qs1qs0, *q1q0);

  ps1ps0 = _mm_andnot_si128(flat, ps1ps0);
  *p1p0 = _mm_and_si128(flat, flat_p1p0);
  *p1p0 = _mm_or_si128(ps1ps0, *p1p0);
}

void aom_lpf_horizontal_6_sse2(unsigned char *s, int p,
                               const unsigned char *_blimit,
                               const unsigned char *_limit,
                               const unsigned char *_thresh) {
  __m128i p2, p1, p0, q0, q1, q2;
  __m128i p1p0, q1q0;

  p2 = _mm_cvtsi32_si128(*(int *)(s - 3 * p));
  p1 = _mm_cvtsi32_si128(*(int *)(s - 2 * p));
  p0 = _mm_cvtsi32_si128(*(int *)(s - 1 * p));
  q0 = _mm_cvtsi32_si128(*(int *)(s - 0 * p));
  q1 = _mm_cvtsi32_si128(*(int *)(s + 1 * p));
  q2 = _mm_cvtsi32_si128(*(int *)(s + 2 * p));

  lpf_internal_6_sse2(&p2, &q2, &p1, &q1, &p0, &q0, &q1q0, &p1p0, _blimit,
                      _limit, _thresh);

  xx_storel_32(s - 1 * p, p1p0);
  xx_storel_32(s - 2 * p, _mm_srli_si128(p1p0, 8));
  xx_storel_32(s + 0 * p, q1q0);
  xx_storel_32(s + 1 * p, _mm_srli_si128(q1q0, 8));
}

static AOM_FORCE_INLINE void lpf_internal_8_sse2(
    __m128i *p3, __m128i *q3, __m128i *p2, __m128i *q2, __m128i *p1,
    __m128i *q1, __m128i *p0, __m128i *q0, __m128i *q1q0_out, __m128i *p1p0_out,
    __m128i *p2_out, __m128i *q2_out, __m128i *blimit, __m128i *limit,
    __m128i *thresh) {
  const __m128i zero = _mm_setzero_si128();
  __m128i mask, hev, flat;
  __m128i p2_16, q2_16, p1_16, p0_16, q0_16, q1_16, p3_16, q3_16, q3p3,
      flat_p1p0, flat_q0q1;
  __m128i q2p2, q1p1, q0p0;
  __m128i q1q0, p1p0, ps1ps0, qs1qs0;
  __m128i work_a, op2, oq2;

  q3p3 = _mm_unpacklo_epi64(*p3, *q3);
  q2p2 = _mm_unpacklo_epi64(*p2, *q2);
  q1p1 = _mm_unpacklo_epi64(*p1, *q1);
  q0p0 = _mm_unpacklo_epi64(*p0, *q0);

  p1p0 = _mm_unpacklo_epi64(q0p0, q1p1);
  q1q0 = _mm_unpackhi_epi64(q0p0, q1p1);

  {
    // filter_mask and hev_mask

    // considering sse doesn't have unsigned elements comparison the idea is to
    // find at least one case when X > limit, it means the corresponding  mask
    // bit is set.
    // to achieve that we find global max value of all inputs of abs(x-y) or
    // (abs(p0 - q0) * 2 + abs(p1 - q1) / 2 If it is > limit the mask is set
    // otherwise - not

    const __m128i one = _mm_set1_epi8(1);
    const __m128i fe = _mm_set1_epi8(0xfe);
    const __m128i ff = _mm_cmpeq_epi8(fe, fe);
    __m128i abs_p1q1, abs_p0q0, abs_q1q0, abs_p1p0, work;

    abs_p1p0 = abs_diff(q1p1, q0p0);
    abs_q1q0 = _mm_srli_si128(abs_p1p0, 8);

    abs_p0q0 = abs_diff(p1p0, q1q0);
    abs_p1q1 = _mm_srli_si128(abs_p0q0, 8);
    abs_p0q0 = _mm_unpacklo_epi64(abs_p0q0, abs_p0q0);

    flat = _mm_max_epu8(abs_p1p0, abs_q1q0);
    hev = _mm_subs_epu8(flat, *thresh);
    hev = _mm_xor_si128(_mm_cmpeq_epi8(hev, zero), ff);
    // replicate for the further "merged variables" usage
    hev = _mm_unpacklo_epi64(hev, hev);

    abs_p0q0 = _mm_adds_epu8(abs_p0q0, abs_p0q0);
    abs_p1q1 = _mm_srli_epi16(_mm_and_si128(abs_p1q1, fe), 1);
    mask = _mm_subs_epu8(_mm_adds_epu8(abs_p0q0, abs_p1q1), *blimit);
    mask = _mm_xor_si128(_mm_cmpeq_epi8(mask, zero), ff);
    // mask |= (abs(p0 - q0) * 2 + abs(p1 - q1) / 2  > blimit) * -1;
    mask = _mm_max_epu8(abs_p1p0, mask);
    // mask |= (abs(p1 - p0) > limit) * -1;
    // mask |= (abs(q1 - q0) > limit) * -1;

    work = _mm_max_epu8(abs_diff(q2p2, q1p1), abs_diff(q3p3, q2p2));

    mask = _mm_max_epu8(work, mask);
    mask = _mm_max_epu8(mask, _mm_srli_si128(mask, 8));
    mask = _mm_subs_epu8(mask, *limit);
    mask = _mm_cmpeq_epi8(mask, zero);
    // replicate for the further "merged variables" usage
    mask = _mm_unpacklo_epi64(mask, mask);

    // flat_mask4

    flat = _mm_max_epu8(abs_diff(q2p2, q0p0), abs_diff(q3p3, q0p0));
    flat = _mm_max_epu8(abs_p1p0, flat);

    flat = _mm_max_epu8(flat, _mm_srli_si128(flat, 8));
    flat = _mm_subs_epu8(flat, one);
    flat = _mm_cmpeq_epi8(flat, zero);
    flat = _mm_and_si128(flat, mask);
    // replicate for the further "merged variables" usage
    flat = _mm_unpacklo_epi64(flat, flat);
  }

  // filter8
  {
    const __m128i four = _mm_set1_epi16(4);

    __m128i workp_a, workp_b, workp_shft0, workp_shft1;
    p2_16 = _mm_unpacklo_epi8(*p2, zero);
    p1_16 = _mm_unpacklo_epi8(*p1, zero);
    p0_16 = _mm_unpacklo_epi8(*p0, zero);
    q0_16 = _mm_unpacklo_epi8(*q0, zero);
    q1_16 = _mm_unpacklo_epi8(*q1, zero);
    q2_16 = _mm_unpacklo_epi8(*q2, zero);
    p3_16 = _mm_unpacklo_epi8(*p3, zero);
    q3_16 = _mm_unpacklo_epi8(*q3, zero);

    // op2
    workp_a =
        _mm_add_epi16(_mm_add_epi16(p3_16, p3_16), _mm_add_epi16(p2_16, p1_16));
    workp_a = _mm_add_epi16(_mm_add_epi16(workp_a, four), p0_16);
    workp_b = _mm_add_epi16(_mm_add_epi16(q0_16, p2_16), p3_16);
    workp_shft0 = _mm_srli_epi16(_mm_add_epi16(workp_a, workp_b), 3);
    op2 = _mm_packus_epi16(workp_shft0, workp_shft0);

    // op1
    workp_b = _mm_add_epi16(_mm_add_epi16(q0_16, q1_16), p1_16);
    workp_shft0 = _mm_srli_epi16(_mm_add_epi16(workp_a, workp_b), 3);

    // op0
    workp_a = _mm_add_epi16(_mm_sub_epi16(workp_a, p3_16), q2_16);
    workp_b = _mm_add_epi16(_mm_sub_epi16(workp_b, p1_16), p0_16);
    workp_shft1 = _mm_srli_epi16(_mm_add_epi16(workp_a, workp_b), 3);

    flat_p1p0 = _mm_packus_epi16(workp_shft1, workp_shft0);

    // oq0
    workp_a = _mm_add_epi16(_mm_sub_epi16(workp_a, p3_16), q3_16);
    workp_b = _mm_add_epi16(_mm_sub_epi16(workp_b, p0_16), q0_16);
    workp_shft0 = _mm_srli_epi16(_mm_add_epi16(workp_a, workp_b), 3);

    // oq1
    workp_a = _mm_add_epi16(_mm_sub_epi16(workp_a, p2_16), q3_16);
    workp_b = _mm_add_epi16(_mm_sub_epi16(workp_b, q0_16), q1_16);
    workp_shft1 = _mm_srli_epi16(_mm_add_epi16(workp_a, workp_b), 3);

    flat_q0q1 = _mm_packus_epi16(workp_shft0, workp_shft1);

    // oq2
    workp_a = _mm_add_epi16(_mm_sub_epi16(workp_a, p1_16), q3_16);
    workp_b = _mm_add_epi16(_mm_sub_epi16(workp_b, q1_16), q2_16);
    workp_shft1 = _mm_srli_epi16(_mm_add_epi16(workp_a, workp_b), 3);
    oq2 = _mm_packus_epi16(workp_shft1, workp_shft1);
  }

  // lp filter - the same for 6, 8 and 14 versions
  filter4_sse2(&p1p0, &q1q0, &hev, &mask, &qs1qs0, &ps1ps0);

  qs1qs0 = _mm_andnot_si128(flat, qs1qs0);
  q1q0 = _mm_and_si128(flat, flat_q0q1);
  *q1q0_out = _mm_or_si128(qs1qs0, q1q0);

  ps1ps0 = _mm_andnot_si128(flat, ps1ps0);
  p1p0 = _mm_and_si128(flat, flat_p1p0);
  *p1p0_out = _mm_or_si128(ps1ps0, p1p0);

  work_a = _mm_andnot_si128(flat, *q2);
  q2_16 = _mm_and_si128(flat, oq2);
  *q2_out = _mm_or_si128(work_a, q2_16);

  work_a = _mm_andnot_si128(flat, *p2);
  p2_16 = _mm_and_si128(flat, op2);
  *p2_out = _mm_or_si128(work_a, p2_16);
}

void aom_lpf_horizontal_8_sse2(unsigned char *s, int p,
                               const unsigned char *_blimit,
                               const unsigned char *_limit,
                               const unsigned char *_thresh) {
  __m128i p2, p1, p0, q0, q1, q2, p3, q3;
  __m128i q1q0, p1p0, p2_out, q2_out;
  __m128i blimit = _mm_load_si128((const __m128i *)_blimit);
  __m128i limit = _mm_load_si128((const __m128i *)_limit);
  __m128i thresh = _mm_load_si128((const __m128i *)_thresh);

  p3 = _mm_cvtsi32_si128(*(int *)(s - 4 * p));
  p2 = _mm_cvtsi32_si128(*(int *)(s - 3 * p));
  p1 = _mm_cvtsi32_si128(*(int *)(s - 2 * p));
  p0 = _mm_cvtsi32_si128(*(int *)(s - 1 * p));
  q0 = _mm_cvtsi32_si128(*(int *)(s - 0 * p));
  q1 = _mm_cvtsi32_si128(*(int *)(s + 1 * p));
  q2 = _mm_cvtsi32_si128(*(int *)(s + 2 * p));
  q3 = _mm_cvtsi32_si128(*(int *)(s + 3 * p));

  lpf_internal_8_sse2(&p3, &q3, &p2, &q2, &p1, &q1, &p0, &q0, &q1q0, &p1p0,
                      &p2_out, &q2_out, &blimit, &limit, &thresh);

  xx_storel_32(s - 1 * p, p1p0);
  xx_storel_32(s - 2 * p, _mm_srli_si128(p1p0, 8));
  xx_storel_32(s + 0 * p, q1q0);
  xx_storel_32(s + 1 * p, _mm_srli_si128(q1q0, 8));
  xx_storel_32(s - 3 * p, p2_out);
  xx_storel_32(s + 2 * p, q2_out);
}

void aom_lpf_horizontal_14_dual_sse2(unsigned char *s, int p,
                                     const unsigned char *_blimit0,
                                     const unsigned char *_limit0,
                                     const unsigned char *_thresh0,
                                     const unsigned char *_blimit1,
                                     const unsigned char *_limit1,
                                     const unsigned char *_thresh1) {
  __m128i q6p6, q5p5, q4p4, q3p3, q2p2, q1p1, q0p0;
  __m128i blimit =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_blimit0),
                         _mm_load_si128((const __m128i *)_blimit1));
  __m128i limit = _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_limit0),
                                     _mm_load_si128((const __m128i *)_limit1));
  __m128i thresh =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_thresh0),
                         _mm_load_si128((const __m128i *)_thresh1));

  q4p4 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i *)(s - 5 * p)),
                            _mm_loadl_epi64((__m128i *)(s + 4 * p)));
  q3p3 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i *)(s - 4 * p)),
                            _mm_loadl_epi64((__m128i *)(s + 3 * p)));
  q2p2 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i *)(s - 3 * p)),
                            _mm_loadl_epi64((__m128i *)(s + 2 * p)));
  q1p1 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i *)(s - 2 * p)),
                            _mm_loadl_epi64((__m128i *)(s + 1 * p)));

  q0p0 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i *)(s - 1 * p)),
                            _mm_loadl_epi64((__m128i *)(s - 0 * p)));

  q5p5 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i *)(s - 6 * p)),
                            _mm_loadl_epi64((__m128i *)(s + 5 * p)));

  q6p6 = _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i *)(s - 7 * p)),
                            _mm_loadl_epi64((__m128i *)(s + 6 * p)));

  lpf_internal_14_sse2(&q6p6, &q5p5, &q4p4, &q3p3, &q2p2, &q1p1, &q0p0, &blimit,
                       &limit, &thresh);

  _mm_storel_epi64((__m128i *)(s - 1 * p), q0p0);
  _mm_storel_epi64((__m128i *)(s + 0 * p), _mm_srli_si128(q0p0, 8));
  _mm_storel_epi64((__m128i *)(s - 2 * p), q1p1);
  _mm_storel_epi64((__m128i *)(s + 1 * p), _mm_srli_si128(q1p1, 8));
  _mm_storel_epi64((__m128i *)(s - 3 * p), q2p2);
  _mm_storel_epi64((__m128i *)(s + 2 * p), _mm_srli_si128(q2p2, 8));
  _mm_storel_epi64((__m128i *)(s - 4 * p), q3p3);
  _mm_storel_epi64((__m128i *)(s + 3 * p), _mm_srli_si128(q3p3, 8));
  _mm_storel_epi64((__m128i *)(s - 5 * p), q4p4);
  _mm_storel_epi64((__m128i *)(s + 4 * p), _mm_srli_si128(q4p4, 8));
  _mm_storel_epi64((__m128i *)(s - 6 * p), q5p5);
  _mm_storel_epi64((__m128i *)(s + 5 * p), _mm_srli_si128(q5p5, 8));
}

void aom_lpf_horizontal_8_dual_sse2(uint8_t *s, int p, const uint8_t *_blimit0,
                                    const uint8_t *_limit0,
                                    const uint8_t *_thresh0,
                                    const uint8_t *_blimit1,
                                    const uint8_t *_limit1,
                                    const uint8_t *_thresh1) {
  __m128i blimit =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_blimit0),
                         _mm_load_si128((const __m128i *)_blimit1));
  __m128i limit = _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_limit0),
                                     _mm_load_si128((const __m128i *)_limit1));
  __m128i thresh =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_thresh0),
                         _mm_load_si128((const __m128i *)_thresh1));

  __m128i p2, p1, p0, q0, q1, q2, p3, q3;
  __m128i q1q0, p1p0, p2_out, q2_out;

  p3 = _mm_loadl_epi64((__m128i *)(s - 4 * p));
  p2 = _mm_loadl_epi64((__m128i *)(s - 3 * p));
  p1 = _mm_loadl_epi64((__m128i *)(s - 2 * p));
  p0 = _mm_loadl_epi64((__m128i *)(s - 1 * p));
  q0 = _mm_loadl_epi64((__m128i *)(s - 0 * p));
  q1 = _mm_loadl_epi64((__m128i *)(s + 1 * p));
  q2 = _mm_loadl_epi64((__m128i *)(s + 2 * p));
  q3 = _mm_loadl_epi64((__m128i *)(s + 3 * p));

  lpf_internal_8_sse2(&p3, &q3, &p2, &q2, &p1, &q1, &p0, &q0, &q1q0, &p1p0,
                      &p2_out, &q2_out, &blimit, &limit, &thresh);

  _mm_storel_epi64((__m128i *)(s - 1 * p), p1p0);
  _mm_storel_epi64((__m128i *)(s - 2 * p), _mm_srli_si128(p1p0, 8));
  _mm_storel_epi64((__m128i *)(s + 0 * p), q1q0);
  _mm_storel_epi64((__m128i *)(s + 1 * p), _mm_srli_si128(q1q0, 8));
  _mm_storel_epi64((__m128i *)(s - 3 * p), p2_out);
  _mm_storel_epi64((__m128i *)(s + 2 * p), q2_out);
}

void aom_lpf_horizontal_4_dual_sse2(unsigned char *s, int p,
                                    const unsigned char *_blimit0,
                                    const unsigned char *_limit0,
                                    const unsigned char *_thresh0,
                                    const unsigned char *_blimit1,
                                    const unsigned char *_limit1,
                                    const unsigned char *_thresh1) {
  __m128i p1, p0, q0, q1;
  __m128i qs1qs0, ps1ps0;

  p1 = _mm_loadl_epi64((__m128i *)(s - 2 * p));
  p0 = _mm_loadl_epi64((__m128i *)(s - 1 * p));
  q0 = _mm_loadl_epi64((__m128i *)(s - 0 * p));
  q1 = _mm_loadl_epi64((__m128i *)(s + 1 * p));

  const __m128i zero = _mm_setzero_si128();
  const __m128i blimit =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_blimit0),
                         _mm_load_si128((const __m128i *)_blimit1));
  const __m128i limit =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_limit0),
                         _mm_load_si128((const __m128i *)_limit1));

  __m128i l = _mm_unpacklo_epi64(blimit, limit);

  __m128i thresh0 =
      _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *)_thresh0), zero);

  __m128i thresh1 =
      _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *)_thresh1), zero);

  __m128i t = _mm_unpacklo_epi64(thresh0, thresh1);

  lpf_internal_4_sse2(&p1, &p0, &q0, &q1, &l, &t, &qs1qs0, &ps1ps0);

  _mm_storel_epi64((__m128i *)(s - 1 * p), ps1ps0);
  _mm_storel_epi64((__m128i *)(s - 2 * p), _mm_srli_si128(ps1ps0, 8));
  _mm_storel_epi64((__m128i *)(s + 0 * p), qs1qs0);
  _mm_storel_epi64((__m128i *)(s + 1 * p), _mm_srli_si128(qs1qs0, 8));
}

void aom_lpf_vertical_4_dual_sse2(uint8_t *s, int p, const uint8_t *_blimit0,
                                  const uint8_t *_limit0,
                                  const uint8_t *_thresh0,
                                  const uint8_t *_blimit1,
                                  const uint8_t *_limit1,
                                  const uint8_t *_thresh1) {
  __m128i p0, q0, q1, p1;
  __m128i x0, x1, x2, x3, x4, x5, x6, x7;
  __m128i d0, d1, d2, d3, d4, d5, d6, d7;
  __m128i qs1qs0, ps1ps0;

  const __m128i zero = _mm_setzero_si128();
  const __m128i blimit =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_blimit0),
                         _mm_load_si128((const __m128i *)_blimit1));
  const __m128i limit =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_limit0),
                         _mm_load_si128((const __m128i *)_limit1));

  __m128i l = _mm_unpacklo_epi64(blimit, limit);

  __m128i thresh0 =
      _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *)_thresh0), zero);

  __m128i thresh1 =
      _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *)_thresh1), zero);

  __m128i t = _mm_unpacklo_epi64(thresh0, thresh1);

  x0 = _mm_loadl_epi64((__m128i *)((s - 2)));
  x1 = _mm_loadl_epi64((__m128i *)((s - 2) + p));
  x2 = _mm_loadl_epi64((__m128i *)((s - 2) + 2 * p));
  x3 = _mm_loadl_epi64((__m128i *)((s - 2) + 3 * p));
  x4 = _mm_loadl_epi64((__m128i *)((s - 2) + 4 * p));
  x5 = _mm_loadl_epi64((__m128i *)((s - 2) + 5 * p));
  x6 = _mm_loadl_epi64((__m128i *)((s - 2) + 6 * p));
  x7 = _mm_loadl_epi64((__m128i *)((s - 2) + 7 * p));

  transpose8x8_low_sse2(&x0, &x1, &x2, &x3, &x4, &x5, &x6, &x7, &p1, &p0, &q0,
                        &q1);

  lpf_internal_4_sse2(&p1, &p0, &q0, &q1, &l, &t, &qs1qs0, &ps1ps0);

  p1 = _mm_srli_si128(ps1ps0, 8);
  q1 = _mm_srli_si128(qs1qs0, 8);

  transpose4x8_8x4_sse2(&p1, &ps1ps0, &qs1qs0, &q1, &d0, &d1, &d2, &d3, &d4,
                        &d5, &d6, &d7);

  xx_storel_32((s - 2 + 0 * p), d0);
  xx_storel_32((s - 2 + 1 * p), d1);
  xx_storel_32((s - 2 + 2 * p), d2);
  xx_storel_32((s - 2 + 3 * p), d3);
  xx_storel_32((s - 2 + 4 * p), d4);
  xx_storel_32((s - 2 + 5 * p), d5);
  xx_storel_32((s - 2 + 6 * p), d6);
  xx_storel_32((s - 2 + 7 * p), d7);
}

void aom_lpf_vertical_6_sse2(unsigned char *s, int p,
                             const unsigned char *blimit,
                             const unsigned char *limit,
                             const unsigned char *thresh) {
  __m128i d0d1, d2d3, d4d5;
  __m128i d1, d3, d5;

  __m128i p2, p1, p0, q0, q1, q2;
  __m128i p1p0, q1q0;
  DECLARE_ALIGNED(16, unsigned char, temp_dst[16]);

  p2 = _mm_loadl_epi64((__m128i *)((s - 3) + 0 * p));
  p1 = _mm_loadl_epi64((__m128i *)((s - 3) + 1 * p));
  p0 = _mm_loadl_epi64((__m128i *)((s - 3) + 2 * p));
  q0 = _mm_loadl_epi64((__m128i *)((s - 3) + 3 * p));
  q1 = _mm_setzero_si128();
  q2 = _mm_setzero_si128();

  transpose6x6_sse2(&p2, &p1, &p0, &q0, &q1, &q2, &d0d1, &d2d3, &d4d5);

  d1 = _mm_srli_si128(d0d1, 8);
  d3 = _mm_srli_si128(d2d3, 8);
  d5 = _mm_srli_si128(d4d5, 8);

  // Loop filtering
  lpf_internal_6_sse2(&d0d1, &d5, &d1, &d4d5, &d2d3, &d3, &q1q0, &p1p0, blimit,
                      limit, thresh);

  p0 = _mm_srli_si128(p1p0, 8);
  q0 = _mm_srli_si128(q1q0, 8);

  transpose6x6_sse2(&d0d1, &p0, &p1p0, &q1q0, &q0, &d5, &d0d1, &d2d3, &d4d5);

  _mm_store_si128((__m128i *)(temp_dst), d0d1);
  memcpy((s - 3) + 0 * p, temp_dst, 6);
  memcpy((s - 3) + 1 * p, temp_dst + 8, 6);
  _mm_store_si128((__m128i *)(temp_dst), d2d3);
  memcpy((s - 3) + 2 * p, temp_dst, 6);
  memcpy((s - 3) + 3 * p, temp_dst + 8, 6);
}

void aom_lpf_vertical_8_sse2(unsigned char *s, int p,
                             const unsigned char *_blimit,
                             const unsigned char *_limit,
                             const unsigned char *_thresh) {
  __m128i d0, d1, d2, d3, d4, d5, d6, d7;

  __m128i p2, p0, q0, q2;
  __m128i x2, x1, x0, x3;
  __m128i q1q0, p1p0;
  __m128i blimit = _mm_load_si128((const __m128i *)_blimit);
  __m128i limit = _mm_load_si128((const __m128i *)_limit);
  __m128i thresh = _mm_load_si128((const __m128i *)_thresh);

  x3 = _mm_loadl_epi64((__m128i *)((s - 4) + 0 * p));
  x2 = _mm_loadl_epi64((__m128i *)((s - 4) + 1 * p));
  x1 = _mm_loadl_epi64((__m128i *)((s - 4) + 2 * p));
  x0 = _mm_loadl_epi64((__m128i *)((s - 4) + 3 * p));

  transpose4x8_8x4_sse2(&x3, &x2, &x1, &x0, &d0, &d1, &d2, &d3, &d4, &d5, &d6,
                        &d7);
  // Loop filtering
  lpf_internal_8_sse2(&d0, &d7, &d1, &d6, &d2, &d5, &d3, &d4, &q1q0, &p1p0, &p2,
                      &q2, &blimit, &limit, &thresh);

  p0 = _mm_srli_si128(p1p0, 8);
  q0 = _mm_srli_si128(q1q0, 8);

  transpose8x8_low_sse2(&d0, &p2, &p0, &p1p0, &q1q0, &q0, &q2, &d7, &d0, &d1,
                        &d2, &d3);

  _mm_storel_epi64((__m128i *)(s - 4 + 0 * p), d0);
  _mm_storel_epi64((__m128i *)(s - 4 + 1 * p), d1);
  _mm_storel_epi64((__m128i *)(s - 4 + 2 * p), d2);
  _mm_storel_epi64((__m128i *)(s - 4 + 3 * p), d3);
}

void aom_lpf_vertical_8_dual_sse2(uint8_t *s, int p, const uint8_t *_blimit0,
                                  const uint8_t *_limit0,
                                  const uint8_t *_thresh0,
                                  const uint8_t *_blimit1,
                                  const uint8_t *_limit1,
                                  const uint8_t *_thresh1) {
  __m128i blimit =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_blimit0),
                         _mm_load_si128((const __m128i *)_blimit1));
  __m128i limit = _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_limit0),
                                     _mm_load_si128((const __m128i *)_limit1));
  __m128i thresh =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_thresh0),
                         _mm_load_si128((const __m128i *)_thresh1));

  __m128i x0, x1, x2, x3, x4, x5, x6, x7;
  __m128i d1, d3, d5, d7;
  __m128i q1q0, p1p0;
  __m128i p2, p1, q1, q2;
  __m128i d0d1, d2d3, d4d5, d6d7;

  x0 = _mm_loadl_epi64((__m128i *)(s - 4 + 0 * p));
  x1 = _mm_loadl_epi64((__m128i *)(s - 4 + 1 * p));
  x2 = _mm_loadl_epi64((__m128i *)(s - 4 + 2 * p));
  x3 = _mm_loadl_epi64((__m128i *)(s - 4 + 3 * p));
  x4 = _mm_loadl_epi64((__m128i *)(s - 4 + 4 * p));
  x5 = _mm_loadl_epi64((__m128i *)(s - 4 + 5 * p));
  x6 = _mm_loadl_epi64((__m128i *)(s - 4 + 6 * p));
  x7 = _mm_loadl_epi64((__m128i *)(s - 4 + 7 * p));

  transpose8x8_sse2(&x0, &x1, &x2, &x3, &x4, &x5, &x6, &x7, &d0d1, &d2d3, &d4d5,
                    &d6d7);

  d1 = _mm_srli_si128(d0d1, 8);
  d3 = _mm_srli_si128(d2d3, 8);
  d5 = _mm_srli_si128(d4d5, 8);
  d7 = _mm_srli_si128(d6d7, 8);

  lpf_internal_8_sse2(&d0d1, &d7, &d1, &d6d7, &d2d3, &d5, &d3, &d4d5, &q1q0,
                      &p1p0, &p2, &q2, &blimit, &limit, &thresh);

  p1 = _mm_srli_si128(p1p0, 8);
  q1 = _mm_srli_si128(q1q0, 8);

  transpose8x8_sse2(&d0d1, &p2, &p1, &p1p0, &q1q0, &q1, &q2, &d7, &d0d1, &d2d3,
                    &d4d5, &d6d7);

  _mm_storel_epi64((__m128i *)(s - 4 + 0 * p), d0d1);
  _mm_storel_epi64((__m128i *)(s - 4 + 1 * p), _mm_srli_si128(d0d1, 8));
  _mm_storel_epi64((__m128i *)(s - 4 + 2 * p), d2d3);
  _mm_storel_epi64((__m128i *)(s - 4 + 3 * p), _mm_srli_si128(d2d3, 8));
  _mm_storel_epi64((__m128i *)(s - 4 + 4 * p), d4d5);
  _mm_storel_epi64((__m128i *)(s - 4 + 5 * p), _mm_srli_si128(d4d5, 8));
  _mm_storel_epi64((__m128i *)(s - 4 + 6 * p), d6d7);
  _mm_storel_epi64((__m128i *)(s - 4 + 7 * p), _mm_srli_si128(d6d7, 8));
}

void aom_lpf_vertical_14_sse2(unsigned char *s, int p,
                              const unsigned char *_blimit,
                              const unsigned char *_limit,
                              const unsigned char *_thresh) {
  __m128i q6p6, q5p5, q4p4, q3p3, q2p2, q1p1, q0p0;
  __m128i x6, x5, x4, x3, x2, x1, x0;
  __m128i p0, p1, p2, p3, p4, p5, p6, p7;
  __m128i q0, q1, q2, q3, q4, q5, q6, q7;
  __m128i p0_out, p1_out, p2_out, p3_out;
  __m128i blimit = _mm_load_si128((__m128i *)_blimit);
  __m128i limit = _mm_load_si128((__m128i *)_limit);
  __m128i thresh = _mm_load_si128((__m128i *)_thresh);

  x6 = _mm_loadl_epi64((__m128i *)((s - 8) + 0 * p));
  x5 = _mm_loadl_epi64((__m128i *)((s - 8) + 1 * p));
  x4 = _mm_loadl_epi64((__m128i *)((s - 8) + 2 * p));
  x3 = _mm_loadl_epi64((__m128i *)((s - 8) + 3 * p));

  transpose4x8_8x4_sse2(&x6, &x5, &x4, &x3, &p0, &p1, &p2, &p3, &p4, &p5, &p6,
                        &p7);

  x6 = _mm_loadl_epi64((__m128i *)(s + 0 * p));
  x5 = _mm_loadl_epi64((__m128i *)(s + 1 * p));
  x4 = _mm_loadl_epi64((__m128i *)(s + 2 * p));
  x3 = _mm_loadl_epi64((__m128i *)(s + 3 * p));

  transpose4x8_8x4_sse2(&x6, &x5, &x4, &x3, &q0, &q1, &q2, &q3, &q4, &q5, &q6,
                        &q7);

  q6p6 = _mm_unpacklo_epi64(p1, q6);
  q5p5 = _mm_unpacklo_epi64(p2, q5);
  q4p4 = _mm_unpacklo_epi64(p3, q4);
  q3p3 = _mm_unpacklo_epi64(p4, q3);
  q2p2 = _mm_unpacklo_epi64(p5, q2);
  q1p1 = _mm_unpacklo_epi64(p6, q1);
  q0p0 = _mm_unpacklo_epi64(p7, q0);

  lpf_internal_14_sse2(&q6p6, &q5p5, &q4p4, &q3p3, &q2p2, &q1p1, &q0p0, &blimit,
                       &limit, &thresh);

  transpose8x8_low_sse2(&p0, &p1, &q5p5, &q4p4, &q3p3, &q2p2, &q1p1, &q0p0,
                        &p0_out, &p1_out, &p2_out, &p3_out);

  x0 = _mm_srli_si128(q0p0, 8);
  x1 = _mm_srli_si128(q1p1, 8);
  x2 = _mm_srli_si128(q2p2, 8);
  x3 = _mm_srli_si128(q3p3, 8);
  x4 = _mm_srli_si128(q4p4, 8);
  x5 = _mm_srli_si128(q5p5, 8);
  x6 = _mm_srli_si128(q6p6, 8);

  transpose8x8_low_sse2(&x0, &x1, &x2, &x3, &x4, &x5, &x6, &q7, &q0, &q1, &q2,
                        &q3);

  _mm_storel_epi64((__m128i *)(s - 8 + 0 * p), p0_out);
  _mm_storel_epi64((__m128i *)(s - 8 + 1 * p), p1_out);
  _mm_storel_epi64((__m128i *)(s - 8 + 2 * p), p2_out);
  _mm_storel_epi64((__m128i *)(s - 8 + 3 * p), p3_out);

  _mm_storel_epi64((__m128i *)(s + 0 * p), q0);
  _mm_storel_epi64((__m128i *)(s + 1 * p), q1);
  _mm_storel_epi64((__m128i *)(s + 2 * p), q2);
  _mm_storel_epi64((__m128i *)(s + 3 * p), q3);
}

void aom_lpf_vertical_14_dual_sse2(
    unsigned char *s, int p, const uint8_t *_blimit0, const uint8_t *_limit0,
    const uint8_t *_thresh0, const uint8_t *_blimit1, const uint8_t *_limit1,
    const uint8_t *_thresh1) {
  __m128i q6p6, q5p5, q4p4, q3p3, q2p2, q1p1, q0p0;
  __m128i x7, x6, x5, x4, x3, x2, x1, x0;
  __m128i d0d1, d2d3, d4d5, d6d7, d8d9, d10d11, d12d13, d14d15;
  __m128i q0, q1, q2, q3, q7;
  __m128i p0p1, p2p3, p4p5, p6p7;

  __m128i blimit =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_blimit0),
                         _mm_load_si128((const __m128i *)_blimit1));
  __m128i limit = _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_limit0),
                                     _mm_load_si128((const __m128i *)_limit1));
  __m128i thresh =
      _mm_unpacklo_epi32(_mm_load_si128((const __m128i *)_thresh0),
                         _mm_load_si128((const __m128i *)_thresh1));

  x7 = _mm_loadu_si128((__m128i *)((s - 8) + 0 * p));
  x6 = _mm_loadu_si128((__m128i *)((s - 8) + 1 * p));
  x5 = _mm_loadu_si128((__m128i *)((s - 8) + 2 * p));
  x4 = _mm_loadu_si128((__m128i *)((s - 8) + 3 * p));
  x3 = _mm_loadu_si128((__m128i *)((s - 8) + 4 * p));
  x2 = _mm_loadu_si128((__m128i *)((s - 8) + 5 * p));
  x1 = _mm_loadu_si128((__m128i *)((s - 8) + 6 * p));
  x0 = _mm_loadu_si128((__m128i *)((s - 8) + 7 * p));

  transpose8x16_16x8_sse2(&x7, &x6, &x5, &x4, &x3, &x2, &x1, &x0, &d0d1, &d2d3,
                          &d4d5, &d6d7, &d8d9, &d10d11, &d12d13, &d14d15);

  q6p6 = _mm_unpacklo_epi64(d2d3, _mm_srli_si128(d12d13, 8));
  q5p5 = _mm_unpacklo_epi64(d4d5, _mm_srli_si128(d10d11, 8));
  q4p4 = _mm_unpacklo_epi64(d6d7, _mm_srli_si128(d8d9, 8));
  q3p3 = _mm_unpacklo_epi64(d8d9, _mm_srli_si128(d6d7, 8));
  q2p2 = _mm_unpacklo_epi64(d10d11, _mm_srli_si128(d4d5, 8));
  q1p1 = _mm_unpacklo_epi64(d12d13, _mm_srli_si128(d2d3, 8));
  q0p0 = _mm_unpacklo_epi64(d14d15, _mm_srli_si128(d0d1, 8));
  q7 = _mm_srli_si128(d14d15, 8);

  lpf_internal_14_sse2(&q6p6, &q5p5, &q4p4, &q3p3, &q2p2, &q1p1, &q0p0, &blimit,
                       &limit, &thresh);

  x0 = _mm_srli_si128(q0p0, 8);
  x1 = _mm_srli_si128(q1p1, 8);
  x2 = _mm_srli_si128(q2p2, 8);
  x3 = _mm_srli_si128(q3p3, 8);
  x4 = _mm_srli_si128(q4p4, 8);
  x5 = _mm_srli_si128(q5p5, 8);
  x6 = _mm_srli_si128(q6p6, 8);

  transpose16x8_8x16_sse2(&d0d1, &q6p6, &q5p5, &q4p4, &q3p3, &q2p2, &q1p1,
                          &q0p0, &x0, &x1, &x2, &x3, &x4, &x5, &x6, &q7, &p0p1,
                          &p2p3, &p4p5, &p6p7, &q0, &q1, &q2, &q3);

  _mm_storeu_si128((__m128i *)(s - 8 + 0 * p), p0p1);
  _mm_storeu_si128((__m128i *)(s - 8 + 1 * p), p2p3);
  _mm_storeu_si128((__m128i *)(s - 8 + 2 * p), p4p5);
  _mm_storeu_si128((__m128i *)(s - 8 + 3 * p), p6p7);
  _mm_storeu_si128((__m128i *)(s - 8 + 4 * p), q0);
  _mm_storeu_si128((__m128i *)(s - 8 + 5 * p), q1);
  _mm_storeu_si128((__m128i *)(s - 8 + 6 * p), q2);
  _mm_storeu_si128((__m128i *)(s - 8 + 7 * p), q3);
}
