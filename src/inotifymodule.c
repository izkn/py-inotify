#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <errno.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>

#include <stdio.h>


static PyObject*
init(PyObject* self, PyObject* args) {
    int fd;

    fd = inotify_init();

    if (fd == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    return PyLong_FromLong(fd);
}


static PyObject*
init1(PyObject* self, PyObject* args, PyObject* kw) {
    static char *kwlist[] = {"flags", NULL};

    uint32_t flags = 0;
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|I", kwlist, &flags))
        return NULL;

    fd = inotify_init1(flags);

    if (fd == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    return PyLong_FromLong(fd);
}


static PyObject*
add_watch(PyObject* self, PyObject* args, PyObject* kw) {
    static char* kwlist[] = {"fd", "pathname", "mask", NULL};

    int fd, wd;
    char* pathname;
    uint32_t mask;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "isI", kwlist, &fd,
                                     &pathname, &mask))
        return NULL;

    wd = inotify_add_watch(fd, pathname, mask);

    if (wd == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    return PyLong_FromLong(wd);
}


static PyObject*
rm_watch(PyObject* self, PyObject* args, PyObject* kw) {
    static char* kwlist[] = {"fd", "wd", NULL};

    int fd, wd, rm;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "ii", kwlist, &fd, &wd))
        return NULL;

    rm = inotify_rm_watch(fd, wd);

    if (rm == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    return PyLong_FromLong(rm);
}


static PyTypeObject* parsed_event_object = NULL;


