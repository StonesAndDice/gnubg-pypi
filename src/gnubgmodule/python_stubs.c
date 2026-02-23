/*
 * Stub implementations for old Python interpreter functions
 * These are called from gnubg_lib.c but not needed for the Python module
 */

#include "python_stubs.h"
#include "config.h"

#if defined(USE_PYTHON)

/* fClockwise, DrawBoard, FIBSBoard are defined in drawboard.c (linked via
 * libgnubg) - do not redefine here */

/* Multithreading: real MT_InitThreads, MT_Close, TLSCreate, TLSSetValue, etc.
 * come from mtsupport.c so eval has valid nnState (MT_Get_nnState()). No stubs here. */

/* Stub: Old Python interpreter initialization */
void PythonInitialise(const char *argv0) {
  /* Not needed for Python module - initialization happens in PyInit__gnubg */
  (void)argv0;
}

/* Stub: Old Python interpreter shutdown */
void PythonShutdown(void) { (void)0; /* Not needed for Python module */ }

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

/* CloseThread: provided by mtsupport.c (used by MT_CloseThreads) */

/* DrawBoard and FIBSBoard are defined in drawboard.c (linked via libgnubg) - do
 * not stub here */

void SGFErrorHandler(void *unused1, void *unused2) {
  (void)unused1;
  (void)unused2;
}

/* Move formatting and parsing functions (FormatMove, FormatMovePlain,
 * ParseMove, CanonicalMoveOrder) are now provided by drawboard.c - no stubs
 * needed */

/* Stub: SGF parsing - not needed for Python module */
int SGFParse(void *unused1) {
  /* Not needed for Python module */
  (void)unused1;
  return -1;
}

#endif /* USE_PYTHON */
