#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string>

// Include GNUBG headers
// We wrap them in extern "C" because the original project headers are C
extern "C" {
    #include "gnubgmodule.h"
    #include "backgammon.h"   // Defines 'ms' (matchstate), 'msBoard', 'GAME_NONE'
    #include "lib/gnubg-types.h" // Defines 'TanBoard'
}

/* -------------------------------------------------------------------------
 * Helper Functions
 * ------------------------------------------------------------------------- */

/* * Ported from gnubgmodule.c: BoardToPy
 * Converts the internal TanBoard representation to a Python tuple of tuples.
 */
static PyObject *
BoardToPy(const TanBoard anBoard)
{
    PyObject *b = PyTuple_New(2);
    PyObject *b0 = PyTuple_New(25);
    PyObject *b1 = PyTuple_New(25);
    unsigned int k;

    for (k = 0; k < 25; ++k) {
        // In Python 3, PyInt_FromLong is replaced by PyLong_FromLong
        PyTuple_SET_ITEM(b0, k, PyLong_FromLong(anBoard[0][k]));
        PyTuple_SET_ITEM(b1, k, PyLong_FromLong(anBoard[1][k]));
    }

    PyTuple_SET_ITEM(b, 0, b0);
    PyTuple_SET_ITEM(b, 1, b1);

    return b;
}

/* -------------------------------------------------------------------------
 * Python Exposed Functions
 * ------------------------------------------------------------------------- */

/*
 * Ported from gnubgmodule.c: PythonBoard
 * Exposed as: gnubg.board()
 */
static PyObject *
PythonBoard(PyObject * self, PyObject * args)
{
    // :board indicates no arguments are expected
    if (!PyArg_ParseTuple(args, ":board"))
        return NULL;

    // Check if a game is active (ms is the global matchstate defined in backgammon.h)
    if (ms.gs == GAME_NONE) {
        Py_RETURN_NONE;
    }

    // msBoard() is a macro/function in backgammon.h returning the current board
    return BoardToPy(msBoard());
}

/* -------------------------------------------------------------------------
 * Module Registration
 * ------------------------------------------------------------------------- */

// Method table
static PyMethodDef GnubgMethods[] = {
    {"board", PythonBoard, METH_VARARGS,
     "Get the current board\n"
     "    arguments: none\n"
     "    returns: tuple of two lists of 25 ints:\n"
     "        pieces on points 1..24 and the bar"},

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