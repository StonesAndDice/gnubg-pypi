#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

// Include glib and Windows headers before extern "C" so C++/template code in
// them is not parsed with C linkage (fixes MinGW build: template with C
// linkage, mmintrin.h __m64 errors).
#include <glib.h>
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
#include <windows.h>
#include <winsock2.h>
#endif

// Include GNUBG headers; wrap in extern "C" so C symbols link correctly.
extern "C" {
#include "backgammon.h"  // Defines 'ms' (matchstate), 'msBoard', 'GAME_NONE', GetMatchStateCubeInfo
#include "dice.h"       // RollDice, rngCurrent, rngctxCurrent
#include "drawboard.h"  // FormatMove, ParseMove
#include "eval.h"  // Evaluation functions, eq2mwc, mwc2eq, se_eq2mwc, se_mwc2eq
#include "gnubgmodule.h"
#include "lib/gnubg-types.h"  // Defines 'TanBoard'
#include "matchequity.h"      // aafMET, aafMETPostCrawford, MAXSCORE
#include "matchid.h"      // posinfo, cubeinfo, MatchID, MatchIDFromMatchState
#include "multithread.h"  // MT_SafeGet, MT_SafeSet (for command)
#include "output.h"       // foutput_to_mem, szMemOutput (for show)
#include "positionid.h"  // Position ID functions, PositionBearoff, PositionFromBearoff, Combination
}
#include <cstdlib>  // std::getenv, setenv (POSIX)
#include <cerrno>   // errno
#include <climits>  // INT_MIN (for navigate)
#include <csignal>  // SIGINT (for command)
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#include <dlfcn.h>
#endif
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
#include <stdlib.h>  // _putenv_s
#endif

/* -------------------------------------------------------------------------
 * Helper Functions
 * ------------------------------------------------------------------------- */

/* * Ported from gnubgmodule.c: BoardToPy
 * Converts the internal TanBoard representation to a Python tuple of tuples.
 */
static PyObject *BoardToPy(const TanBoard anBoard) {
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

/*
 * Ported from gnubgmodule.c: Board1ToPy
 * Converts a single board (25 points) to a Python tuple.
 */
static PyObject *Board1ToPy(unsigned int anBoard[25]) {
  unsigned int k;
  PyObject *b = PyTuple_New(25);

  for (k = 0; k < 25; ++k) {
    PyTuple_SET_ITEM(b, k, PyLong_FromLong(anBoard[k]));
  }

  return b;
}

/*
 * Ported from gnubgmodule.c: PyToBoard1
 * Converts a Python sequence to a single board (25 points).
 * Returns 1 on success, 0 on failure.
 */
static int PyToBoard1(PyObject *p, unsigned int anBoard[25]) {
  if (PySequence_Check(p) && PySequence_Size(p) == 25) {
    int j;

    for (j = 0; j < 25; ++j) {
      PyObject *pi = PySequence_Fast_GET_ITEM(p, j);
      anBoard[j] = (unsigned int)PyLong_AsLong(pi);
    }
    return 1;
  }

  return 0;
}

/*
 * Ported from gnubgmodule.c: PyToBoard
 * Converts a Python sequence to a TanBoard (2 players x 25 points).
 * Returns 1 on success, 0 on failure.
 */
static int PyToBoard(PyObject *p, TanBoard anBoard) {
  if (PySequence_Check(p) && PySequence_Size(p) == 2) {
    int i;
    for (i = 0; i < 2; ++i) {
      PyObject *py = PySequence_Fast_GET_ITEM(p, i);

      if (!PyToBoard1(py, anBoard[i])) {
        return 0;
      }
    }
    return 1;
  }

  return 0;
}

/*
 * Ported from gnubgmodule.c: PyToMove
 * Converts a Python move tuple to an anMove structure.
 * A tuple can be expressed in two forms:
 * - form returned by findbestmove: (21,18,11,5)
 * - form returned by parsemove: ((21,18),(11,5))
 * Returns 1 on success, 0 on failure.
 */
static int PyToMove(PyObject *p, signed int anMove[8]) {
  Py_ssize_t tuplelen;

  if (!PySequence_Check(p))
    return 0;

  tuplelen = PySequence_Size(p);
  if (tuplelen > 0) {
    int j;
    int anIndex = 0;

    /* Process outer tuple */
    for (j = 0; j < tuplelen && anIndex < 8; ++j) {
      PyObject *pi = PySequence_Fast_GET_ITEM(p, j);
      if (!PySequence_Check(pi)) {
        if (PyLong_Check(pi)) {
          /* Found value like findbestmove returns */
          anMove[anIndex++] = (int)PyLong_AsLong(pi) - 1;
        } else {
          /* Value not an integer */
          return 0;
        }
      } else {
        /* Found inner tuple like parsemove returns */
        if (PySequence_Check(pi) && PySequence_Size(pi) == 2) {
          int k;
          /* Process inner tuple */
          for (k = 0; k < 2 && anIndex < 8; ++k, anIndex++) {
            PyObject *pii = PySequence_Fast_GET_ITEM(pi, k);
            if (PyLong_Check(pii))
              anMove[anIndex] = (int)PyLong_AsLong(pii) - 1;
            else
              /* Value not an integer */
              return 0;
          }
        } else {
          /* Inner tuple doesn't have exactly 2 elements */
          return 0;
        }
      }
    }
    /* If we have found more items than the maximum */
    if (anIndex >= 8 && j < tuplelen)
      return 0;

    return 1;
  } else {
    /* No tuples equivalent to no legal moves */
    return 1;
  }
}

/*
 * Ported from gnubgmodule.c: CubeInfoToPy
 * Converts cubeinfo structure to Python dictionary.
 */
static PyObject *CubeInfoToPy(const cubeinfo *pci) {
  return Py_BuildValue("{s:i,s:i,s:i,s:i,s:(i,i),"
                       "s:i,s:i,s:i,s:((f,f),(f,f)),s:i}",
                       "cube", pci->nCube, "cubeowner", pci->fCubeOwner, "move",
                       pci->fMove, "matchto", pci->nMatchTo, "score",
                       pci->anScore[0], pci->anScore[1], "crawford",
                       pci->fCrawford, "jacoby", pci->fJacoby, "beavers",
                       pci->fBeavers, "gammonprice", pci->arGammonPrice[0],
                       pci->arGammonPrice[2], pci->arGammonPrice[1],
                       pci->arGammonPrice[3], "bgv", pci->bgv);
}

/*
 * Ported from gnubgmodule.c: PosInfoToPy
 * Converts posinfo structure to Python dictionary.
 */
static PyObject *PosInfoToPy(const posinfo *ppi) {
  return Py_BuildValue("{s:(i,i),s:i,s:i,s:i,s:i}", "dice", ppi->anDice[0],
                       ppi->anDice[1], "turn", ppi->fTurn, "resigned",
                       ppi->fResigned, "doubled", ppi->fDoubled, "gamestate",
                       ppi->gs);
}

/*
 * Ported from gnubgmodule.c: EvalContextToPy
 * Converts evalcontext structure to Python dictionary.
 */
static PyObject *EvalContextToPy(const evalcontext *pec) {
  return Py_BuildValue("{s:i,s:i,s:i,s:i,s:f}", "cubeful", pec->fCubeful,
                       "plies", pec->nPlies, "deterministic",
                       pec->fDeterministic, "prune", pec->fUsePrune, "noise",
                       pec->rNoise);
}

/*
 * Ported from gnubgmodule.c: PyToCubeInfo
 * Converts a Python dict (from cubeinfo()) to cubeinfo struct.
 * Returns 0 on success, -1 on error.
 */
static int PyToCubeInfo(PyObject *p, cubeinfo *pci) {
  PyObject *pyKey, *pyValue;
  Py_ssize_t iPos = 0;
  static const char *aszKeys[] = {"jacoby", "crawford",    "move", "beavers",
                                  "cube",   "matchto",     "bgv",  "cubeowner",
                                  "score",  "gammonprice", NULL};
  void *apv[10];
  int *pi;
  float *pf;
  int i = 0;
  apv[i++] = &pci->fJacoby;
  apv[i++] = &pci->fCrawford;
  apv[i++] = &pci->fMove;
  apv[i++] = &pci->fBeavers;
  apv[i++] = &pci->nCube;
  apv[i++] = &pci->nMatchTo;
  apv[i++] = &pci->bgv;
  apv[i++] = &pci->fCubeOwner;
  apv[i++] = pci->anScore;
  apv[i] = pci->arGammonPrice;

  if (!PyDict_Check(p))
    return -1;

  while (PyDict_Next(p, &iPos, &pyKey, &pyValue)) {
    const char *pchKey;
    PyObject *keyBytes = NULL;
    int iKey;

    keyBytes = PyUnicode_AsUTF8String(pyKey);
    if (!keyBytes || !(pchKey = PyBytes_AsString(keyBytes))) {
      Py_XDECREF(keyBytes);
      return -1;
    }

    iKey = -1;
    for (i = 0; aszKeys[i] && iKey < 0; ++i)
      if (strcmp(aszKeys[i], pchKey) == 0)
        iKey = i;
    Py_DECREF(keyBytes);

    if (iKey < 0) {
      PyErr_SetString(
          PyExc_ValueError,
          "invalid key in cubeinfo (see gnubg.cubeinfo() for an example)");
      return -1;
    }

    switch (iKey) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      if (!PyLong_Check(pyValue)) {
        PyErr_SetString(
            PyExc_ValueError,
            "invalid value in cubeinfo (see gnubg.cubeinfo() for an example)");
        return -1;
      }
      *((int *)apv[iKey]) = (int)PyLong_AsLong(pyValue);
      break;
    case 8:
      pi = (int *)apv[iKey];
      if (!PyArg_ParseTuple(pyValue, "ii", pi, pi + 1)) {
        PyErr_SetString(PyExc_ValueError, "invalid score in cubeinfo");
        return -1;
      }
      break;
    case 9:
      pf = (float *)apv[iKey];
      if (!PyArg_ParseTuple(pyValue, "(ff)(ff)", pf, pf + 2, pf + 1, pf + 3)) {
        PyErr_SetString(PyExc_ValueError, "invalid gammonprice in cubeinfo");
        return -1;
      }
      break;
    default:
      break;
    }
  }
  return 0;
}

/*
 * Ported from gnubgmodule.c: PyToPosInfo
 * Converts a Python dict (from posinfo()) to posinfo struct.
 * Returns 0 on success, -1 on error.
 */
static int PyToPosInfo(PyObject *p, posinfo *ppi) {
  PyObject *pyKey, *pyValue;
  Py_ssize_t iPos = 0;
  static const char *aszKeys[] = {"turn",      "resigned", "doubled",
                                  "gamestate", "dice",     NULL};
  void *apv[5];
  int *pi;
  int i = 0;
  apv[i++] = &ppi->fTurn;
  apv[i++] = &ppi->fResigned;
  apv[i++] = &ppi->fDoubled;
  apv[i++] = &ppi->gs;
  apv[i] = ppi->anDice;

  if (!PyDict_Check(p))
    return -1;

  while (PyDict_Next(p, &iPos, &pyKey, &pyValue)) {
    const char *pchKey;
    PyObject *keyBytes = NULL;
    int iKey;

    keyBytes = PyUnicode_AsUTF8String(pyKey);
    if (!keyBytes || !(pchKey = PyBytes_AsString(keyBytes))) {
      Py_XDECREF(keyBytes);
      return -1;
    }

    iKey = -1;
    for (i = 0; aszKeys[i] && iKey < 0; ++i)
      if (strcmp(aszKeys[i], pchKey) == 0)
        iKey = i;
    Py_DECREF(keyBytes);

    if (iKey < 0) {
      PyErr_SetString(
          PyExc_ValueError,
          "invalid key in posinfo (see gnubg.posinfo() for an example)");
      return -1;
    }

    switch (iKey) {
    case 0:
    case 1:
    case 2:
      if (!PyLong_Check(pyValue)) {
        PyErr_SetString(
            PyExc_ValueError,
            "invalid value in posinfo (see gnubg.posinfo() for an example)");
        return -1;
      }
      *((int *)apv[iKey]) = (int)PyLong_AsLong(pyValue);
      break;
    case 3:
      if (!PyLong_Check(pyValue)) {
        PyErr_SetString(
            PyExc_ValueError,
            "invalid value in posinfo (see gnubg.posinfo() for an example)");
        return -1;
      }
      ppi->gs = (gamestate)PyLong_AsLong(pyValue);
      break;
    case 4:
      pi = (int *)apv[iKey];
      if (!PyArg_ParseTuple(pyValue, "ii", pi, pi + 1)) {
        PyErr_SetString(PyExc_ValueError, "invalid dice in posinfo");
        return -1;
      }
      break;
    default:
      break;
    }
  }
  return 0;
}

