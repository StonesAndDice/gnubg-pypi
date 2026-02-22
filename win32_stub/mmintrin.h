/*
 * Stub for MinGW x86_64: avoid broken __m64 / MMX intrinsics pulled in via
 * windows.h -> winnt.h -> x86intrin.h -> mmintrin.h. GNUBG does not use MMX.
 * This file is included first via -I so the real mmintrin.h is never loaded.
 */
#ifndef _MMINTRIN_H_INCLUDED
#define _MMINTRIN_H_INCLUDED
#endif
