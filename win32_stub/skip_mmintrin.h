/*
 * Force the real mmintrin.h to be skipped when included later (windows.h chain).
 * -include this file so _MMINTRIN_H_INCLUDED is defined before any includes.
 */
#ifndef _MMINTRIN_H_INCLUDED
#define _MMINTRIN_H_INCLUDED
#endif