/*
 * Ported from gnubgmodule.c: METRow
 * Build Python list from one row of MET (n floats).
 */
static PyObject *METRow(const float ar[MAXSCORE], const int n) {
  PyObject *pyList = PyList_New(n);
  if (!pyList)
    return NULL;
  for (int i = 0; i < n; ++i) {
    PyObject *pyf = PyFloat_FromDouble((double)ar[i]);
    if (!pyf) {
      Py_DECREF(pyList);
      return NULL;
    }
    if (PyList_SetItem(pyList, i, pyf) < 0) {
      Py_DECREF(pyList);
      return NULL;
    }
  }
  return pyList;
}

/*
 * Ported from gnubgmodule.c: METPre
 * Build Python list of lists for pre-Crawford MET (n x n).
 */
static PyObject *METPre(const float aar[MAXSCORE][MAXSCORE], const int n) {
  PyObject *pyList = PyList_New(n);
  if (!pyList)
    return NULL;
  for (int i = 0; i < n; ++i) {
    PyObject *pyRow = METRow(aar[i], n);
    if (!pyRow) {
      Py_DECREF(pyList);
      return NULL;
    }
    if (PyList_SetItem(pyList, i, pyRow) < 0) {
      Py_DECREF(pyList);
      return NULL;
    }
  }
  return pyList;
}

/*
 * Ported from gnubgmodule.c: MoveFilterToPy, MoveFiltersToPy, PyToMoveFilter,
 * PyToMoveFilters For getevalhintfilter / setevalhintfilter.
 */
static PyObject *MoveFilterToPy(int ply, int level, const movefilter *pmf) {
  return Py_BuildValue("{s:i,s:i,s:i,s:i,s:f}", "ply", ply, "level", level,
                       "acceptmoves", pmf->Accept, "extramoves", pmf->Extra,
                       "threshold", (double)pmf->Threshold);
}

static PyObject *
MoveFiltersToPy(const movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]) {
  PyObject *pyList = PyList_New(0);
  if (!pyList)
    return NULL;
  for (int ply = 0; ply < MAX_FILTER_PLIES; ++ply)
    for (int level = 0; level <= ply; ++level) {
      PyObject *pyFilter = MoveFilterToPy(ply + 1, level, &aamf[ply][level]);
      if (!pyFilter) {
        Py_DECREF(pyList);
        return NULL;
      }
      if (PyList_Append(pyList, pyFilter) < 0) {
        Py_DECREF(pyFilter);
        Py_DECREF(pyList);
        return NULL;
      }
      Py_DECREF(pyFilter);
    }
  return pyList;
}

static int PyToMoveFilter(PyObject *p, movefilter *pmf, int *ply, int *level) {
  PyObject *pyKey, *pyValue;
  Py_ssize_t iPos = 0;
  static const char *aszKeys[] = {"ply",        "level",     "acceptmoves",
                                  "extramoves", "threshold", NULL};
  void *apv[5];
  int i = 0;
  apv[i++] = ply;
  apv[i++] = level;
  apv[i++] = &pmf->Accept;
  apv[i++] = &pmf->Extra;
  apv[i] = &pmf->Threshold;

  if (!PyDict_Check(p))
    return -1;

  while (PyDict_Next(p, &iPos, &pyKey, &pyValue)) {
    const char *pchKey;
    PyObject *keyBytes = PyUnicode_AsUTF8String(pyKey);
    if (!keyBytes || !(pchKey = PyBytes_AsString(keyBytes))) {
      Py_XDECREF(keyBytes);
      return -1;
    }
    i = -1;
    for (int k = 0; aszKeys[k] && i < 0; ++k)
      if (strcmp(aszKeys[k], pchKey) == 0)
        i = k;
    Py_DECREF(keyBytes);
    if (i < 0) {
      PyErr_SetString(PyExc_ValueError,
                      "invalid key in movefilter (see "
                      "gnubg.getevalhintfilter() for an example)");
      return -1;
    }
    switch (i) {
    case 0:
    case 1:
    case 2:
    case 3:
      if (!PyLong_Check(pyValue)) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid value in movefilter (see "
                        "gnubg.getevalhintfilter() for an example)");
        return -1;
      }
      *((int *)apv[i]) = (int)PyLong_AsLong(pyValue);
      break;
    case 4:
      if (!PyFloat_Check(pyValue)) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid value in movefilter (see "
                        "gnubg.getevalhintfilter() for an example)");
        return -1;
      }
      pmf->Threshold = (float)PyFloat_AsDouble(pyValue);
      break;
    default:
      break;
    }
  }
  return 0;
}

/*
 * Ported from gnubgmodule.c: RolloutContextToPy
 * For hint() rollout details.
 */
static PyObject *RolloutContextToPy(const rolloutcontext *rc) {
  return Py_BuildValue(
      "{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,"
      "s:i,s:i,s:i,s:i,s:i,s:i,s:f,s:f}",
      "cubeful", rc->fCubeful, "variance-reduction", rc->fVarRedn,
      "initial-position", rc->fInitial, "quasi-random-dice", rc->fRotate,
      "late-eval", rc->fLateEvals, "truncated-rollouts", rc->fDoTruncate,
      "n-truncation", rc->nTruncate, "truncate-bearoff2", rc->fTruncBearoff2,
      "truncate-bearoffOS", rc->fTruncBearoffOS, "stop-on-std", rc->fStopOnSTD,
      "trials", rc->nTrials, "seed", (int)rc->nSeed, "minimum-games",
      rc->nMinimumGames, "stop-on-jsd", rc->fStopOnJsd, "late-on-move-n",
      rc->nLate, "minimum-jsd-games", rc->nMinimumJsdGames, "std-limit",
      (double)rc->rStdLimit, "jsd-limit", (double)rc->rJsdLimit);
}

/*
 * Exposed as: gnubg.rolloutcontext([fCubeful, fVarRedn, ...])
 * Make a rollout context dict from optional 16 ints + 2 floats; defaults when
 * omitted.
 */
static PyObject *PythonRolloutContext(PyObject *self, PyObject *args) {
  rolloutcontext rc;
  int fCubeful = 1, fVarRedn = 1, fInitial = 0, fRotate = 1, fLateEvals = 0,
      fDoTruncate = 0;
  int nTruncate = 10, fTruncBearoff2 = 1, fTruncBearoffOS = 1, fStopOnSTD = 0;
  int nTrials = 1296, nSeed = 0, nMinimumGames = 324, fStopOnJsd = 0, nLate = 5,
      nMinimumJsdGames = 324;
  float rStdLimit = 0.01f, rJsdLimit = 2.33f;

  (void)self;
  memset(&rc, 0, sizeof(rc));
  if (!PyArg_ParseTuple(
          args, "|iiiiiiiiiiiiiiiiff", &fCubeful, &fVarRedn, &fInitial,
          &fRotate, &fLateEvals, &fDoTruncate, &nTruncate, &fTruncBearoff2,
          &fTruncBearoffOS, &fStopOnSTD, &nTrials, &nSeed, &nMinimumGames,
          &fStopOnJsd, &nLate, &nMinimumJsdGames, &rStdLimit, &rJsdLimit))
    return NULL;

  rc.fCubeful = fCubeful ? 1 : 0;
  rc.fVarRedn = fVarRedn ? 1 : 0;
  rc.fInitial = fInitial ? 1 : 0;
  rc.fRotate = fRotate ? 1 : 0;
  rc.fLateEvals = fLateEvals ? 1 : 0;
  rc.fDoTruncate = fDoTruncate ? 1 : 0;
  rc.nTruncate = (unsigned short)nTruncate;
  rc.fTruncBearoff2 = fTruncBearoff2 ? 1 : 0;
  rc.fTruncBearoffOS = fTruncBearoffOS ? 1 : 0;
  rc.fStopOnSTD = fStopOnSTD ? 1 : 0;
  rc.nTrials = nTrials;
  rc.nSeed = (unsigned long)nSeed;
  rc.nMinimumGames = nMinimumGames;
  rc.fStopOnJsd = fStopOnJsd ? 1 : 0;
  rc.nLate = (unsigned short)nLate;
  rc.nMinimumJsdGames = nMinimumJsdGames;
  rc.rStdLimit = rStdLimit;
  rc.rJsdLimit = rJsdLimit;

  return RolloutContextToPy(&rc);
}

/* Convert Python (d0, d1) to int anDice[2] (values 1-6). Returns 1 on success,
 * 0 on failure. */
static int PyToDice(PyObject *p, int anDice[2]) {
  if (PySequence_Check(p) && PySequence_Size(p) == 2) {
    for (int j = 0; j < 2; ++j) {
      PyObject *pi = PySequence_GetItem(p, j);
      if (!pi)
        return 0;
      anDice[j] = (int)PyLong_AsLong(pi);
      Py_DECREF(pi);
    }
    return 1;
  }
  return 0;
}

static int
PyToMoveFilters(PyObject *p,
                movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]) {
  movefilter newFilter;
  movefilter defaultFilter = {-1, 0, 0.0f};
  int iPly, iLevel;
  movefilter aamftmp[MAX_FILTER_PLIES][MAX_FILTER_PLIES];

  if (!PySequence_Check(p)) {
    PyErr_SetString(PyExc_ValueError,
                    "invalid movefilter list (see gnubg.getevalhintfilter() "
                    "for an example)");
    return -1;
  }
  for (int i = 0; i < MAX_FILTER_PLIES; ++i)
    for (int j = 0; j < MAX_FILTER_PLIES; ++j)
      aamftmp[i][j] = defaultFilter;

  Py_ssize_t iSeqLen = PySequence_Size(p);
  for (Py_ssize_t entry = 0; entry < iSeqLen; ++entry) {
    PyObject *pObj = PySequence_GetItem(p, entry);
    if (!pObj)
      return -1;
    iPly = iLevel = 0;
    if (PyToMoveFilter(pObj, &newFilter, &iPly, &iLevel) != 0) {
      Py_DECREF(pObj);
      PyErr_SetString(PyExc_ValueError,
                      "invalid movefilter in list (see "
                      "gnubg.getevalhintfilter() for an example)");
      return -1;
    }
    Py_DECREF(pObj);
    if ((iPly < 1 || iPly > MAX_FILTER_PLIES) ||
        (iLevel < 0 || iLevel >= iPly)) {
      PyErr_SetString(PyExc_ValueError,
                      "invalid movefilter list (see gnubg.getevalhintfilter() "
                      "for an example)");
      return -1;
    }
    aamftmp[iPly - 1][iLevel] = newFilter;
  }
  memcpy(aamf, aamftmp, sizeof(aamftmp));
  return 0;
}

/* -------------------------------------------------------------------------
 * Python Exposed Functions
 * ------------------------------------------------------------------------- */

/*
 * Ported from gnubgmodule.c: PythonBoard
 * Exposed as: gnubg.board()
 */
static PyObject *PythonBoard(PyObject *self, PyObject *args) {
  // :board indicates no arguments are expected
  if (!PyArg_ParseTuple(args, ":board"))
    return NULL;

  // Check if a game is active (ms is the global matchstate defined in
  // backgammon.h)
  if (ms.gs == GAME_NONE) {
    Py_RETURN_NONE;
  }

  // msBoard() is a macro/function in backgammon.h returning the current board
  return BoardToPy(msBoard());
}

/*
 * Ported from gnubgmodule.c: PythonPositionID
 * Exposed as: gnubg.positionid(board)
 * Returns position ID string from board.
 */
static PyObject *PythonPositionID(PyObject *self, PyObject *args) {
  PyObject *pyBoard = NULL;
  TanBoard anBoard;

  memcpy(anBoard, msBoard(), sizeof(TanBoard));

  if (!PyArg_ParseTuple(args, "|O:positionid", &pyBoard))
    return NULL;

  if (pyBoard && !PyToBoard(pyBoard, anBoard)) {
    PyErr_SetString(PyExc_TypeError, "Invalid board format");
    return NULL;
  }

  return PyUnicode_FromString(PositionID((ConstTanBoard)anBoard));
}

/*
 * Ported from gnubgmodule.c: PythonPositionFromID
 * Exposed as: gnubg.positionfromid(id)
 * Returns board from position ID string.
 */
static PyObject *PythonPositionFromID(PyObject *self, PyObject *args) {
  char *sz = NULL;
  TanBoard anBoard;

  if (!PyArg_ParseTuple(args, "|s:positionfromid", &sz)) {
    return NULL;
  }

  if (sz) {
    if (!PositionFromID(anBoard, sz)) {
      PyErr_SetString(PyExc_ValueError, "Invalid position ID");
      return NULL;
    }
  } else {
    memcpy(anBoard, msBoard(), sizeof(TanBoard));
  }

  return BoardToPy((ConstTanBoard)anBoard);
}

