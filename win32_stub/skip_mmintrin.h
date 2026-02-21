/*
 * Force the real mmintrin.h to be skipped when included later (windows.h chain).
 * -include this file so _MMINTRIN_H_INCLUDED is defined before any includes.
 */
 #ifndef _MMINTRIN_H_INCLUDED
 #define _MMINTRIN_H_INCLUDED
 
 /* Provide a dummy __m64 type to satisfy downstream headers like xmmintrin.h */
 typedef long long __m64;
 
 #endif