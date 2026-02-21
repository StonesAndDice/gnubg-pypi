/*
 * Stub implementations for old Python interpreter functions
 * These are called from gnubg_lib.c but not needed for the Python module
 */

#include "config.h"

#if defined(USE_PYTHON)

/* fClockwise, DrawBoard, FIBSBoard are defined in drawboard.c (linked via libgnubg) - do not redefine here */

/* Stub: Multithreading functions - not needed for Python module */
void MT_Release(void) {
    /* Not needed - single threaded Python module */
}

void MT_Exclusive(void) {
    /* Not needed - single threaded Python module */
}

void MT_Close(void) {
    /* Not needed - single threaded Python module */
}

void MT_InitThreads(void) {
    /* Not needed - single threaded Python module */
}

void MT_CreateThreadLocalData(void *unused) {
    /* Not needed - single threaded Python module */
    (void)unused;
}

void Mutex_Lock(void *unused) {
    /* Not needed - single threaded Python module */
    (void)unused;
}

void Mutex_Release(void *unused) {
    /* Not needed - single threaded Python module */
    (void)unused;
}

void ResetManualEvent(void *unused) {
    /* Not needed - single threaded Python module */
    (void)unused;
}

void SetManualEvent(void *unused) {
    /* Not needed - single threaded Python module */
    (void)unused;
}

void WaitForManualEvent(void *unused) {
    /* Not needed - single threaded Python module */
    (void)unused;
}

void TLSSetValue(void *unused1, void *unused2) {
    /* Not needed - single threaded Python module */
    (void)unused1; (void)unused2;
}

/* Stub: Old Python interpreter initialization */
void PythonInitialise(const char *argv0) {
    /* Not needed for Python module - initialization happens in PyInit__gnubg */
    (void)argv0;
}

/* Stub: Old Python interpreter shutdown */
void PythonShutdown(void) {
    /* Not needed for Python module */
}

/* Stub: Old Python interpreter run command */
void PythonRun(const char *sz) {
    /* Not needed for Python module */
    (void)sz;
}

/* Stub: Old Python file loader */
void LoadPythonFile(const char *sz, int fQuiet) {
    /* Not needed for Python module */
    (void)sz;
    (void)fQuiet;
}

/* Stub: Thread cleanup (from mtsupport.c) */
void CloseThread(void *unused) {
    /* Not needed for Python module - no threading */
    (void)unused;
}

/* DrawBoard and FIBSBoard are defined in drawboard.c (linked via libgnubg) - do not stub here */

void SGFErrorHandler(void *unused1, void *unused2) {
    (void)unused1; (void)unused2;
}

/* Move formatting and parsing functions (FormatMove, FormatMovePlain, ParseMove, CanonicalMoveOrder) 
 * are now provided by drawboard.c - no stubs needed */

/* Stub: SGF parsing - not needed for Python module */
int SGFParse(void *unused1) {
    /* Not needed for Python module */
    (void)unused1;
    return -1;
}

#endif /* USE_PYTHON */