/*
 * Ported from gnubgmodule.c: PythonPositionKey
 * Exposed as: gnubg.positionkey(board)
 * Returns position key as tuple of 10 ints.
 */
static PyObject *PythonPositionKey(PyObject *self, PyObject *args) {
  PyObject *pyBoard = NULL;
  TanBoard anBoard;
  oldpositionkey key;

  memcpy(anBoard, msBoard(), sizeof(TanBoard));

  if (!PyArg_ParseTuple(args, "|O:positionkey", &pyBoard))
    return NULL;

  if (pyBoard && !PyToBoard(pyBoard, anBoard)) {
    PyErr_SetString(PyExc_TypeError, "Invalid board format");
    return NULL;
  }

  oldPositionKey((ConstTanBoard)anBoard, &key);

  PyObject *a = PyTuple_New(10);
  for (int i = 0; i < 10; ++i) {
    PyTuple_SET_ITEM(a, i, PyLong_FromLong(key.auch[i]));
  }
  return a;
}

/*
 * Ported from gnubgmodule.c: PythonPositionFromKey
 * Exposed as: gnubg.positionfromkey(key)
 * Returns board from position key (tuple of 10 ints).
 */
static PyObject *PythonPositionFromKey(PyObject *self, PyObject *args) {
  TanBoard anBoard;
  PyObject *pyKey = NULL;
  oldpositionkey key;

  if (!PyArg_ParseTuple(args, "|O:positionfromkey", &pyKey))
    return NULL;

  if (pyKey) {
    if (!PySequence_Check(pyKey) || PySequence_Size(pyKey) != 10) {
      PyErr_SetString(PyExc_TypeError, "Key must be a sequence of 10 integers");
      return NULL;
    }

    for (int i = 0; i < 10; ++i) {
      PyObject *py = PySequence_GetItem(pyKey, i);
      if (!py) {
        return NULL;
      }
      key.auch[i] = (unsigned char)PyLong_AsLong(py);
      Py_DECREF(py);
    }
  } else {
    for (int i = 0; i < 10; ++i)
      key.auch[i] = 0;
  }

  oldPositionFromKey(anBoard, &key);

  return BoardToPy((ConstTanBoard)anBoard);
}

/*
 * Ported from gnubgmodule.c: PythonCubeInfo
 * Exposed as: gnubg.cubeinfo(...)
 * Creates a cube info dictionary.
 */
static PyObject *PythonCubeInfo(PyObject *self, PyObject *args) {
  cubeinfo ci;
  // Default values for money game when no arguments provided
  int nCube = 1;
  int fCubeOwner = -1;  // Centered cube
  int fMove = 0;        // Player 0 to move
  int nMatchTo = 0;     // Money game
  int anScore[2] = {0, 0};
  int fCrawford = 0;
  int fJacobyRule = 1;  // Jacoby rule enabled by default
  int fBeavers = 0;
  bgvariation bgv = VARIATION_STANDARD;

  // Use matchstate values if available and valid, otherwise use defaults
  if (ms.fMove >= 0 && ms.fMove <= 1) {
    fMove = ms.fMove;
  }
  if (ms.fCubeOwner >= -1 && ms.fCubeOwner <= 1) {
    fCubeOwner = ms.fCubeOwner;
  }
  if (ms.nCube >= 1) {
    nCube = ms.nCube;
  }
  if (ms.nMatchTo >= 0) {
    nMatchTo = ms.nMatchTo;
    anScore[0] = ms.anScore[0];
    anScore[1] = ms.anScore[1];
    fCrawford = ms.fCrawford;
  }
  if (ms.bgv >= 0) {
    bgv = ms.bgv;
  }

  if (!PyArg_ParseTuple(args, "|iiii(ii)iiii:cubeinfo", &nCube, &fCubeOwner,
                        &fMove, &nMatchTo, &anScore[0], &anScore[1], &fCrawford,
                        &bgv))
    return NULL;

  if (SetCubeInfo(&ci, nCube, fCubeOwner, fMove, nMatchTo, anScore, fCrawford,
                  fJacobyRule, fBeavers, bgv)) {
    PyErr_SetString(PyExc_RuntimeError, "Error in SetCubeInfo");
    return NULL;
  }

  return CubeInfoToPy(&ci);
}

/*
 * Ported from gnubgmodule.c: PythonPosInfo
 * Exposed as: gnubg.posinfo(...)
 * Creates a position info dictionary.
 */
static PyObject *PythonPosInfo(PyObject *self, PyObject *args) {
  posinfo pi;
  // Default values when no arguments provided
  int fTurn = 0;                // Player 0's turn
  int fResigned = 0;            // Not resigned
  int fDoubled = 0;             // Not doubled
  gamestate gs = GAME_PLAYING;  // Playing state
  int anDice[2] = {0, 0};       // No dice rolled yet

  // Use matchstate values if available and valid, otherwise use defaults
  if (ms.fTurn >= 0 && ms.fTurn <= 1) {
    fTurn = ms.fTurn;
  }
  if (ms.fResigned >= 0 && ms.fResigned <= 3) {
    fResigned = ms.fResigned;
  }
  if (ms.fDoubled >= 0 && ms.fDoubled <= 1) {
    fDoubled = ms.fDoubled;
  }
  if (ms.gs >= GAME_NONE && ms.gs <= GAME_DROP) {
    gs = ms.gs;
  }
  if (ms.anDice[0] >= 0 && ms.anDice[0] <= 6 && ms.anDice[1] >= 0 &&
      ms.anDice[1] <= 6) {
    anDice[0] = ms.anDice[0];
    anDice[1] = ms.anDice[1];
  }

  if (!PyArg_ParseTuple(args, "|iiii(ii):posinfo", &fTurn, &fResigned,
                        &fDoubled, &gs, &anDice[0], &anDice[1]))
    return NULL;

  // Validate inputs
  if (fTurn < 0 || fTurn > 1 || fResigned < 0 || fResigned > 3 ||
      fDoubled < 0 || fDoubled > 1 || anDice[0] > 6 || anDice[0] < 0 ||
      anDice[1] > 6 || anDice[1] < 0 || gs > GAME_DROP) {
    PyErr_SetString(PyExc_ValueError, "Invalid posinfo parameters");
    return NULL;
  }

  pi.fTurn = fTurn;
  pi.fResigned = fResigned;
  pi.fDoubled = fDoubled;
  pi.gs = gs;
  pi.anDice[0] = anDice[0];
  pi.anDice[1] = anDice[1];

  return PosInfoToPy(&pi);
}

/*
 * Ported from gnubgmodule.c: PythonEvalContext
 * Exposed as: gnubg.evalcontext(...)
 * Creates an evaluation context dictionary.
 */
static PyObject *PythonEvalContext(PyObject *self, PyObject *args) {
  evalcontext ec;
  int fCubeful = 0;
  int nPlies = 0;
  int fDeterministic = 1;
  int fUsePrune = 0;
  float rNoise = 0.0f;

  if (!PyArg_ParseTuple(args, "|iiiif:evalcontext", &fCubeful, &nPlies,
                        &fDeterministic, &fUsePrune, &rNoise))
    return NULL;

  ec.fCubeful = fCubeful ? 1 : 0;
  ec.nPlies = (nPlies < 8) ? nPlies : 7;
  ec.fDeterministic = fDeterministic ? 1 : 0;
  ec.fUsePrune = fUsePrune ? 1 : 0;
  ec.rNoise = rNoise;

  return EvalContextToPy(&ec);
}

/* Convert Python dict (from evalcontext()) to evalcontext. Returns 0 on
 * success, -1 on error. */
static int PyToEvalContext(PyObject *p, evalcontext *pec) {
  PyObject *pyKey, *pyValue;
  Py_ssize_t iPos = 0;
  static const char *aszKeys[] = {"cubeful", "plies", "deterministic",
                                  "prune",   "noise", NULL};

  if (!PyDict_Check(p)) {
    PyErr_SetString(PyExc_TypeError,
                    "evalcontext must be a dict (see gnubg.evalcontext())");
    return -1;
  }

  while (PyDict_Next(p, &iPos, &pyKey, &pyValue)) {
    PyObject *utf8 = NULL;
    const char *pchKey = NULL;
    int iKey = -1;

    utf8 = PyUnicode_AsUTF8String(pyKey);
    if (!utf8)
      return -1;
    pchKey = PyBytes_AsString(utf8);
    if (!pchKey) {
      Py_DECREF(utf8);
      return -1;
    }

    for (int i = 0; aszKeys[i]; ++i)
      if (strcmp(aszKeys[i], pchKey) == 0) {
        iKey = i;
        break;
      }
    Py_DECREF(utf8);

    if (iKey < 0) {
      PyErr_SetString(PyExc_ValueError,
                      "invalid key in evalcontext (see gnubg.evalcontext())");
      return -1;
    }

    switch (iKey) {
    case 0: /* cubeful */
    case 1: /* plies */
    case 2: /* deterministic */
    case 3: /* prune */
      if (!PyLong_Check(pyValue)) {
        PyErr_SetString(
            PyExc_ValueError,
            "invalid value in evalcontext (see gnubg.evalcontext())");
        return -1;
      }
      {
        long i = PyLong_AsLong(pyValue);
        if (iKey == 0)
          pec->fCubeful = i ? 1 : 0;
        else if (iKey == 1)
          pec->nPlies = (i < 8) ? (unsigned int)i : 7;
        else if (iKey == 2)
          pec->fDeterministic = i ? 1 : 0;
        else
          pec->fUsePrune = i ? 1 : 0;
      }
      break;
    case 4: /* noise */
      if (!PyFloat_Check(pyValue)) {
        PyErr_SetString(
            PyExc_ValueError,
            "invalid value in evalcontext (see gnubg.evalcontext())");
        return -1;
      }
      pec->rNoise = (float)PyFloat_AsDouble(pyValue);
      break;
    default:
      break;
    }
  }
  return 0;
}

/*
 * Exposed as: gnubg.evaluate([board], [cubeinfo], [evalcontext])
 * Evaluate position; returns tuple of 6 floats (win, wingammon, winbackgammon,
 * losegammon, losebackgammon, equity).
 */
static PyObject *PythonEvaluate(PyObject *self, PyObject *args) {
  PyObject *pyBoard = NULL;
  PyObject *pyCubeInfo = NULL;
  PyObject *pyEvalContext = NULL;
  TanBoard anBoard;
  cubeinfo ci;
  evalcontext ec;
  float arOutput[NUM_ROLLOUT_OUTPUTS];

  (void)self;
  memcpy(anBoard, msBoard(), sizeof(TanBoard));
  GetMatchStateCubeInfo(&ci, &ms);
  memcpy(&ec, &ecBasic, sizeof(evalcontext));

  if (!PyArg_ParseTuple(args, "|OOO:evaluate", &pyBoard, &pyCubeInfo,
                        &pyEvalContext))
    return NULL;

  if (pyBoard && !PyToBoard(pyBoard, anBoard)) {
    PyErr_SetString(PyExc_TypeError, "Invalid board format");
    return NULL;
  }
  if (pyCubeInfo && PyToCubeInfo(pyCubeInfo, &ci) != 0)
    return NULL;
  if (pyEvalContext && PyToEvalContext(pyEvalContext, &ec) != 0)
    return NULL;

  if (GeneralEvaluationE(arOutput, (ConstTanBoard)anBoard, &ci, &ec) < 0) {
    PyErr_SetString(PyExc_RuntimeError, "EvaluatePosition failed");
    return NULL;
  }

  return Py_BuildValue("(ffffff)", (double)arOutput[0], (double)arOutput[1],
                       (double)arOutput[2], (double)arOutput[3],
                       (double)arOutput[4], (double)arOutput[5]);
}

/*
 * Exposed as: gnubg.findbestmove([board], [cubeinfo], [evalcontext], [dice],
 * [movefilters]) Find best move for the given dice; returns tuple of (from, to,
 * ...) 1-based, or empty tuple if no move.
 */
