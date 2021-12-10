// A hack to allow execution under Python_d from Visual Studio
#ifndef NOTLIMITED
#ifdef _DEBUG
#undef _DEBUG
#endif
#endif

#ifndef _DEBUG
#define Py_LIMITED_API 0x03070000
#endif

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#define MOD_NAME "_py_wininet"

int exec_Stream_py_wininet(PyObject*);
int exec_Session_py_wininet(PyObject*);
