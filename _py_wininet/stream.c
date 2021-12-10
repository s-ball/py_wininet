#include "_py_wininet.h"

/*
 * Declare a local type duck typing io.RawIOBase
 */

typedef struct {
    PyObject_HEAD

        char* data;
    const char* first;
    Py_ssize_t first_size;
    const char* second;
    Py_ssize_t second_size;
    int_fast16_t closed;
} Stream_Object;


static void Stream_dealloc(Stream_Object* self) {
    if (self->data) PyMem_Free((char*)self->data);
    ((destructor)PyType_GetSlot(Py_TYPE(self), Py_tp_free))((PyObject*)self);
}

static int Stream_init(Stream_Object* self, PyObject* args, PyObject* kwargs) {
    char* kwnames[] = { "first", "second", NULL };
    char* first, * second;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "y#|y#", kwnames,
        &first, &self->first_size, &second, &self->second_size)) {
        self->data = PyMem_Malloc(self->first_size + self->second_size);
        if (self->data) {
            self->first = self->data;
            self->second = self->first + self->first_size;
            memcpy(self->data, first, self->first_size);
            memcpy(self->data + self->first_size, second, self->second_size);
            return 0;
        }
        PyErr_Format(PyExc_MemoryError, "cannot allocate");
    }
    return -1;
}

static PyObject* Stream_readinto(Stream_Object* self, PyObject* b) {
    if (self->closed) {
        PyErr_Format(PyExc_ValueError, "I/O operation on closed file.");
        return NULL;
    }
    char* buf;
    Py_ssize_t len;
#pragma warning(suppress : 4996)
    if (0 != PyObject_AsWriteBuffer(b, &buf, &len)) return NULL;
    Py_ssize_t* sz;
    const char** data;
    if (self->first_size > 0) {
        sz = &self->first_size;
        data = &self->first;
    }
    else {
        sz = &self->second_size;
        data = &self->second;
    }
    Py_ssize_t l = (len < *sz) ? len : *sz;
    memcpy(buf, *data, l);
    *data += l;
    *sz -= l;
    return PyLong_FromSsize_t(l);
}

static PyObject* Stream_readall(Stream_Object* self, PyObject* b) {
    if (self->closed) {
        PyErr_Format(PyExc_ValueError, "I/O operation on closed file.");
        return NULL;
    }
    PyObject* arr = PyByteArray_FromStringAndSize("", 0);
    PyObject* tot = PyBytes_FromString("");
    PyObject* got;
    int cr = PyByteArray_Resize(arr, 8192);
    for (;;) {
        got = Stream_readinto(self, arr);
        if (!got) break;
        cr = PyLong_AsLong(got);
        Py_DECREF(got);
        if (cr == 0) {
            break;
        }
        PyBytes_ConcatAndDel(&tot, PyBytes_FromStringAndSize(PyByteArray_AsString(arr), cr));
        if (!tot) break;
    }
    Py_DECREF(arr);
    if (!got) {
        Py_DECREF(tot);
        return NULL;
    }
    return tot;
}

static PyObject* Stream_read(Stream_Object* self, PyObject* args, PyObject* kwargs) {
    Py_ssize_t size = -1;
    char* kwnames[] = { "size", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|n", kwnames, &size)) {
        return NULL;
    }
    if (size == -1) {
        return Stream_readall(self, NULL);
    }
    PyObject* arr = PyByteArray_FromStringAndSize("", 0);
    if (arr) {
        int cr = PyByteArray_Resize(arr, size);
        PyObject* len = Stream_readinto(self, arr);
        if (!len) {
            Py_DECREF(arr);
            return NULL;
        }
        size = PyLong_AsSsize_t(len);
        Py_DECREF(len);
    }
    PyObject* tot = PyBytes_FromStringAndSize(PyByteArray_AsString(arr), size);
    Py_DECREF(arr);
    return tot;
}