static PyObject *PythonFindBestMove(PyObject *self, PyObject *args) {
  PyObject *pyBoard = NULL;
  PyObject *pyCubeInfo = NULL;
  PyObject *pyEvalContext = NULL;
  PyObject *pyDice = NULL;
  PyObject *pyMoveFilters = NULL;
  TanBoard anBoard;
  cubeinfo ci;
  evalcontext ec;
  movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
  int anMove[8];
  int anDice[2] = {0, 0};

  (void)self;
  memcpy(anBoard, msBoard(), sizeof(TanBoard));
  GetMatchStateCubeInfo(&ci, &ms);
  memcpy(&ec, &ecBasic, sizeof(evalcontext));
  memcpy(aamf, defaultFilters, sizeof(aamf));

  if (!PyArg_ParseTuple(args, "|OOOOO:findbestmove", &pyBoard, &pyCubeInfo,
                        &pyEvalContext, &pyDice, &pyMoveFilters))
    return NULL;

  if (pyDice && !PyToDice(pyDice, anDice)) {
    PyErr_SetString(PyExc_TypeError,
                    "dice must be a sequence of 2 integers (1-6)");
    return NULL;
  }
  if (!pyDice && ms.gs == GAME_PLAYING && ms.anDice[0] >= 1 &&
      ms.anDice[0] <= 6 && ms.anDice[1] >= 1 && ms.anDice[1] <= 6) {
    anDice[0] = ms.anDice[0];
    anDice[1] = ms.anDice[1];
  }
  if (anDice[0] < 1 || anDice[0] > 6 || anDice[1] < 1 || anDice[1] > 6) {
    PyErr_SetString(PyExc_ValueError,
                    "dice required: provide (die1, die2) with values 1-6");
    return NULL;
  }

  if (pyBoard && !PyToBoard(pyBoard, anBoard)) {
    PyErr_SetString(PyExc_TypeError, "Invalid board format");
    return NULL;
  }
  if (pyCubeInfo && PyToCubeInfo(pyCubeInfo, &ci) != 0)
    return NULL;
  if (pyEvalContext && PyToEvalContext(pyEvalContext, &ec) != 0)
    return NULL;
  if (pyMoveFilters && PyToMoveFilters(pyMoveFilters, aamf) != 0)
    return NULL;

  if (FindBestMove(anMove, anDice[0], anDice[1], anBoard, &ci, &ec, aamf) < 0) {
    PyErr_SetString(PyExc_RuntimeError, "FindBestMove failed");
    return NULL;
  }

  Py_ssize_t n = 0;
  while (n < 8 && anMove[n] >= 0)
    n++;
  PyObject *p = PyTuple_New(n);
  if (!p)
    return NULL;
  for (Py_ssize_t k = 0; k < n; ++k)
    PyTuple_SET_ITEM(p, k, PyLong_FromLong(anMove[k] + 1));
  return p;
}

/*
 * Exposed as: gnubg.findbestmoves(...)
 * Same args as findbestmove. Returns list of dicts {"move": [(from,to),...], "score": float},
 * ordered by score descending (best first). Best move is moves[0]["move"].
 */
static PyObject *PythonFindBestMoves(PyObject *self, PyObject *args) {
  PyObject *pyBoard = NULL;
  PyObject *pyCubeInfo = NULL;
  PyObject *pyEvalContext = NULL;
  PyObject *pyDice = NULL;
  PyObject *pyMoveFilters = NULL;
  TanBoard anBoard;
  cubeinfo ci;
  evalcontext ec;
  movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
  movelist ml;
  int anDice[2] = {0, 0};

  (void)self;
  memcpy(anBoard, msBoard(), sizeof(TanBoard));
  GetMatchStateCubeInfo(&ci, &ms);
  memcpy(&ec, &ecBasic, sizeof(evalcontext));
  memcpy(aamf, defaultFilters, sizeof(aamf));

  if (!PyArg_ParseTuple(args, "|OOOOO:findbestmoves", &pyBoard, &pyCubeInfo,
                        &pyEvalContext, &pyDice, &pyMoveFilters))
    return NULL;

  if (pyDice && !PyToDice(pyDice, anDice)) {
    PyErr_SetString(PyExc_TypeError,
                    "dice must be a sequence of 2 integers (1-6)");
    return NULL;
  }
  if (!pyDice && ms.gs == GAME_PLAYING && ms.anDice[0] >= 1 &&
      ms.anDice[0] <= 6 && ms.anDice[1] >= 1 && ms.anDice[1] <= 6) {
    anDice[0] = ms.anDice[0];
    anDice[1] = ms.anDice[1];
  }
  if (anDice[0] < 1 || anDice[0] > 6 || anDice[1] < 1 || anDice[1] > 6) {
    PyErr_SetString(PyExc_ValueError,
                    "dice required: provide (die1, die2) with values 1-6");
    return NULL;
  }

  if (pyBoard && !PyToBoard(pyBoard, anBoard)) {
    PyErr_SetString(PyExc_TypeError, "Invalid board format");
    return NULL;
  }
  if (pyCubeInfo && PyToCubeInfo(pyCubeInfo, &ci) != 0)
    return NULL;
  if (pyEvalContext && PyToEvalContext(pyEvalContext, &ec) != 0)
    return NULL;
  if (pyMoveFilters && PyToMoveFilters(pyMoveFilters, aamf) != 0)
    return NULL;

  if (FindnSaveBestMoves(&ml, anDice[0], anDice[1], (ConstTanBoard)anBoard,
                        NULL, 0.0f, &ci, &ec, aamf) < 0) {
    PyErr_SetString(PyExc_RuntimeError, "FindnSaveBestMoves failed");
    return NULL;
  }

  /* Build (score, move_tuple) pairs and sort by score descending */
  std::vector<std::pair<float, std::vector<long>>> scored;
  scored.reserve(ml.cMoves);
  for (unsigned int i = 0; i < ml.cMoves; i++) {
    const move *pm = &ml.amMoves[i];
    std::vector<long> m;
    for (int k = 0; k < 8 && pm->anMove[k] >= 0; k++)
      m.push_back(pm->anMove[k] + 1);
    scored.push_back(std::make_pair(pm->rScore, std::move(m)));
  }
  g_free(ml.amMoves);

  std::sort(scored.begin(), scored.end(),
            [](const std::pair<float, std::vector<long>> &a,
               const std::pair<float, std::vector<long>> &b) {
              return a.first > b.first;
            });

  PyObject *list = PyList_New(0);
  if (!list)
    return NULL;
  for (const auto &p : scored) {
    PyObject *moveTuple = PyTuple_New((Py_ssize_t)p.second.size());
    if (!moveTuple) {
      Py_DECREF(list);
      return NULL;
    }
    for (size_t k = 0; k < p.second.size(); ++k)
      PyTuple_SET_ITEM(moveTuple, (Py_ssize_t)k, PyLong_FromLong(p.second[k]));
    PyObject *dict = Py_BuildValue("{s:O s:f}", "move", moveTuple, "score", p.first);
    Py_DECREF(moveTuple);
    if (!dict) {
      Py_DECREF(list);
      return NULL;
    }
    if (PyList_Append(list, dict) != 0) {
      Py_DECREF(dict);
      Py_DECREF(list);
      return NULL;
    }
    Py_DECREF(dict);
  }
  return list;
}

/*
 * Ported from gnubgmodule.c: PythonClassifyPosition
 * Exposed as: gnubg.classify(board, variant)
 * Classifies a backgammon position.
 */
static PyObject *PythonClassifyPosition(PyObject *self, PyObject *args) {
  PyObject *pyBoard = NULL;
  TanBoard anBoard;
  bgvariation iVariant = ms.bgv;

  memcpy(anBoard, msBoard(), sizeof(TanBoard));

  if (!PyArg_ParseTuple(args, "|Oi:classify", &pyBoard, &iVariant))
    return NULL;

  if (pyBoard && !PyToBoard(pyBoard, anBoard)) {
    PyErr_SetString(PyExc_TypeError, "Invalid board format");
    return NULL;
  }

  if (iVariant >= NUM_VARIATIONS) {
    PyErr_SetString(PyExc_ValueError, "Unknown variation");
    return NULL;
  }

  return PyLong_FromLong(ClassifyPosition((ConstTanBoard)anBoard, iVariant));
}

/*
 * Ported from gnubgmodule.c: PythonLuckRating
 * Exposed as: gnubg.luckrating(luck)
 * Converts luck value to rating (0-5).
 */
static PyObject *PythonLuckRating(PyObject *self, PyObject *args) {
  float r;
  if (!PyArg_ParseTuple(args, "f:luckrating", &r))
    return NULL;

  return PyLong_FromLong(getLuckRating(r));
}

/*
 * Ported from gnubgmodule.c: PythonErrorRating
 * Exposed as: gnubg.errorrating(error)
 * Converts error value to rating (0-7).
 */
static PyObject *PythonErrorRating(PyObject *self, PyObject *args) {
  float r;
  if (!PyArg_ParseTuple(args, "f:errorrating", &r))
    return NULL;

  return PyLong_FromLong(GetRating(r));
}

/*
 * Ported from gnubgmodule.c: PythonParseMove
 * Exposed as: gnubg.parsemove(string)
 * Parses a move string and returns move tuple.
 */
static PyObject *PythonParseMove(PyObject *self, PyObject *args) {
  PyObject *pyMoves;
  char *pch;
  char *sz;
  int an[8];
  int nummoves;
  int movesidx;
  int saved_errno;

  if (!PyArg_ParseTuple(args, "s:parsemove", &pch))
    return NULL;

  sz = g_strdup(pch);
  if (!sz) {
    PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory");
    return NULL;
  }

  // Convert dashes to slashes
  for (char *p = sz; *p; p++)
    if (*p == '-')
      *p = '/';

  saved_errno = errno;
  errno = 0;
  nummoves = ParseMove(sz, an);
  g_free(sz);
  errno = saved_errno;

  if (nummoves < 0) {
    // ParseMove sets errno on error (EINVAL = 22)
    // The original implementation just returns NULL without setting an error,
    // but we need to set an error for Python 3
    // Note: ParseMove validates moves and may reject them for various reasons
    PyErr_SetString(PyExc_ValueError, "Invalid move string");
    return NULL;
  }

  if (!(pyMoves = PyTuple_New(nummoves)))
    return NULL;

  for (movesidx = 0; movesidx < nummoves; movesidx++) {
    PyObject *pyMove;
    PyObject *pyMoveL;
    PyObject *pyMoveR;

    pyMoveL = PyLong_FromLong(an[movesidx * 2]);
    pyMoveR = PyLong_FromLong(an[movesidx * 2 + 1]);

    if (!(pyMove = PyTuple_New(2)))
      return NULL;

    if (PyTuple_SetItem(pyMove, 0, pyMoveL) < 0)
      return NULL;
    if (PyTuple_SetItem(pyMove, 1, pyMoveR) < 0)
      return NULL;

    if (PyTuple_SetItem(pyMoves, movesidx, pyMove) < 0)
      return NULL;
  }

  return pyMoves;
}

/*
 * Ported from gnubgmodule.c: PythonMoveTuple2String
 * Exposed as: gnubg.movetupletostring(move, board)
 * Converts a move tuple to a string representation.
 */
static PyObject *PythonMoveTuple2String(PyObject *self, PyObject *args) {
  PyObject *pyBoard = NULL;
  PyObject *pyMove = NULL;
  char szMove[FORMATEDMOVESIZE];
  signed int anMove[8];
  TanBoard anBoard;

  memset(anBoard, 0, sizeof(TanBoard));
  memset(anMove, -1, sizeof(anMove));

  if (!PyArg_ParseTuple(args, "OO:movetupletostring", &pyMove, &pyBoard))
    return NULL;

  if (!pyMove || !pyBoard) {
    PyErr_SetString(PyExc_TypeError,
                    "Requires 2 arguments (move tuple, board)");
    return NULL;
  }

  if (!PyToMove(pyMove, anMove)) {
    PyErr_SetString(PyExc_TypeError, "Invalid move tuple");
    return NULL;
  }

  if (!PyToBoard(pyBoard, anBoard)) {
    PyErr_SetString(PyExc_TypeError, "Invalid board format");
    return NULL;
  }

  szMove[0] = '\0';
  FormatMove(szMove, (ConstTanBoard)anBoard, anMove);

  return PyUnicode_FromString(szMove);
}

/*
 * Ported from gnubgmodule.c: PythonMET
 * Exposed as: gnubg.met([maxscore])
 * Returns match equity table (list of lists).
 */
static PyObject *PythonMET(PyObject *self, PyObject *args) {
  int n = ms.nMatchTo ? ms.nMatchTo : MAXSCORE;
  if (!PyArg_ParseTuple(args, "|i:met", &n))
    return NULL;
  if (n < 0 || n > MAXSCORE) {
    PyErr_SetString(PyExc_ValueError, "invalid match length");
    return NULL;
  }

  PyObject *pyMET = PyList_New(3);
  if (!pyMET)
    return NULL;

  PyObject *pyList = METPre(aafMET, n);
  if (!pyList) {
    Py_DECREF(pyMET);
    return NULL;
  }
  if (PyList_SetItem(pyMET, 0, pyList) < 0) {
    Py_DECREF(pyMET);
    return NULL;
  }

  for (int i = 0; i < 2; ++i) {
    pyList = METRow(aafMETPostCrawford[i], n);
    if (!pyList) {
      Py_DECREF(pyMET);
      return NULL;
    }
    if (PyList_SetItem(pyMET, i + 1, pyList) < 0) {
      Py_DECREF(pyMET);
      return NULL;
    }
  }
  return pyMET;
}

