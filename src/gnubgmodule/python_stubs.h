/*
 * Header for Python stub functions
 */

#ifndef PYTHON_STUBS_H
#define PYTHON_STUBS_H

#if defined(USE_PYTHON)

void PythonInitialise(const char *argv0);
void PythonShutdown(void);
void PythonRun(const char *sz);
void LoadPythonFile(const char *sz, int fQuiet);
void CloseThread(void *unused);

#endif /* USE_PYTHON */

#endif /* PYTHON_STUBS_H */
