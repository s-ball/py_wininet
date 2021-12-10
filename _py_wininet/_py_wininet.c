#include "_py_wininet.h"
#include <Windows.h>
#include <wininet.h>

static int exec_mod(PyObject* module) {
    int cr = -1;
    PyObject* error = PyErr_NewExceptionWithDoc(MOD_NAME ".error",
        PyDoc_STR("Generic exception for the " MOD_NAME " module"),
        NULL, NULL);
    if (error) {
        PyObject* mod_name = PyModule_GetNameObject(module);
        if (mod_name) {
            if (0 == PyObject_SetAttrString(error, "__module__", mod_name)) {
                cr = PyModule_AddObject(module, "error", error);
            }
            Py_DECREF(mod_name);
        }
        if (cr != 0) {
            Py_DECREF(error);
        }
    }
    return  cr;
}
/*
 * Documentation for _py_wininet.
 */
PyDoc_STRVAR(mod_doc, "Internal module to wrap the Wininet library.");

static PyModuleDef_Slot mod_slots[] = {
    { Py_mod_exec, exec_mod },
    { Py_mod_exec, exec_Stream_py_wininet },
    { Py_mod_exec, exec_Session_py_wininet },
    { 0, NULL }
};

static PyModuleDef mod_def = {
    PyModuleDef_HEAD_INIT,
    MOD_NAME,
    mod_doc,
    0,              /* m_size */
    NULL,           /* m_methods */
    mod_slots,
    NULL,           /* m_traverse */
    NULL,           /* m_clear */
    NULL,           /* m_free */
};

PyMODINIT_FUNC PyInit__py_wininet() {
    return PyModuleDef_Init(&mod_def);
}