/*
 * Ported from gnubgmodule.c: PythonMatchID
 * Exposed as: gnubg.matchid([cubeinfo], [posinfo])
 * Returns match ID string.
 */
static PyObject *PythonMatchID(PyObject *self, PyObject *args) {
  PyObject *pyCubeInfo = NULL;
  PyObject *pyPosInfo = NULL;
  cubeinfo ci = {1,
                 0,
                 0,
                 0,
                 {0, 0},
                 FALSE,
                 TRUE,
                 FALSE,
                 {1.0f, 1.0f, 1.0f, 1.0f},
                 VARIATION_STANDARD};
  posinfo pi = {{0, 0}, 0, 0, 0, GAME_NONE};

  if (!PyArg_ParseTuple(args, "|OO:matchid", &pyCubeInfo, &pyPosInfo))
    return NULL;

  if (!pyCubeInfo && ms.gs == GAME_NONE) {
    PyErr_SetString(PyExc_ValueError, "no current position available");
    return NULL;
  }
  if (pyCubeInfo && PyToCubeInfo(pyCubeInfo, &ci) != 0)
    return NULL;
  if (pyPosInfo && PyToPosInfo(pyPosInfo, &pi) != 0)
    return NULL;
  if (pyCubeInfo && !pyPosInfo) {
    PyErr_SetString(PyExc_TypeError,
                    "matchid: cubeinfo requires posinfo (see gnubg.matchid())");
    return NULL;
  }

  if (!pyCubeInfo)
    return PyUnicode_FromString(MatchIDFromMatchState(&ms));
  return PyUnicode_FromString(MatchID((unsigned int *)pi.anDice, pi.fTurn,
                                      pi.fResigned, pi.fDoubled, ci.fMove,
                                      ci.fCubeOwner, ci.fCrawford, ci.nMatchTo,
                                      ci.anScore, ci.nCube, ci.fJacoby, pi.gs));
}

/*
 * Ported from gnubgmodule.c: PythonGnubgID
 * Exposed as: gnubg.gnubgid([board], [cubeinfo], [posinfo])
 * Returns GNUbgID string (positionid:matchid).
 */
static PyObject *PythonGnubgID(PyObject *self, PyObject *args) {
  PyObject *pyBoard = NULL;
  PyObject *pyCubeInfo = NULL;
  PyObject *pyPosInfo = NULL;
  TanBoard anBoard;
  cubeinfo ci;
  posinfo pi;
  char *szPosID = NULL;
  char *szMatchID = NULL;
  char *szGnubgID = NULL;
  PyObject *pyRetVal = NULL;

  memcpy(anBoard, msBoard(), sizeof(TanBoard));
  pi.anDice[0] = ms.anDice[0];
  pi.anDice[1] = ms.anDice[1];
  pi.fTurn = ms.fTurn;
  pi.fResigned = ms.fResigned;
  pi.fDoubled = ms.fDoubled;
  pi.gs = ms.gs;
  GetMatchStateCubeInfo(&ci, &ms);

  if (!PyArg_ParseTuple(args, "|OOO:gnubgid", &pyBoard, &pyCubeInfo,
                        &pyPosInfo))
    return NULL;

  if (pyBoard && (!pyPosInfo || !pyCubeInfo)) {
    PyErr_SetString(
        PyExc_TypeError,
        "gnubgid requires 0 or exactly 3 arguments (board, cubeinfo, posinfo). "
        "See gnubg.board(), gnubg.cubeinfo(), gnubg.posinfo().");
    return NULL;
  }
  if (pyBoard && !PyToBoard(pyBoard, anBoard)) {
    PyErr_SetString(PyExc_TypeError, "Invalid board format");
    return NULL;
  }
  if (!pyBoard && ms.gs == GAME_NONE) {
    PyErr_SetString(PyExc_ValueError, "no current position available");
    return NULL;
  }
  if (pyCubeInfo && PyToCubeInfo(pyCubeInfo, &ci) != 0)
    return NULL;
  if (pyPosInfo && PyToPosInfo(pyPosInfo, &pi) != 0)
    return NULL;

  szPosID = g_strdup(PositionID((ConstTanBoard)anBoard));
  szMatchID =
      g_strdup(MatchID((unsigned int *)pi.anDice, pi.fTurn, pi.fResigned,
                       pi.fDoubled, ci.fMove, ci.fCubeOwner, ci.fCrawford,
                       ci.nMatchTo, ci.anScore, ci.nCube, ci.fJacoby, pi.gs));
  szGnubgID = g_strjoin(":", szPosID, szMatchID, NULL);
  pyRetVal = PyUnicode_FromString(szGnubgID);
  g_free(szPosID);
  g_free(szMatchID);
  g_free(szGnubgID);
  return pyRetVal;
}

/*
 * Ported from gnubgmodule.c: PythonDiceRolls
 * Exposed as: gnubg.dicerolls(n)
 * Returns list of n dice rolls as tuples (die1, die2).
 */
static PyObject *PythonDiceRolls(PyObject *self, PyObject *args) {
  long n;
  unsigned int anDice[2];
  if (!PyArg_ParseTuple(args, "l:dicerolls", &n))
    return NULL;
  if (n <= 0) {
    PyErr_SetString(PyExc_ValueError, "number of rolls must be greater than 0");
    return NULL;
  }

  PyObject *pyDiceRolls = PyTuple_New((Py_ssize_t)n);
  if (!pyDiceRolls)
    return NULL;

  for (Py_ssize_t idx = 0; idx < n; ++idx) {
    RollDice(anDice, &rngCurrent, rngctxCurrent);
    PyObject *pyDie1 = PyLong_FromLong(anDice[0]);
    PyObject *pyDie2 = PyLong_FromLong(anDice[1]);
    if (!pyDie1 || !pyDie2) {
      Py_XDECREF(pyDie1);
      Py_XDECREF(pyDie2);
      Py_DECREF(pyDiceRolls);
      return NULL;
    }
    PyObject *pyDiceRoll = PyTuple_Pack(2, pyDie1, pyDie2);
    Py_DECREF(pyDie1);
    Py_DECREF(pyDie2);
    if (!pyDiceRoll) {
      Py_DECREF(pyDiceRolls);
      return NULL;
    }
    if (PyTuple_SetItem(pyDiceRolls, idx, pyDiceRoll) < 0) {
      Py_DECREF(pyDiceRolls);
      return NULL;
    }
  }
  return pyDiceRolls;
}

/*
 * Ported from gnubgmodule.c: PythonMatchChecksum
 * Exposed as: gnubg.matchchecksum()
 * Returns MD5 checksum of current match as 32-char hex string.
 */
static PyObject *PythonMatchChecksum(PyObject *self, PyObject *args) {
  if (!PyArg_ParseTuple(args, ":matchchecksum"))
    return NULL;
  return PyUnicode_FromString(GetMatchCheckSum());
}

/*
 * Ported from gnubgmodule.c: PythonPositionBearoff
 * Exposed as: gnubg.positionbearoff([board], [nPoints], [nChequers])
 * Returns bearoff id for the given position (one side of board).
 */
static PyObject *PythonPositionBearoff(PyObject *self, PyObject *args) {
  PyObject *pyBoard = NULL;
  int nChequers = 15;
  int nPoints = 6;
  TanBoard anBoard;
  memset(anBoard, 0, sizeof(anBoard));
  memcpy(anBoard, msBoard(), sizeof(TanBoard));

  if (!PyArg_ParseTuple(args, "|Oii:positionbearoff", &pyBoard, &nPoints,
                        &nChequers))
    return NULL;
  if (pyBoard && !PyToBoard1(pyBoard, anBoard[0])) {
    PyErr_SetString(PyExc_TypeError, "Invalid board format");
    return NULL;
  }
  return PyLong_FromLong((long)PositionBearoff(
      anBoard[0], (unsigned int)nPoints, (unsigned int)nChequers));
}

/*
 * Ported from gnubgmodule.c: PythonPositionFromBearoff
 * Exposed as: gnubg.positionfrombearoff([id], [nChequers], [nPoints])
 * Returns single board (25 ints) from bearoff id.
 */
static PyObject *PythonPositionFromBearoff(PyObject *self, PyObject *args) {
  TanBoard anBoard;
  unsigned int iPos = 0;
  int nChequers = 15;
  int nPoints = 6;
  memset(anBoard, 0, sizeof(anBoard));

  if (!PyArg_ParseTuple(args, "|iii:positionfrombearoff", &iPos, &nChequers,
                        &nPoints))
    return NULL;
  if (nChequers < 1 || nChequers > 15 || nPoints < 1 || nPoints > 25) {
    PyErr_SetString(PyExc_ValueError, "invalid number of chequers or points");
    return NULL;
  }

  unsigned int n =
      Combination((unsigned int)(nChequers + nPoints), (unsigned int)nPoints);
  if (iPos >= n) {
    PyErr_SetString(PyExc_ValueError, "invalid position number");
    return NULL;
  }
  PositionFromBearoff(anBoard[0], iPos, (unsigned int)nPoints,
                      (unsigned int)nChequers);
  return Board1ToPy(anBoard[0]);
}

/*
 * Ported from gnubgmodule.c: PythonEq2mwc
 * Exposed as: gnubg.eq2mwc([equity], [cubeinfo])
 * Converts cubeless equity to match-winning chance.
 */
static PyObject *PythonEq2mwc(PyObject *self, PyObject *args) {
  PyObject *pyCubeInfo = NULL;
  float r = 0.0f;
  cubeinfo ci;
  if (!PyArg_ParseTuple(args, "|fO:eq2mwc", &r, &pyCubeInfo))
    return NULL;
  GetMatchStateCubeInfo(&ci, &ms);
  if (pyCubeInfo && PyToCubeInfo(pyCubeInfo, &ci) != 0)
    return NULL;
  return PyFloat_FromDouble((double)eq2mwc(r, &ci));
}

/*
 * Ported from gnubgmodule.c: PythonEq2mwcStdErr
 * Exposed as: gnubg.eq2mwc_stderr([equity], [cubeinfo])
 */
static PyObject *PythonEq2mwcStdErr(PyObject *self, PyObject *args) {
  PyObject *pyCubeInfo = NULL;
  float r = 0.0f;
  cubeinfo ci;
  if (!PyArg_ParseTuple(args, "|fO:eq2mwc_stderr", &r, &pyCubeInfo))
    return NULL;
  GetMatchStateCubeInfo(&ci, &ms);
  if (pyCubeInfo && PyToCubeInfo(pyCubeInfo, &ci) != 0)
    return NULL;
  return PyFloat_FromDouble((double)se_eq2mwc(r, &ci));
}

/*
 * Ported from gnubgmodule.c: PythonMwc2eq
 * Exposed as: gnubg.mwc2eq([mwc], [cubeinfo])
 * Converts match-winning chance to equity.
 */
static PyObject *PythonMwc2eq(PyObject *self, PyObject *args) {
  PyObject *pyCubeInfo = NULL;
  float r = 0.0f;
  cubeinfo ci;
  if (!PyArg_ParseTuple(args, "|fO:mwc2eq", &r, &pyCubeInfo))
    return NULL;
  GetMatchStateCubeInfo(&ci, &ms);
  if (pyCubeInfo && PyToCubeInfo(pyCubeInfo, &ci) != 0)
    return NULL;
  return PyFloat_FromDouble((double)mwc2eq(r, &ci));
}

/*
 * Ported from gnubgmodule.c: PythonMwc2eqStdErr
 * Exposed as: gnubg.mwc2eq_stderr([mwc], [cubeinfo])
 */
static PyObject *PythonMwc2eqStdErr(PyObject *self, PyObject *args) {
  PyObject *pyCubeInfo = NULL;
  float r = 0.0f;
  cubeinfo ci;
  if (!PyArg_ParseTuple(args, "|fO:mwc2eq_stderr", &r, &pyCubeInfo))
    return NULL;
  GetMatchStateCubeInfo(&ci, &ms);
  if (pyCubeInfo && PyToCubeInfo(pyCubeInfo, &ci) != 0)
    return NULL;
  return PyFloat_FromDouble((double)se_mwc2eq(r, &ci));
}

/*
 * Hint callback: called from C (hint_move). Must have C linkage.
 */
