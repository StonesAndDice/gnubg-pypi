/*
 * Stub for MinGW: avoid pulling in full x86intrin.h -> immintrin.h -> mmintrin.h
 * chain from winnt.h, which breaks with -mno-mmx. Define only what winnt.h needs.
 */
#ifndef WIN32_STUB_X86INTRIN_H
#define WIN32_STUB_X86INTRIN_H

#ifdef __cplusplus
extern "C" {
#endif

static inline void _mm_clflush(void const *__p) { (void)__p; }
static inline void _mm_lfence(void) {}
static inline void _mm_mfence(void) {}
static inline void _mm_sfence(void) {}
static inline void _mm_pause(void) {}
static inline void _mm_prefetch(void const *__p, int __i) { (void)__p; (void)__i; }
static inline void _m_prefetchw(void const *__p) { (void)__p; }
static inline unsigned int _mm_getcsr(void) { return 0; }
static inline void _mm_setcsr(unsigned int __x) { (void)__x; }
/* __faststorefence provided by p sdk_inc/intrin-impl.h - do not redefine */

#ifdef __cplusplus
}
#endif

#endif
