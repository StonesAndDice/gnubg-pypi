/*
 * Header for Python stub functions
 */

#ifndef SRC_GNUBGMODULE_PYTHON_STUBS_H_
#define SRC_GNUBGMODULE_PYTHON_STUBS_H_

#if defined(USE_PYTHON)

void PythonInitialise(const char *argv0);
void PythonShutdown(void);
void PythonRun(const char *sz);
void LoadPythonFile(const char *sz, int fQuiet);
void CloseThread(void *unused);

#endif /* USE_PYTHON */

#endif  // SRC_GNUBGMODULE_PYTHON_STUBS_H_