extern "C" {
static int PythonHint_Callback(procrecorddata *pr) {
  char szMove[FORMATEDMOVESIZE];
  PyObject *list = (PyObject *)pr->pvUserData;
  PyObject *hintdict = NULL, *ctxdict = NULL, *details = NULL;

  int index = (int)(intptr_t)pr->avOutputData[PROCREC_HINT_ARGOUT_INDEX];
  const matchstate *pms =
      (const matchstate *)pr->avOutputData[PROCREC_HINT_ARGOUT_MATCHSTATE];
  const movelist *pml =
      (const movelist *)pr->avOutputData[PROCREC_HINT_ARGOUT_MOVELIST];
  const evalsetup *pes = &pml->amMoves[index].esMove;
  float rEq = pml->amMoves[index].rScore;
  float rEqTop = pml->amMoves[0].rScore;
  float rEqDiff = rEq - rEqTop;

  FormatMove(szMove, (ConstTanBoard)pms->anBoard, pml->amMoves[index].anMove);

  switch (pes->et) {
  case EVAL_NONE:
    hintdict =
        Py_BuildValue("{s:i,s:s,s:f,s:f}", "movenum", index + 1, "type", "none",
                      "equity", (double)rEq, "eqdiff", (double)rEqDiff);
    break;
  case EVAL_EVAL: {
    const move *pmi = &pml->amMoves[index];
    details =
        Py_BuildValue("{s:(fffff),s:f}", "probs", (double)pmi->arEvalMove[0],
                      (double)pmi->arEvalMove[1], (double)pmi->arEvalMove[2],
                      (double)pmi->arEvalMove[3], (double)pmi->arEvalMove[4],
                      "score", (double)pmi->rScore);
    ctxdict = EvalContextToPy(&pes->ec);
    hintdict = Py_BuildValue("{s:i,s:s,s:s,s:f,s:f,s:N,s:N}", "movenum",
                             index + 1, "type", "eval", "move", szMove,
                             "equity", (double)rEq, "eqdiff", (double)rEqDiff,
                             "context", ctxdict, "details", details);
    break;
  }
  case EVAL_ROLLOUT: {
    const move *pmi = &pml->amMoves[index];
    const float *p = pmi->arEvalMove;
    const float *s = pmi->arEvalStdDev;
    details = Py_BuildValue(
        "{s:(fffff),s:(fffff),s:f,s:f,s:f,s:f,s:i,s:f}", "probs", (double)p[0],
        (double)p[1], (double)p[2], (double)p[3], (double)p[4], "probs-std",
        (double)s[0], (double)s[1], (double)s[2], (double)s[3], (double)s[4],
        "match-eq", (double)p[OUTPUT_EQUITY], "cubeful-eq",
        (double)p[OUTPUT_CUBEFUL_EQUITY], "score", (double)pmi->rScore,
        "score2", (double)pmi->rScore2, "trials", (int)pes->rc.nGamesDone,
        "stopped-on-jsd", (double)pes->rc.rStoppedOnJSD);
    ctxdict = RolloutContextToPy(&pes->rc);
    hintdict = Py_BuildValue("{s:i,s:s,s:s,s:f,s:f,s:N,s:N}", "movenum",
                             index + 1, "type", "rollout", "move", szMove,
                             "equity", (double)rEq, "eqdiff", (double)rEqDiff,
                             "context", ctxdict, "details", details);
    break;
  }
  default:
    hintdict = Py_BuildValue("{s:i,s:s,s:i}", "movenum", index + 1, "type",
                             "unknown", "typenum", (int)pes->et);
    break;
  }
  if (hintdict) {
    PyList_Append(list, hintdict);
    Py_DECREF(hintdict);
  }
  return 1; /* TRUE */
}
}

/*
 * Ported from gnubgmodule.c: PythonHint
 * Exposed as: gnubg.hint([maxmoves])
 */
static PyObject *PythonHint(PyObject *self, PyObject *args) {
  int nMaxMoves = -1;
  char szNumber[11];
  procrecorddata prochint;
  PyObject *retval = NULL;
  PyObject *gnubgid = NULL;

  if (!PyArg_ParseTuple(args, "|i:hint", &nMaxMoves))
    return NULL;
  if (nMaxMoves < 0)
    nMaxMoves = MAX_MOVES;

  if (ms.gs != GAME_PLAYING) {
    PyErr_SetString(PyExc_RuntimeError, "You must set up a board first.");
    return NULL;
  }
  if (!ms.anDice[0] && !ms.fDoubled && !ms.fResigned) {
    PyErr_SetString(PyExc_RuntimeError,
                    "Hints for cube actions not yet implemented");
    return NULL;
  }
  if (ms.fResigned) {
    PyErr_SetString(PyExc_RuntimeError,
                    "Hints for resignations not yet implemented");
    return NULL;
  }
  if (ms.fDoubled) {
    PyErr_SetString(PyExc_RuntimeError,
                    "Hints for take actions not yet implemented");
    return NULL;
  }
  if (!ms.anDice[0]) {
    PyErr_SetString(PyExc_RuntimeError, "No dice");
    return NULL;
  }

  snprintf(szNumber, sizeof(szNumber), "%d", nMaxMoves);
  memset(&prochint, 0, sizeof(prochint));
  prochint.pvUserData = PyList_New(0);
  if (!prochint.pvUserData)
    return NULL;
  prochint.pfProcessRecord = PythonHint_Callback;
  prochint.avInputData[PROCREC_HINT_ARGIN_SHOWPROGRESS] = (void *)(intptr_t)0;
  prochint.avInputData[PROCREC_HINT_ARGIN_MAXMOVES] =
      (void *)(intptr_t)nMaxMoves;
  hint_move(szNumber, FALSE, &prochint);
  if (MT_SafeGet(&fInterrupt)) {
    ResetInterrupt();
    Py_DECREF((PyObject *)prochint.pvUserData);
    PyErr_SetString(PyExc_RuntimeError, "interrupted/errno in hint_move");
    return NULL;
  }
  retval = (PyObject *)prochint.pvUserData;
  gnubgid = PythonGnubgID(self, Py_BuildValue("()"));
  if (gnubgid) {
    PyObject *tmp = Py_BuildValue("{s:s,s:N,s:N}", "hinttype", "chequer",
                                  "gnubgid", gnubgid, "hint", retval);
    Py_DECREF(retval);
    Py_DECREF(gnubgid);
    retval = tmp;
  }
  return retval;
}

/* Helper: set dict key and decref value (steal reference) */
static void DictSetItemSteal(PyObject *dict, const char *key, PyObject *val) {
  if (PyDict_SetItemString(dict, key, val) == 0)
    Py_DECREF(val);
}

/* No-op for library build (no GUI) */
static PyObject *PythonUpdateUI(PyObject *self, PyObject *args) {
  (void)self;
  (void)args;
  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Ported from gnubgmodule.c: PythonNavigate
 * Exposed as: gnubg.navigate(next=..., game=...)
 */
static PyObject *PythonNavigate(PyObject *self, PyObject *args,
                                PyObject *keywds) {
  int nextRecord = INT_MIN;
  int nextGame = INT_MIN;
  PyObject *r = NULL;
  static const char *kwlist[] = {"next", "game", NULL};

  (void)self;
  if (!lMatch.plNext) {
    PyErr_SetString(PyExc_RuntimeError, "no active match");
    return NULL;
  }
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|ii", (char **)kwlist,
                                   &nextRecord, &nextGame))
    return NULL;

  if (nextRecord == INT_MIN && nextGame == INT_MIN) {
    if (lMatch.plNext->p)
      ChangeGame((listOLD *)lMatch.plNext->p);
  } else {
    int gamesDif = 0;
    int recordsDiff = 0;

    if (nextRecord != INT_MIN && nextRecord < 0) {
      PyErr_SetString(PyExc_ValueError, "negative next record");
      return NULL;
    }
    if (nextGame != INT_MIN && nextGame != 0) {
      listOLD *pl = lMatch.plNext;
      while ((listOLD *)pl->p != plGame && pl != &lMatch)
        pl = pl->plNext;
      int n = nextGame;
      if (n > 0) {
        while (n > 0 && pl->plNext->p) {
          pl = pl->plNext;
          --n;
        }
      } else {
        while (n < 0 && pl->plPrev->p) {
          pl = pl->plPrev;
          ++n;
        }
      }
      ChangeGame((listOLD *)pl->p);
      gamesDif = abs(nextGame) - n;
    }
    if (nextRecord != INT_MIN)
      recordsDiff = nextRecord - InternalCommandNext(0, 0, nextRecord);

    if (plLastMove->plNext && plLastMove->plNext->p) {
      const moverecord *pmr = (const moverecord *)(plLastMove->plNext->p);
      if (pmr->mt == MOVE_NORMAL)
        memcpy(ms.anDice, pmr->anDice, sizeof(ms.anDice));
    }
    if (recordsDiff || gamesDif)
      r = Py_BuildValue("(ii)", recordsDiff, gamesDif);
  }
  if (ms.gs == GAME_NONE)
    ms.gs = GAME_PLAYING;
  if (!r)
    r = Py_None;
  Py_INCREF(r);
  return r;
}

static void addProperty(PyObject *dict, const char *name, const char *val) {
  if (!val)
    return;
  PyObject *v = PyUnicode_FromString(val);
  if (v) {
    PyDict_SetItemString(dict, name, v);
    Py_DECREF(v);
  }
}

/* Format anMove[8] as tuple of (from, to) pairs. */
static PyObject *PyMove(const int move[8]) {
  int n = 0;
  while (n < 4 && move[2 * n] >= 0)
    ++n;
  PyObject *moveTuple = PyTuple_New(n);
  if (!moveTuple)
    return NULL;
  for (int i = 0; i < n; ++i) {
    PyObject *c = Py_BuildValue("(ii)", move[2 * i] + 1, move[2 * i + 1] + 1);
    if (!c) {
      Py_DECREF(moveTuple);
      return NULL;
    }
    PyTuple_SET_ITEM(moveTuple, i, c);
  }
  return moveTuple;
}

/* Minimal PythonGame: info + game records (no analysis). For full match()
 * output. */
static PyObject *PythonGame(const listOLD *plGame, int /* doAnalysis */,
                            int /* verbose */, void * /* scMatch */,
                            int includeBoards, void * /* ms */) {
  const listOLD *pl = plGame->plNext;
  const moverecord *pmr = (const moverecord *)pl->p;
  const xmovegameinfo *g = &pmr->g;
  PyObject *gameDict = PyDict_New();
  PyObject *gameInfoDict = PyDict_New();
  if (!gameDict || !gameInfoDict) {
    Py_XDECREF(gameDict);
    Py_XDECREF(gameInfoDict);
    return NULL;
  }
  DictSetItemSteal(gameInfoDict, "score-X", PyLong_FromLong(g->anScore[0]));
  DictSetItemSteal(gameInfoDict, "score-O", PyLong_FromLong(g->anScore[1]));
  if (g->anScore[0] + 1 == (int)g->nMatch ||
      g->anScore[1] + 1 == (int)g->nMatch)
    DictSetItemSteal(gameInfoDict, "crawford",
                     PyBool_FromLong(g->fCrawfordGame));
  if (g->fWinner >= 0) {
    DictSetItemSteal(gameInfoDict, "winner",
                     PyUnicode_FromString(g->fWinner ? "O" : "X"));
    DictSetItemSteal(gameInfoDict, "points-won", PyLong_FromLong(g->nPoints));
    DictSetItemSteal(gameInfoDict, "resigned", PyBool_FromLong(g->fResigned));
  } else {
    PyObject *none = Py_None;
    Py_INCREF(none);
    DictSetItemSteal(gameInfoDict, "winner", none);
  }
  if (g->nAutoDoubles)
    DictSetItemSteal(gameInfoDict, "initial-cube",
                     PyLong_FromLong(1 << g->nAutoDoubles));
  DictSetItemSteal(gameDict, "info", gameInfoDict);

  TanBoard anBoard;
  int nRecords = 0;
  for (const listOLD *t = pl->plNext; t != plGame; t = t->plNext)
    ++nRecords;
  PyObject *gameTuple = PyTuple_New(nRecords);
  if (!gameTuple) {
    Py_DECREF(gameDict);
    return NULL;
  }
  if (includeBoards)
    InitBoard(anBoard, g->bgv);
  nRecords = 0;
  for (pl = pl->plNext; pl != plGame; pl = pl->plNext) {
    pmr = (const moverecord *)pl->p;
    PyObject *recordDict = PyDict_New();
    if (!recordDict) {
      Py_DECREF(gameTuple);
      Py_DECREF(gameDict);
      return NULL;
    }
    const char *action = NULL;
    int player = -1;
    long points = -1;
    switch (pmr->mt) {
    case MOVE_NORMAL:
      action = "move";
      player = pmr->fPlayer;
      DictSetItemSteal(recordDict, "dice",
                       Py_BuildValue("(ii)", pmr->anDice[0], pmr->anDice[1]));
      DictSetItemSteal(recordDict, "move", PyMove(pmr->n.anMove));
      if (includeBoards) {
        DictSetItemSteal(
            recordDict, "board",
            PyUnicode_FromString(PositionID((ConstTanBoard)anBoard)));
        ApplyMove(anBoard, pmr->n.anMove, 0);
        SwapSides(anBoard);
      }
      break;
    case MOVE_DOUBLE:
      action = "double";
      player = pmr->fPlayer;
      if (includeBoards)
        DictSetItemSteal(
            recordDict, "board",
            PyUnicode_FromString(PositionID((ConstTanBoard)anBoard)));
      break;
    case MOVE_TAKE:
      action = "take";
      player = pmr->fPlayer;
      break;
    case MOVE_DROP:
      action = "drop";
      player = pmr->fPlayer;
      break;
    case MOVE_RESIGN:
      action = "resign";
      player = pmr->fPlayer;
      points = pmr->r.nResigned;
      if (points < 1)
        points = 1;
      else if (points > 3)
        points = 3;
      break;
    case MOVE_SETBOARD:
      action = "set";
      DictSetItemSteal(recordDict, "board",
                       PyUnicode_FromString(PositionIDFromKey(&pmr->sb.key)));
      if (includeBoards)
        PositionFromKey(anBoard, &pmr->sb.key);
      break;
    case MOVE_SETDICE:
      action = "set";
      player = pmr->fPlayer;
      DictSetItemSteal(recordDict, "dice",
                       Py_BuildValue("(ii)", pmr->anDice[0], pmr->anDice[1]));
      break;
    case MOVE_SETCUBEVAL:
      action = "set";
      DictSetItemSteal(recordDict, "cube", PyLong_FromLong(pmr->scv.nCube));
      break;
    case MOVE_SETCUBEPOS:
      action = "set";
      DictSetItemSteal(
          recordDict, "cube-owner",
          PyUnicode_FromString(
              pmr->scp.fCubeOwner == 0
                  ? "X"
                  : (pmr->scp.fCubeOwner == 1 ? "O" : "centered")));
      break;
    default:
      break;
    }
    if (action)
      DictSetItemSteal(recordDict, "action", PyUnicode_FromString(action));
    if (player >= 0)
      DictSetItemSteal(recordDict, "player",
                       PyUnicode_FromString(player ? "O" : "X"));
    if (points >= 0)
      DictSetItemSteal(recordDict, "points", PyLong_FromLong(points));
    if (pmr->sz)
      DictSetItemSteal(recordDict, "comment", PyUnicode_FromString(pmr->sz));
    PyTuple_SET_ITEM(gameTuple, nRecords++, recordDict);
  }
  DictSetItemSteal(gameDict, "game", gameTuple);
  return gameDict;
}