static PyObject* Stream_makefile(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* brd = NULL;
    PyObject* io = PyImport_AddModule("io");
    if (io) {
        PyObject* BufferedReader = PyObject_GetAttrString(io, "BufferedReader");
        if (BufferedReader) {
            PyObject* args = Py_BuildValue("(O)", self);
            if (args) {
                brd = PyObject_Call(BufferedReader, args, NULL);
                Py_DECREF(args);
            }
            Py_DECREF(BufferedReader);
        }
    }
    return brd;
}

static PyObject* Stream_close(Stream_Object* self, PyObject* args) {
    self->closed = 1;
    Py_RETURN_NONE;
}

static PyObject* Stream_closed(Stream_Object* self, void* args) {
    if (self->closed) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* meth_true(PyObject* self, PyObject* args) {
    Py_RETURN_TRUE;
}

static PyObject* meth_false(PyObject* self, PyObject* args) {
    Py_RETURN_FALSE;
}

static PyObject* meth_none(PyObject* self, PyObject* args) {
    Py_RETURN_NONE;
}

PyDoc_STRVAR(readinto_doc, "readinto(b)\n--\n\n"
    "Read bytes into a pre - allocated, writable bytes - like object b"
    " and return the number of bytes read.");

static PyMethodDef Stream_methods[] = {
    {"readinto", (PyCFunction)Stream_readinto, METH_O, readinto_doc},
    {"readable", meth_true, METH_NOARGS, PyDoc_STR("readable()\n--\n\n ")},
    {"writable", meth_false, METH_NOARGS, PyDoc_STR("writable()\n--\n\n ")},
    {"seekable", meth_false, METH_NOARGS, PyDoc_STR("seekable()\n--\n\n ")},
    {"isatty", meth_false, METH_NOARGS, PyDoc_STR("isatty()\n--\n\n ")},
    {"flush", meth_none, METH_NOARGS, PyDoc_STR("flush()\n--\n\n ")},
    {"close", (PyCFunction)Stream_close, METH_NOARGS, PyDoc_STR("close()\n--\n\n ")},
    {"makefile", (PyCFunction)Stream_makefile, METH_VARARGS | METH_KEYWORDS, NULL},
    {"readall", (PyCFunction)Stream_readall, METH_NOARGS, PyDoc_STR("readall()\n--\n\n ")},
    {"read", (PyCFunction)Stream_read, METH_VARARGS | METH_KEYWORDS, PyDoc_STR("read(size=-1)\n--\n\n ")},
    {NULL}
};

static PyGetSetDef Stream_getset[] = {
    {"closed", (PyCFunction)Stream_closed},
    {NULL}
};

static char Stream_doc[] = PyDoc_STR(
    "Binary file-like object that provides the content of two byte-like"
    " objects in sequence.\n\nCannot be sub-classed.");

static PyType_Slot Stream_slots[] = {
    {Py_tp_init, Stream_init},
    {Py_tp_dealloc, Stream_dealloc},
    {Py_tp_methods, Stream_methods},
    {Py_tp_getset, Stream_getset},
    {Py_tp_doc, Stream_doc},
    {0}
};

static PyType_Spec Stream_spec = {
    MOD_NAME ".Stream",
    sizeof(Stream_Object),
    0,
    Py_TPFLAGS_DEFAULT,
    Stream_slots
};
/*
 * Initialization function. May be called multiple times, so avoid
 * using static state.
 */
int exec_Stream_py_wininet(PyObject* module) {
    PyObject* Stream_Type = PyType_FromSpec(&Stream_spec);
    if (!Stream_Type) return -1;
    PyObject* mod_name = PyModule_GetNameObject(module);
    if (mod_name) {
        if (0 == PyObject_SetAttrString(Stream_Type, "__module__", mod_name)) {
            Py_DECREF(mod_name);
            if (0 == PyModule_AddObject(module, "Stream", Stream_Type)) {
                PyObject* io = PyImport_ImportModule("io");
                if (io) {
                    if (0 == PyModule_AddObject(module, "io", io)) {
                        return 0; /* success */
                    }
                    Py_DECREF(io);
                }
                Py_INCREF(Stream_Type);
            }
        }
        else Py_DECREF(mod_name);
    }
    Py_DECREF(Stream_Type);
    return -1;
}
