#include "_py_wininet.h"

#include <Windows.h>
#include <WinInet.h>

/*
 * Loads a document using WinInet
 */

typedef struct {
    PyObject_HEAD

        const wchar_t* url;
    const char* header;
    int header_len;
    const char* header_curr;
    HINTERNET conn_handle;
    HINTERNET request_handle;
    PyObject* error;
} Stream_Object;


static void Stream_dealloc(Stream_Object* self) {
    if (self->header) free((char*)self->header);
    if (self->url) PyMem_Free((LPWSTR) self->url);
    if (self->request_handle) HttpEndRequest(self->request_handle, NULL, 0, 0);
    if (self->conn_handle) InternetCloseHandle(self->conn_handle);
    if (self->error != PyExc_RuntimeError) Py_XDECREF(self->error);
    ((destructor)PyType_GetSlot(Py_TYPE(self), Py_tp_free))((PyObject*)self);
}

LPCWSTR getUnicodeOrNull(PyObject* obj) {
    if (obj == NULL || obj == Py_None) return NULL;
    return (PyUnicode_AsWideCharString(obj, NULL));
}

static LPCWSTR defArr[] = {L"*.*", NULL};

static void freeArray(LPCWSTR* arr) {
    if (arr != defArr && arr != NULL) {
        for (LPCWSTR* it = *arr; *it != NULL; ++it) {
            PyMem_Free((LPWSTR) it);
        }
        free((LPWSTR) arr);
    }
}

static LPCWSTR* getArray(PyObject* obj) {
    if (Py_None == obj) return defArr;
    Py_ssize_t sz = PySequence_Size(obj);
    if (sz == -1) return NULL;
    PyObject* iter;
    LPCWSTR* arr = calloc(sz + 1, sizeof(*arr));
    if (arr == 0) {
        PyErr_Format(PyExc_MemoryError, "Could not malloc");
        return NULL;
    }
    if (sz == 0) return arr;
    if (iter = PyObject_GetIter(obj)) {
        LPCWSTR* it = arr;
        PyObject* o;
        while (o = PyIter_Next(iter)) {
            *it = PyUnicode_AsWideCharString(o, NULL);
            Py_DECREF(o);
            if (!*it) break;
        }
        Py_DECREF(iter);
        if (!PyErr_Occurred()) {
            return arr;
        }
    }
    freeArray(arr);
    return NULL;
}