/*
 * Ported from gnubgmodule.c: PythonMatch
 * Exposed as: gnubg.match(analysis=..., boards=..., statistics=...,
 * verbose=...)
 */
static PyObject *PythonMatch(PyObject *self, PyObject *args, PyObject *keywds) {
  static const char *kwlist[] = {"analysis", "boards", "statistics", "verbose",
                                 NULL};
  int includeAnalysis = 1, verboseAnalysis = 0, statistics = 0, boards = 1;
  const listOLD *firstGame =
      lMatch.plNext ? (const listOLD *)lMatch.plNext->p : NULL;
  PyObject *matchDict = NULL;
  PyObject *matchInfoDict = NULL;

  (void)self;
  if (!firstGame || !firstGame->plNext) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  const moverecord *pmr = (const moverecord *)firstGame->plNext->p;
  const xmovegameinfo *g = &pmr->g;
  if (g->i != 0) {
    PyErr_SetString(PyExc_RuntimeError, "First game missing from match");
    return NULL;
  }
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|iiii", (char **)kwlist,
                                   &includeAnalysis, &boards, &statistics,
                                   &verboseAnalysis))
    return NULL;

  matchDict = PyDict_New();
  matchInfoDict = PyDict_New();
  if (!matchDict || !matchInfoDict) {
    Py_XDECREF(matchDict);
    Py_XDECREF(matchInfoDict);
    return NULL;
  }
  for (int side = 0; side < 2; ++side) {
    PyObject *d = PyDict_New();
    if (!d) {
      Py_DECREF(matchDict);
      Py_DECREF(matchInfoDict);
      return NULL;
    }
    addProperty(d, "rating", mi.pchRating[side]);
    addProperty(d, "name", ap[side].szName);
    DictSetItemSteal(matchInfoDict, side == 0 ? "X" : "O", d);
  }
  DictSetItemSteal(matchInfoDict, "match-length", PyLong_FromLong(g->nMatch));
  if (mi.nYear) {
    PyObject *date = Py_BuildValue("(iii)", mi.nDay, mi.nMonth, mi.nYear);
    DictSetItemSteal(matchInfoDict, "date", date);
  }
  addProperty(matchInfoDict, "event", mi.pchEvent);
  addProperty(matchInfoDict, "round", mi.pchRound);
  addProperty(matchInfoDict, "place", mi.pchPlace);
  addProperty(matchInfoDict, "annotator", mi.pchAnnotator);
  addProperty(matchInfoDict, "comment", mi.pchComment);
  int result = 0;
  int anFinalScore[2];
  if (getFinalScore(anFinalScore)) {
    if (anFinalScore[0] > (int)g->nMatch)
      result = -1;
    else if (anFinalScore[1] > (int)g->nMatch)
      result = 1;
  }
  DictSetItemSteal(matchInfoDict, "result", PyLong_FromLong(result));
  {
    const char *v[] = {"Standard", "Nackgammon", "Hypergammon1", "Hypergammon2",
                       "Hypergammon3"};
    addProperty(matchInfoDict, "variation", v[g->bgv]);
  }
  {
    unsigned int n = !g->fCubeUse + !!g->fCrawford + !!g->fJacoby;
    if (n) {
      PyObject *rules = PyTuple_New(n);
      if (rules) {
        unsigned int idx = 0;
        if (!g->fCubeUse)
          PyTuple_SET_ITEM(rules, idx++, PyUnicode_FromString("NoCube"));
        if (g->fCrawford)
          PyTuple_SET_ITEM(rules, idx++, PyUnicode_FromString("Crawford"));
        if (g->fJacoby)
          PyTuple_SET_ITEM(rules, idx, PyUnicode_FromString("Jacoby"));
        DictSetItemSteal(matchInfoDict, "rules", rules);
      }
    }
  }
  DictSetItemSteal(matchDict, "match-info", matchInfoDict);

  statcontext scm;
  if (statistics)
    IniStatcontext(&scm);
  Py_ssize_t nGames = 0;
  for (const listOLD *pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext)
    ++nGames;
  PyObject *matchTuple = PyTuple_New(nGames);
  if (!matchTuple) {
    Py_DECREF(matchDict);
    return NULL;
  }
  nGames = 0;
  for (const listOLD *pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext) {
    PyObject *pg =
        PythonGame((const listOLD *)pl->p, includeAnalysis, verboseAnalysis,
                   statistics ? &scm : NULL, boards, NULL);
    if (!pg) {
      Py_DECREF(matchTuple);
      Py_DECREF(matchDict);
      return NULL;
    }
    PyTuple_SET_ITEM(matchTuple, nGames++, pg);
  }
  DictSetItemSteal(matchDict, "games", matchTuple);
  /* statistics/stats: PyGameStats lives in original gnubgmodule.c; omit for
   * library build */
  (void)statistics;
  (void)scm;
  return matchDict;
}

/*
 * Ported from gnubgmodule.c: PythonGetEvalHintFilter
 * Exposed as: gnubg.getevalhintfilter()
 */
static PyObject *PythonGetEvalHintFilter(PyObject *self, PyObject *args) {
  if (!PyArg_ParseTuple(args, ":getevalhintfilter"))
    return NULL;
  return MoveFiltersToPy(*GetEvalMoveFilter());
}

/*
 * Ported from gnubgmodule.c: PythonSetEvalHintFilter
 * Exposed as: gnubg.setevalhintfilter(list_of_movefilters)
 */
