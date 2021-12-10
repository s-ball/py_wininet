#include "_py_wininet.h"

#include <Windows.h>
#include <WinInet.h>

/*
 * Declare a wrapper over a HINTERNET session obtained form InternetOpen
 */

typedef struct {
    PyObject_HEAD
    HINTERNET handle;
 } Session_Object;


static void Session_dealloc(Session_Object* self) {
    if (self->handle) {
        InternetCloseHandle(self->handle);
    }
    ((destructor)PyType_GetSlot(Py_TYPE(self), Py_tp_free))((PyObject*)self);
}

static int Session_init(Session_Object* self, PyObject* args, PyObject* kwargs) {
    LPCWSTR defaultAgent = L"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:60.0) Gecko/20100101 Firefox/60.0";
    char* kwnames[] = { "agent", "proxy", "proxypass", NULL};
    PyObject *agentobj = NULL, *proxyobj = NULL, *proxypassobj = NULL;
    int cr = -1;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|UUU", kwnames,
        &agentobj, &proxyobj, &proxypassobj)) {
        LPCWSTR agent = NULL, proxy = NULL, proxypass = NULL;
        if (agentobj != NULL) {
            agent = PyUnicode_AsWideCharString(agentobj, NULL);
            if (!agent)  goto fin;
        }
        if (proxyobj) {
            proxy = PyUnicode_AsWideCharString(proxyobj, NULL);
            if (!proxy) goto fin;
            if (proxypassobj) {
                proxypass = PyUnicode_AsWideCharString(proxypassobj, NULL);
                if (!proxypass) goto fin;
            }
        }
        self->handle = InternetOpen(agent ? agent : defaultAgent,
            (proxy) ? INTERNET_OPEN_TYPE_PROXY : INTERNET_OPEN_TYPE_PRECONFIG,
            proxy, proxypass, 0);
        if (self->handle) cr = 0;
        else {
            PyObject* module = PyImport_AddModule(MOD_NAME);
            if (module) {
                PyObject* error = PyObject_GetAttrString(module, "error");
                if (error) {
                    PyErr_Format(error, "Error at InternetOpen %x", GetLastError());
                    Py_DECREF(error);
                }
             }
        }
        fin:
        if (agent) PyMem_Free((void *) agent);
        if (proxy) PyMem_Free((void *) proxy);
        if (proxypass) PyMem_Free((void *) proxypass);
    }
    return cr;
}


static PyObject* Session_close(Session_Object* self, PyObject* args) {
    PyObject* error = NULL;
    if (self->handle) {
        BOOL cr = InternetCloseHandle(self->handle);
        self->handle = 0;
        if (cr) Py_RETURN_NONE;
    }
    PyObject* module = PyImport_AddModule(MOD_NAME);
    if (module) {
        error = PyObject_GetAttrString(module, "error");
    }
    if (error) {
        PyErr_Format(error, "Cannot close handle %x: %x", self->handle, GetLastError());
        Py_DECREF(error);
    }
    return NULL;
}

static PyObject* Session_closed(Session_Object* self, void* args) {
    if (self->handle) {
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

static PyMethodDef Session_methods[] = {
    {"close", (PyCFunction)Session_close, METH_NOARGS, PyDoc_STR("close()\n--\n\n ")},
    {NULL}
};

static PyGetSetDef Session_getset[] = {
    {"closed", (PyCFunction)Session_closed},
    {NULL}
};

static char Session_doc[] = PyDoc_STR(
    "Wrapper around a HINTERNET session obtained from InternetOpen"
    ".\n\nCannot be sub-classed.");

static PyType_Slot Session_slots[] = {
    {Py_tp_init, Session_init},
    {Py_tp_dealloc, Session_dealloc},
    {Py_tp_methods, Session_methods},
    {Py_tp_getset, Session_getset},
    {Py_tp_doc, Session_doc},
    {0}
};

static PyType_Spec Session_spec = {
    MOD_NAME ".Session",
    sizeof(Session_Object),
    0,
    Py_TPFLAGS_DEFAULT,
    Session_slots
};
/*
 * Initialization function. May be called multiple times, so avoid
 * using static state.
 */
int exec_Session_py_wininet(PyObject* module) {
    PyObject* Session_Type = PyType_FromSpec(&Session_spec);
    if (!Session_Type) return -1;
    PyObject* mod_name = PyModule_GetNameObject(module);
    if (mod_name) {
        if (0 == PyObject_SetAttrString(Session_Type, "__module__", mod_name)) {
            Py_DECREF(mod_name);
            if (0 == PyModule_AddObject(module, "Session", Session_Type)) {
                return 0;
            }
        }
        Py_DECREF(mod_name);
    }
    Py_XDECREF(Session_Type);
    return -1;
}