static int Stream_init(Stream_Object* self, PyObject* args, PyObject* kwargs) {
    int cr = -1;
    const char* info = NULL;
    PyObject* tmp = PyImport_AddModule(MOD_NAME);
    if (tmp) {
        self->error = PyObject_GetAttrString(tmp, "error");
    }
    if (!self->error) self->error = PyExc_RuntimeError;
    char* kwnames[] = { "handle", "type", "host", "method", "selector",
        "fullurl", "referer", "accept_types", "headers", "data", "port", NULL};
    
    HINTERNET handle;
    BOOL ok = FALSE;
    PyObject *hostobj, *methodobj = NULL, *selectorobj, *refererobj = NULL, *accept_typesobj = Py_None;
    PyObject* headerobj = NULL, *typeobj=NULL, *fullurlobj = NULL;
    const char* data = NULL;
    DWORD headerslength = 0;
    Py_ssize_t datalength = 0;
    WORD port = 0;
    DWORD flags;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "nUUUU|UUOUz#H", kwnames, &handle,
        &typeobj, &hostobj, &methodobj, &selectorobj, &fullurlobj, &refererobj, &accept_typesobj,
        &headerobj, &data, &datalength, &port)) {
        LPCWSTR host=NULL, method=NULL, selector = NULL, referer = NULL;
        LPCWSTR* accept_types = NULL;
        LPCWSTR headers = NULL, type=NULL;
        if ((host = PyUnicode_AsWideCharString(hostobj, NULL)) &&
            (type = PyUnicode_AsWideCharString(typeobj, NULL)) &&
            (selector = PyUnicode_AsWideCharString(selectorobj, NULL)) &&
            (headers = PyUnicode_AsWideCharString(headerobj, NULL)) &&
            (self->url = PyUnicode_AsWideCharString(fullurlobj, NULL)) &&
            (accept_types = getArray(accept_typesobj))) {
            method = getUnicodeOrNull(methodobj);
            referer = getUnicodeOrNull(refererobj);
            if (PyErr_Occurred()) goto end;

            flags = 0 == lstrcmp(L"http", type) ? 0 : INTERNET_FLAG_SECURE;
            headerslength = lstrlen(headers);
            info = "InternetConnect";
            self->conn_handle = InternetConnect(handle, host, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
            if (!self->conn_handle) goto end;
            info = "HttpOpenRequest";
            self->request_handle = HttpOpenRequest(self->conn_handle, method, selector, NULL, referer, accept_types, flags, (DWORD_PTR)&self->url);
            if (!self->request_handle) goto end;
            info = "HttpSendRequest";
            if (!HttpSendRequest(self->request_handle, headers, headerslength, (LPVOID)data, (DWORD) datalength)) {
                if (ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED == GetLastError()) {
                    if (ERROR_SUCCESS != InternetErrorDlg(GetDesktopWindow(),
                        self->request_handle,
                        ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
                        FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
                        FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
                        FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
                        NULL)) {
                        info = "InternetErrorDlg";
                        goto end;
                    }
                    if (!HttpSendRequest(self->request_handle, headers, headerslength, (LPVOID)data, (DWORD) datalength)) {
                        goto end;
                    }
                }
                else goto end;
            }
            DWORD length = 0, index = 0;
            info = "HttpQueryInfo";
            HttpQueryInfoA(self->request_handle, HTTP_QUERY_RAW_HEADERS_CRLF, &index, &length, &index);
            char* buff = malloc(length + 1);
            if (buff == NULL) {
                info = NULL;
                PyErr_Format(PyExc_MemoryError, "Could not malloc");
                goto end;
            }
            if (HttpQueryInfoA(self->request_handle, HTTP_QUERY_RAW_HEADERS_CRLF, buff, &length, &index)) {
                buff[length] = 0;
                self->header = self->header_curr = buff;
                self->header_len = length;
                cr = 0;
            }
        }
    end:
        if (cr != 0) free((char*)self->header);
        freeArray(accept_types);
        PyMem_Free((LPWSTR)referer);
        PyMem_Free((LPWSTR)method);
        PyMem_Free((LPWSTR)headers);
        PyMem_Free((LPWSTR)selector);
        PyMem_Free((LPWSTR)host);
    }
    if ((cr != 0) && !PyErr_Occurred()) {
        PyErr_Format(self->error, "WinInet error in %s: %x", info, GetLastError());
    }
    return cr;
}

static PyObject* Stream_readinto(Stream_Object* self, PyObject* b) {
	char* buffer;
	Py_ssize_t buffer_len;
	if (0 != PyObject_AsWriteBuffer(b, &buffer, &buffer_len)) {
		return NULL;
	}
    if (self->header_len > 0) {
        Py_ssize_t len = buffer_len;
        if (len > self->header_len) {
            len = self->header_len;
        }
        memcpy(buffer, self->header_curr, len);
        self->header_curr += len;
        self->header_len -= (int) len;
        return PyLong_FromSsize_t(len);
    }
    else {
        DWORD len;
        if (InternetReadFile(self->request_handle, buffer, (DWORD) buffer_len, &len)) {
            return PyLong_FromUnsignedLong(len);
        }
        PyErr_Format(self->error, "Could not read %d bytes", len);
    }
    return NULL;
}

static PyObject* Stream_close(Stream_Object* self, PyObject* args) {
    DWORD err = 0;
    BOOL cr = HttpEndRequest(self->request_handle, NULL, 0, 0);
    if (!cr) err = GetLastError();
    cr = InternetCloseHandle(self->conn_handle);
    if (!err && !cr) err = GetLastError();
    Py_RETURN_NONE;
}

static PyObject* Stream_closed(Stream_Object* self, void* args) {
    if (self->request_handle) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(readinto_doc, "readinto(b)\n--\n\n"
    "Read bytes into a pre - allocated, writable bytes - like object b"
    " and return the number of bytes read.");

static PyMethodDef Stream_methods[] = {
    {"readinto", (PyCFunction)Stream_readinto, METH_O, readinto_doc},
    {"close", (PyCFunction)Stream_close, METH_NOARGS, PyDoc_STR("close()\n--\n\n ")},
    {NULL}
};

static PyGetSetDef Stream_getset[] = {
    {"closed", (PyCFunction)Stream_closed},
    {NULL}
};

static char Stream_doc[] = PyDoc_STR(
    "Reads the content of the response (including status line and headers)"
    " into the provided writable bytes-like object."
);

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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
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
                return 0;
            }
            else {
                Py_INCREF(Stream_Type);
            }
        }
        else Py_DECREF(mod_name);
    }
    Py_DECREF(Stream_Type);
    return -1;
}