static PyObject*
read_events(PyObject* self, PyObject* args, PyObject* kw) {
    static char* kwlist[] = {"fd", "max_bytes", NULL};

    int fd;
    uint max_bytes = 0;
    char* buf __attribute__((aligned(__alignof__(struct inotify_event))));

    if (!PyArg_ParseTupleAndKeywords(args, kw, "i|I", kwlist, &fd, &max_bytes))
        return NULL;

    uint buf_size = sizeof(char);
    int bytes_available = 0;
    ioctl(fd, FIONREAD, &bytes_available);

    if (bytes_available)
        buf_size *= bytes_available;            // This is risky.. what if they wait a long time?
    else
        buf_size *= 10 * (sizeof(struct inotify_event) + NAME_MAX + 1);

    if (max_bytes < 0)
        max_bytes = 0;

    if (max_bytes && buf_size > max_bytes)
        buf_size = max_bytes;

    ssize_t len;
    buf = malloc(buf_size);
    
    Py_BEGIN_ALLOW_THREADS

    len = read(fd, buf, buf_size);

    Py_END_ALLOW_THREADS

    if (len == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    PyObject* event_list = PyList_New(0);      // Check for failure here (NULL)?

    if (len <= 0)     // Zero bytes in nonblocking mode
        return event_list;

    const struct inotify_event* event;

    for (char* ptr = buf; ptr < buf + len;      // Decode events
            ptr += sizeof(struct inotify_event) + event->len) {

        event = (const struct inotify_event*) ptr;

        PyObject* new_parsed_event = PyStructSequence_New(parsed_event_object);

        PyStructSequence_SetItem(
            new_parsed_event, 0, PyLong_FromLong(event->wd));
        PyStructSequence_SetItem(
            new_parsed_event, 1, PyLong_FromUnsignedLong(event->mask));
        PyStructSequence_SetItem(
            new_parsed_event, 2, PyLong_FromUnsignedLong(event->cookie));
        PyStructSequence_SetItem(
            new_parsed_event, 3, PyLong_FromUnsignedLong(event->len));

        if (event->len)
            PyStructSequence_SetItem(
                new_parsed_event, 4, PyUnicode_FromString(event->name));
        else
            PyStructSequence_SetItem(
                new_parsed_event, 4, PyUnicode_FromString(""));

        PyList_Append(event_list, new_parsed_event);       // Check for failure here?
    }

    free(buf);

    return event_list;
}



// static PyObject*
// events_in_mask(PyObject* self, PyObject* args, PyObject* kw) {
//     static const char* kwlist[] = {"mask", NULL};

//     uint32_t mask;

//     if (!PyArg_ParseTupleAndKeywords(
//             args, kw, "I", const_cast<char**>(kwlist), &fd))
//         return NULL;

//     for (uint i = 0; i < 32; ++i) {
//         if
//     }


static PyMethodDef InotifyMethods[] = {
    {
    "init", (PyCFunction)init, METH_NOARGS,
    "init()\n--\n\n"
    "Execute inotify_init(2).\n"
    "\n"
    "Initialize a new inotify instance.\n"
    "\n"
    "Returns:\n"
    "    int: A file descriptor associated with the new inotify event queue.\n"
    "\n"
    "Raises:\n"
    "    OSError: If the inotify_init call fails."
    },

    {
    "init1", (PyCFunction)init1, (METH_VARARGS | METH_KEYWORDS),
    "init1(flags=0)\n--\n\n"
    "Execute inotify_init1(2).\n"
    "\n"
    "Initialize a new inotify instance. IN_NONBLOCK and IN_CLOEXEC can be\n"
    "bitwise ORed in flags to obtain different behavior. Use IN_NONBLOCK to\n"
    "open the file descriptor in nonblocking mode. Use IN_CLOEXEC to enable\n"
    "the close-on-exec flag for the new file descriptor.\n"
    "\n"
    "Args:\n"
    "    flags (int): A bitmask for configuring special behavior.\n"
    "\n"
    "Returns:\n"
    "    int: A file descriptor associated with the new inotify event queue.\n"
    "\n"
    "Raises:\n"
    "    TypeError: If flags is not an integer.\n"
    "    OSError: If the inotify_init1 call fails."
    },

    {
    "add_watch", (PyCFunction)add_watch, (METH_VARARGS | METH_KEYWORDS),
    "add_watch(fd, pathname, mask)\n--\n\n"
    "Execute inotify_add_watch(2).\n"
    "\n"
    "Add a new watch, or modify an existing watch, for the file whose\n"
    "location is specified in pathname; the caller must have read permission\n"
    "for this file.\n"
    "\n"
    "Args:\n"
    "    fd (int): A file descriptor referring to the inotify instance whose\n"
    "        watch list is to be modified.\n"
    "    pathname (str): The pathname to monitor for filesystem events.\n"
    "    mask (int): Specifies the events to be monitored for pathname.\n"
    "\n"
    "Returns:\n"
    "    int: A unique watch descriptor for the inotify instance associated\n"
    "        with fd, for the filesystem object (inode) corresponding to\n"
    "        pathname.\n"
    "\n"
    "Raises:\n"
    "    TypeError: If any arguments are not of the correct type.\n"
    "    OSError: If the inotify_add_watch call fails."
    },

    {
    "rm_watch", (PyCFunction)rm_watch, (METH_VARARGS | METH_KEYWORDS),
    "rm_watch(fd, wd)\n--\n\n"
    "Execute inotify_rm_watch(2).\n"
    "\n"
    "Remove the watch associated with the watch descriptor wd from the\n"
    "inotify instance associated with the file descriptor fd.\n"
    "\n"
    "Args:\n"
    "    fd (int): The file descriptor of the inotify instance to modify.\n"
    "    wd (int): The watch descriptor of the watch to be removed.\n"
    "\n"
    "Returns:\n"
    "    int: 0\n"
    "\n"
    "Raises:\n"
    "    TypeError: If either fd or wd is not an integer."
    "    OSError: If the inotify_rm_watch call fails."
    },

    {
    "read_events", (PyCFunction)read_events, (METH_VARARGS | METH_KEYWORDS),
    "read_events(fd)\n--\n\n"
    "Read and decode all events on the inotify file descriptor fd.\n"
    "\n"
    "If the inotify instance was initialized with IN_NONBLOCK and there\n"
    "are no events on fd, an empty tuple will be returned.\n"
    "\n"
    "Args:\n"
    "    fd (int): An inotify file descriptor.\n"
    "    max_bytes (int): The maximum number of bytes to read. This will\n"
    "        result in an OSError if max_bytes is less than the size of\n"
    "        one event.\n"
    "\n"
    "Returns:\n"
    "    list: A list of Event namedtuples; one for each inotify event in\n"
    "        the order they occurred.\n"
    "\n"
    "Raises:\n"
    "    TypeError: If either fd or max_bytes is not an integer."
    "    OSError: If an error occurs while reading from fd."
    },

    {NULL, NULL, 0, NULL}
};


static PyStructSequence_Field event_fields[] = {    // Event namedtuple pieces
    {"wd", NULL},
    {"mask", NULL},
    {"cookie", NULL},
    {"len", NULL},
    {"name", NULL},
    {NULL}
};


static PyStructSequence_Desc event_namedtuple = {
    "Event",
    NULL,
    event_fields,
    5
};


static struct PyModuleDef inotifymodule = {
    PyModuleDef_HEAD_INIT,
    "inotify",
    NULL,
    -1,
    InotifyMethods
};


PyMODINIT_FUNC
PyInit_inotify(void) {
    parsed_event_object = PyStructSequence_NewType(&event_namedtuple);

    PyObject* module = PyModule_Create(&inotifymodule);

    PyModule_AddIntMacro(module, IN_ACCESS);
    PyModule_AddIntMacro(module, IN_MODIFY);
    PyModule_AddIntMacro(module, IN_ATTRIB);
    PyModule_AddIntMacro(module, IN_CLOSE_WRITE);
    PyModule_AddIntMacro(module, IN_CLOSE_NOWRITE);
    PyModule_AddIntMacro(module, IN_OPEN);
    PyModule_AddIntMacro(module, IN_MOVED_FROM);
    PyModule_AddIntMacro(module, IN_MOVED_TO);
    PyModule_AddIntMacro(module, IN_CREATE);
    PyModule_AddIntMacro(module, IN_DELETE);
    PyModule_AddIntMacro(module, IN_DELETE_SELF);
    PyModule_AddIntMacro(module, IN_MOVE_SELF);

    PyModule_AddIntMacro(module, IN_UNMOUNT);
    PyModule_AddIntMacro(module, IN_Q_OVERFLOW);
    PyModule_AddIntMacro(module, IN_IGNORED);

    PyModule_AddIntMacro(module, IN_CLOSE);
    PyModule_AddIntMacro(module, IN_MOVE);

    PyModule_AddIntMacro(module, IN_ONLYDIR);
    PyModule_AddIntMacro(module, IN_DONT_FOLLOW);
    PyModule_AddIntMacro(module, IN_EXCL_UNLINK);
    PyModule_AddIntMacro(module, IN_MASK_CREATE);
    PyModule_AddIntMacro(module, IN_MASK_ADD);
    PyModule_AddIntMacro(module, IN_ISDIR);
    PyModule_AddIntMacro(module, IN_ONESHOT);
    PyModule_AddIntMacro(module, IN_ALL_EVENTS);

    PyModule_AddIntMacro(module, IN_CLOEXEC);
    PyModule_AddIntMacro(module, IN_NONBLOCK);

    return module;
}