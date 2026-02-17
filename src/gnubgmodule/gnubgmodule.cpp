#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string>

// Include your GNUBG headers here
// Wrap them in extern "C" because they are C headers
extern "C" {
    #include "gnubgmodule.h" // Replace with actual GNUBG headers
    // Example: typedef struct { void* p; } listOLD; 
}

/* * FIX 1: Casting Keyword Lists 
 * C++ is strict about 'const char*'. Python expects 'char*'.
 */
static PyObject* py_example_keywords(PyObject* self, PyObject* args, PyObject* keywds) {
    int val1 = 0, val2 = 0;
    static const char* kwlist[] = {"val1", "val2", NULL};

    // Note the const_cast<char**> to satisfy the Python API
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|ii", 
                                     const_cast<char**>(kwlist), 
                                     &val1, &val2)) {
        return NULL;
    }
    return Py_BuildValue("ii", val1, val2);
}

/* * FIX 2: Explicit Casting for void*
 * C++ does not allow implicit conversion from void* (pl->p) to a specific pointer type.
 */
static PyObject* py_process_list(PyObject* self, PyObject* args) {
    // This is a placeholder for your 'PythonGame' logic
    // PyObject *pg = PythonGame(static_cast<const listOLD*>(pl->p), ...);
    
    Py_RETURN_NONE;
}

// Method table
static PyMethodDef GnubgMethods[] = {
    {"test_keywords", (PyCFunction)py_example_keywords, METH_VARARGS | METH_KEYWORDS, "Test keywords"},
    {"process_list", py_process_list, METH_VARARGS, "Test void* casting"},
    {NULL, NULL, 0, NULL}
};

// Module definition
static struct PyModuleDef gnubgmodule = {
    PyModuleDef_HEAD_INIT,
    "_gnubg", 
    "Python bindings for the full GNUBG engine",
    -1,
    GnubgMethods
};

// Initialization function
PyMODINIT_FUNC PyInit__gnubg(void) {
    return PyModule_Create(&gnubgmodule);
}