static PyObject *PythonSetEvalHintFilter(PyObject *self, PyObject *args) {
  PyObject *pyMoveFilters = NULL;
  TmoveFilter *aamf = GetEvalMoveFilter();
  if (!PyArg_ParseTuple(args, "|O:setevalhintfilter", &pyMoveFilters))
    return NULL;
  if (!pyMoveFilters) {
    PyErr_SetString(PyExc_TypeError,
                    "setevalhintfilter requires 1 argument: a list of move "
                    "filters (see gnubg.getevalhintfilter() for an example)");
    return NULL;
  }
  if (PyToMoveFilters(pyMoveFilters, *aamf) != 0)
    return NULL;
  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Ported from gnubgmodule.c: PythonCommand
 * Exposed as: gnubg.command(cmd_string)
 */
static PyObject *PythonCommand(PyObject *self, PyObject *args) {
  const char *pch = NULL;
  char *sz = NULL;
  psighandler sh;
  int suppress_output = 0;
  if (!PyArg_ParseTuple(args, "s:command", &pch))
    return NULL;
  sz = g_strdup(pch);
  {
    char *trimmed = g_strstrip(sz);
    if (g_ascii_strncasecmp(trimmed, "new session", 11) == 0 &&
        (trimmed[11] == '\0' || g_ascii_isspace(trimmed[11])))
      suppress_output = 1;
  }
  if (suppress_output)
    outputoff();
  PortableSignal(SIGINT, HandleInterrupt, &sh, FALSE);
  HandleCommand(sz, acTop);
  while (fNextTurn)
    NextTurn(TRUE);
  outputx();
  if (suppress_output)
    outputon();
  g_free(sz);
  PortableSignalRestore(SIGINT, &sh);
  if (MT_SafeGet(&fInterrupt)) {
    MT_SafeSet(&fInterrupt, FALSE);
  }
  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Ported from gnubgmodule.c: PythonShow
 * Exposed as: gnubg.show(arguments_string)
 */
static PyObject *PythonShow(PyObject *self, PyObject *args) {
  const char *pch = NULL;
  char *sz = NULL;
  PyObject *p = NULL;
  if (!PyArg_ParseTuple(args, "s:show", &pch))
    return NULL;
  sz = g_strdup(pch);
  foutput_to_mem = TRUE;
  HandleCommand(sz, acShow);
  foutput_to_mem = FALSE;
  g_free(sz);
  if (szMemOutput) {
    size_t len = strlen(szMemOutput);
    while (len > 0 && szMemOutput[len - 1] == '\n')
      szMemOutput[--len] = '\0';
    p = PyUnicode_FromString(szMemOutput);
    g_free(szMemOutput);
    szMemOutput = NULL;
  } else {
    p = PyUnicode_FromString("");
  }
  return p;
}

/*
 * Ported from gnubgmodule.c: PythonNextTurn
 * Exposed as: gnubg.nextturn()
 */
static PyObject *PythonNextTurn(PyObject *self, PyObject *args) {
  if (!PyArg_ParseTuple(args, ":nextturn"))
    return NULL;
  fNextTurn = TRUE;
  while (fNextTurn) {
    if (NextTurn(TRUE) == -1)
      fNextTurn = FALSE;
  }
  Py_RETURN_NONE;
}

/*
 * Ported from gnubgmodule.c: PythonSetGNUbgID
 * Exposed as: gnubg.setgnubgid(gnubgid_or_xgid_string)
 */
static PyObject *PythonSetGNUbgID(PyObject *self, PyObject *args) {
  const char *pch = NULL;
  char *sz = NULL;
  if (!PyArg_ParseTuple(args, "s:setgnubgid", &pch))
    return NULL;
  sz = g_strdup(pch);
  SetGNUbgID(sz);
  g_free(sz);
  Py_RETURN_NONE;
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

    {"positionid", PythonPositionID, METH_VARARGS,
     "Return position ID from board\n"
     "    arguments: [board] (optional, uses current board if not provided)\n"
     "    returns: position ID as string"},

    {"positionfromid", PythonPositionFromID, METH_VARARGS,
     "Return board from position ID\n"
     "    arguments: [position ID as string] (optional)\n"
     "    returns: board (tuple of two tuples of 25 ints)"},

    {"positionkey", PythonPositionKey, METH_VARARGS,
     "Return key for position\n"
     "    arguments: [board] (optional, uses current board if not provided)\n"
     "    returns: tuple of 10 ints"},

    {"positionfromkey", PythonPositionFromKey, METH_VARARGS,
     "Return position from key\n"
     "    arguments: [list/tuple of 10 ints] (optional)\n"
     "    returns: board (tuple of two tuples of 25 ints)"},

    {"cubeinfo", PythonCubeInfo, METH_VARARGS,
     "Make a cubeinfo dictionary\n"
     "    arguments: [cube value, cube owner, player on move, match length,\n"
     "                score tuple, crawford flag, bgv]\n"
     "    returns: cubeinfo dictionary"},

    {"posinfo", PythonPosInfo, METH_VARARGS,
     "Make a posinfo dictionary\n"
     "    arguments: [player on roll, player resigned, player doubled,\n"
     "                gamestate, dice tuple]\n"
     "    returns: posinfo dictionary"},

    {"evalcontext", PythonEvalContext, METH_VARARGS,
     "Make an evalcontext dictionary\n"
     "    arguments: [cubeful, plies, deterministic, prune, noise]\n"
     "    returns: evalcontext dictionary"},

    {"evaluate", PythonEvaluate, METH_VARARGS,
     "Evaluate position (win/gammon/backgammon probs and equity)\n"
     "    arguments: [board], [cubeinfo], [evalcontext] (all optional)\n"
     "    returns: tuple of 6 floats (win, wingammon, winbackgammon, "
     "losegammon, losebackgammon, equity)"},

    {"rolloutcontext", PythonRolloutContext, METH_VARARGS,
     "Make a rolloutcontext dictionary\n"
     "    arguments: optional 16 ints + 2 floats (cubeful, variance-reduction, "
     "...)\n"
     "    returns: rolloutcontext dictionary"},

    {"classify", PythonClassifyPosition, METH_VARARGS,
     "Classify position type\n"
     "    arguments: [board, variant]\n"
     "    returns: position class as integer"},

    {"luckrating", PythonLuckRating, METH_VARARGS,
     "Convert luck per move to rating (0-5)\n"
     "    arguments: float luck per move\n"
     "    returns: int 0..5 (very unlucky to very lucky)"},

    {"errorrating", PythonErrorRating, METH_VARARGS,
     "Convert error per move to rating (0-7)\n"
     "    arguments: float error per move\n"
     "    returns: int 0..7 (awful to supernatural)"},

    {"parsemove", PythonParseMove, METH_VARARGS,
     "Parse move string\n"
     "    arguments: string containing move to parse (e.g., '8/5 6/5')\n"
     "    returns: tuple of (tuple (int, int)) representing each move"},

    {"movetupletostring", PythonMoveTuple2String, METH_VARARGS,
     "Convert move tuple to string\n"
     "    arguments: move tuple, board\n"
     "    returns: string representation of move"},

    {"findbestmove", PythonFindBestMove, METH_VARARGS,
     "Find best move for position and dice\n"
     "    arguments: [board], [cubeinfo], [evalcontext], [dice], [movefilters] "
     "(dice required if no game)\n"
     "    returns: tuple of (from, to) pairs, 1-based"},

    {"findbestmoves", PythonFindBestMoves, METH_VARARGS,
     "Find all legal moves for position and dice, ordered by score (best first)\n"
     "    arguments: same as findbestmove\n"
     "    returns: list of dicts {\"move\": (from,to,...), \"score\": float}"},

    {"met", PythonMET, METH_VARARGS,
     "Return match equity table\n"
     "    arguments: [max score] (optional)\n"
     "    returns: list of 3: pre-Crawford table, post-Crawford player 0, "
     "post-Crawford player 1"},

    {"matchid", PythonMatchID, METH_VARARGS,
     "Return match ID string\n"
     "    arguments: [cubeinfo], [posinfo] (optional; from gnubg.cubeinfo(), "
     "gnubg.posinfo())\n"
     "    returns: match ID string"},

    {"gnubgid", PythonGnubgID, METH_VARARGS,
     "Return GNUbgID string (positionid:matchid)\n"
     "    arguments: [board], [cubeinfo], [posinfo] (optional; use 0 or all "
     "3)\n"
     "    returns: GNUbgID string"},

    {"dicerolls", PythonDiceRolls, METH_VARARGS,
     "Return list of dice rolls from RNG\n"
     "    arguments: number of rolls (int)\n"
     "    returns: list of (die1, die2) tuples"},

    {"matchchecksum", PythonMatchChecksum, METH_VARARGS,
     "Calculate checksum for current match\n"
     "    arguments: none\n"
     "    returns: MD5 digest as 32-char hex string"},

    {"positionbearoff", PythonPositionBearoff, METH_VARARGS,
     "Return bearoff id for position\n"
     "    arguments: [board], [nPoints], [nChequers] (optional)\n"
     "    returns: int bearoff id"},

    {"positionfrombearoff", PythonPositionFromBearoff, METH_VARARGS,
     "Return board (one side) from bearoff id\n"
     "    arguments: [id], [nChequers], [nPoints] (optional)\n"
     "    returns: tuple of 25 ints"},

    {"eq2mwc", PythonEq2mwc, METH_VARARGS,
     "Convert equity to match-winning chance\n"
     "    arguments: [float equity], [cubeinfo] (optional)\n"
     "    returns: float MWC"},

    {"eq2mwc_stderr", PythonEq2mwcStdErr, METH_VARARGS,
     "Convert equity standard error to MWC\n"
     "    arguments: [float equity], [cubeinfo] (optional)\n"
     "    returns: float MWC stderr"},

    {"mwc2eq", PythonMwc2eq, METH_VARARGS,
     "Convert match-winning chance to equity\n"
     "    arguments: [float mwc], [cubeinfo] (optional)\n"
     "    returns: float equity"},

    {"mwc2eq_stderr", PythonMwc2eqStdErr, METH_VARARGS,
     "Convert MWC standard error to equity\n"
     "    arguments: [float mwc], [cubeinfo] (optional)\n"
     "    returns: float equity stderr"},

    {"getevalhintfilter", PythonGetEvalHintFilter, METH_VARARGS,
     "Return hint/eval move filters\n"
     "    arguments: none\n"
     "    returns: list of movefilter dicts"},

    {"setevalhintfilter", PythonSetEvalHintFilter, METH_VARARGS,
     "Set hint/eval move filters\n"
     "    arguments: list of movefilter dicts (see getevalhintfilter)\n"
     "    returns: None"},

    {"command", PythonCommand, METH_VARARGS,
     "Execute a GNUBG command\n"
     "    arguments: string containing command\n"
     "    returns: None"},

    {"show", PythonShow, METH_VARARGS,
     "Execute 'show arguments' command\n"
     "    arguments: string (e.g. 'board', 'match')\n"
     "    returns: result string with trailing newlines stripped"},

    {"nextturn", PythonNextTurn, METH_VARARGS,
     "Play one turn\n"
     "    arguments: none\n"
     "    returns: None"},

    {"setgnubgid", PythonSetGNUbgID, METH_VARARGS,
     "Set current board and match from GNUbgID or XGID string\n"
     "    arguments: string (GNUbgID or XGID)\n"
     "    returns: None"},

    {"hint", PythonHint, METH_VARARGS,
     "Get hint for current position (chequer play)\n"
     "    arguments: [maxmoves] (optional)\n"
     "    returns: dict with hinttype, gnubgid, hint (list of move analyses)"},

    {"navigate", (PyCFunction)(PyCFunctionWithKeywords)PythonNavigate,
     METH_VARARGS | METH_KEYWORDS,
     "Navigate match/session\n"
     "    arguments: next=N, game=N (optional)\n"
     "    returns: None or (records_moved, games_moved)"},

    {"match", (PyCFunction)(PyCFunctionWithKeywords)PythonMatch,
     METH_VARARGS | METH_KEYWORDS,
     "Get current match\n"
     "    arguments: analysis=, boards=, statistics=, verbose= (optional)\n"
     "    returns: dict with match-info and games"},

    {"updateui", PythonUpdateUI, METH_VARARGS,
     "No-op in library build (no GUI). Kept for API compatibility."},

    {NULL, NULL, 0, NULL}};

// Module definition
static struct PyModuleDef gnubgmodule = {
    PyModuleDef_HEAD_INIT, "_gnubg",
    "Python bindings for the full GNUBG engine", -1, GnubgMethods};

// Helper: return true if path contains gnubg.wd or gnubg.weights (validate data dir)
static bool data_dir_has_weights(const char *dir) {
  if (!dir)
    return false;
  std::string base(dir);
  FILE *f = fopen((base + "/gnubg.wd").c_str(), "rb");
  if (f) {
    fclose(f);
    return true;
  }
  f = fopen((base + "/gnubg.weights").c_str(), "r");
  if (f) {
    fclose(f);
    return true;
  }
  return false;
}

// Compute package data dir from the extension's location so pip-installed package works without env.
static std::string get_package_data_dir_from_so(void) {
  auto data_dir_from_base = [](const std::string &base) -> std::string {
    if (base.size() >= 5 && base.compare(base.size() - 5, 5, "gnubg") == 0 &&
        (base.size() == 5 || base[base.size() - 6] == '/' || base[base.size() - 6] == '\\'))
      return base + "/data";
    return base + "/gnubg/data";
  };
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
  Dl_info info;
  if (dladdr(reinterpret_cast<void *>(&get_package_data_dir_from_so), &info) && info.dli_fname) {
    std::string path(info.dli_fname);
    size_t slash = path.find_last_of("/\\");
    if (slash != std::string::npos) {
      std::string dir = path.substr(0, slash);
      return data_dir_from_base(dir);
    }
  }
#elif defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
  char path[MAX_PATH];
  HMODULE hMod = NULL;
  if (GetModuleHandleExA(
          GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
          reinterpret_cast<LPCSTR>(&get_package_data_dir_from_so), &hMod) &&
      hMod && GetModuleFileNameA(hMod, path, sizeof(path))) {
    std::string s(path);
    size_t slash = s.find_last_of("/\\");
    if (slash != std::string::npos) {
      std::string data_dir = data_dir_from_base(s.substr(0, slash));
      std::replace(data_dir.begin(), data_dir.end(), '/', '\\');
      return data_dir;
    }
  }
#endif
  return std::string();
}

// Set GNUBG_DATA_DIR to package-relative path when not already set, so data loads "off the bat".
static void set_gnubg_data_dir_env_if_unset(void) {
  if (std::getenv("GNUBG_DATA_DIR") != nullptr)
    return;
  std::string pkg_data = get_package_data_dir_from_so();
  if (pkg_data.empty())
    return;
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
  std::string env = "GNUBG_DATA_DIR=" + pkg_data;
  _putenv(env.c_str());
#else
  setenv("GNUBG_DATA_DIR", pkg_data.c_str(), 1);  // 1 = overwrite (we only call when unset)
#endif
}

// Set package data dir: prefer GNUBG_DATA_DIR env (override), then path relative to
// this shared object so the package is self-contained and no env var is required.
static void set_pkg_datadir_from_module(void) {
  set_gnubg_data_dir_env_if_unset();
  auto try_set = [](const std::string &data_dir) -> bool {
    if (data_dir_has_weights(data_dir.c_str())) {
      gnubg_lib_set_pkg_datadir(data_dir.c_str());
      return true;
    }
    return false;
  };
  const char *env_dir = std::getenv("GNUBG_DATA_DIR");
  if (env_dir && env_dir[0] && try_set(env_dir))
    return;
  auto data_dir_from_base = [](const std::string &base) -> std::string {
    if (base.size() >= 5 && base.compare(base.size() - 5, 5, "gnubg") == 0 &&
        (base.size() == 5 || base[base.size() - 6] == '/' || base[base.size() - 6] == '\\'))
      return base + "/data";
    return base + "/gnubg/data";
  };
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
  Dl_info info;
  if (dladdr(reinterpret_cast<void *>(&set_pkg_datadir_from_module), &info) && info.dli_fname) {
    std::string path(info.dli_fname);
    size_t slash = path.find_last_of("/\\");
    if (slash != std::string::npos) {
      std::string dir = path.substr(0, slash);
      if (try_set(data_dir_from_base(dir)))
        return;
      // Editable build: .so in build/cp310/ -> try source (prefer package data: weights + bearoff + met)
      if (dir.find("build") != std::string::npos) {
        size_t build_pos = dir.find("build");
        std::string prefix = dir.substr(0, build_pos);
        if (try_set(prefix + "src/gnubgmodule/data"))
          return;
        if (try_set(prefix + "src/gnubg"))
          return;
      }
    }
  }
#elif defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
  char path[MAX_PATH];
  HMODULE hMod = NULL;
  if (GetModuleHandleExA(
          GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
          reinterpret_cast<LPCSTR>(&set_pkg_datadir_from_module), &hMod) &&
      hMod && GetModuleFileNameA(hMod, path, sizeof(path))) {
    std::string s(path);
    size_t slash = s.find_last_of("/\\");
    if (slash != std::string::npos) {
      std::string data_dir = data_dir_from_base(s.substr(0, slash));
      std::replace(data_dir.begin(), data_dir.end(), '/', '\\');
      if (try_set(data_dir))
        return;
      if (s.find("build") != std::string::npos) {
        size_t build_pos = s.find("build");
        std::string src_data = s.substr(0, build_pos) + "src\\gnubgmodule\\data";
        if (try_set(src_data))
          return;
      }
    }
  }
#endif
}

// Initialization function - must have C linkage for Python to find it
// Explicitly export the symbol to ensure it's visible
extern "C" {
PyMODINIT_FUNC PyInit__gnubg(void) {
  set_pkg_datadir_from_module();
  gnubg_lib_init_for_python();
  return PyModule_Create(&gnubgmodule);
}
}
