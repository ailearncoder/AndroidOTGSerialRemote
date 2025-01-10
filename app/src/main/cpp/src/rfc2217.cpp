/*
 * MIT License
 *
 * Copyright (c) 2025 by ailearncoder <panxuesen520@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if 0
#!/bin/bash
set -e
termux='/data/data/com.termux/files'
flags="-fno-strict-overflow -Wsign-compare -Wunreachable-code -fstack-protector-strong -O3 -DNDEBUG -g -Wall"
include_flags="-I${termux}/usr/include/python3.12"
ld_flags="-L${termux}/usr/lib -lpython3.12 -ldl -lpthread -lm -L./main.dist -lrfc2217"
gcc -o serial.o -c serial.c $flags $include_flags
g++ -o main.o -c $0 $flags $include_flags
g++ -o main serial.o main.o $ld_flags -Wl,-rpath,./
rm -f serial.o main.o
patchelf --set-rpath . main
cp ./main main.dist/
cd ./main.dist && ./main
exit 0
#endif

#define PY_SSIZE_T_CLEAN
#include <python3.12/Python.h>
#include <stdio.h>
#include <cstdlib>

#include <string>

#include "serial.h"
#include "log.h"

extern "C" {
extern int init_start(const char* binary_filename, const int verbose);
extern void* init_import_module(const char *name);
extern void init_exit();
}

int librfc2217_init(const char* binary_filename, const int verbose) {
    int ret = init_start(binary_filename, verbose);
    LOG_DEBUG("init_start %d\n", ret);
    return ret;
}

static void print_backtrace() {
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);

    if (ptype != NULL) {
        PyObject *type_name = PyObject_GetAttrString(ptype, "__name__");
        const char *type_str = PyUnicode_AsUTF8(type_name);
        PyObject *str_exc = PyObject_Str(pvalue);
        const char *err_msg = PyUnicode_AsUTF8(str_exc);

        LOG_ERROR("Exception Type: %s\n", type_str);
        LOG_ERROR("Error Message: %s\n", err_msg);

        Py_XDECREF(type_name);
        Py_XDECREF(str_exc);

        // Import the traceback module
        PyObject *traceback_module = PyImport_ImportModule("traceback");
        if (traceback_module != nullptr) {
            // Get the format_exception function
            PyObject *format_exception_fn = PyObject_GetAttrString(traceback_module, "format_exception");

            if (format_exception_fn && PyCallable_Check(format_exception_fn)) {
                // Call format_exception with (ptype, pvalue, ptraceback)
                PyObject *formatted_list = PyObject_CallFunctionObjArgs(format_exception_fn, ptype, pvalue, ptraceback, NULL);

                if (formatted_list) {
                    // Join the list into a single string
                    PyObject *str_newline = PyUnicode_FromString("");
                    PyObject *joined_str = PyUnicode_Join(str_newline, formatted_list);
                    const char *traceback_str = PyUnicode_AsUTF8(joined_str);

                    LOG_ERROR("Traceback:\n%s", traceback_str);

                    Py_XDECREF(str_newline);
                    Py_XDECREF(joined_str);
                    Py_XDECREF(formatted_list);
                }
            }
            Py_XDECREF(format_exception_fn);
            Py_XDECREF(traceback_module);
        } else {
            LOG_ERROR("Failed to import traceback module\n");
        }
    }

    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
}

int librfc2217_start(const int port, const int tcpPort, const int verbose = 2) {
    PyObject *pModule = (PyObject *)init_import_module("librfc2217");
    LOG_DEBUG("module %p\n", pModule);
    if (!pModule || PyErr_Occurred()) {
#ifdef __LINUX__
        PyErr_Print();
#endif
#ifdef  __ANDROID__
        print_backtrace();
#endif
        PyErr_Clear();
        return -1;
    }
    
    PyTypeObject *type = Py_TYPE(pModule);
    const char *type_name = type->tp_name;
    LOG_DEBUG("Object type: %s\n", type_name);

    PyObject *dict = PyModule_GetDict(pModule);
    LOG_DEBUG("dict %p\n", dict);

    PyObject *platformModule = PyInit_android();
    if (!platformModule) {
        PyErr_Print();
        PyErr_Clear();
        return -1;
    }

    PyObject *platform = PyObject_GetAttrString(platformModule, "Serial");
    PyObject *serial = PyObject_GetAttrString(pModule, "SerialAndroid");
    PyObject *server = PyObject_GetAttrString(pModule, "RFC2217Server");

    if (!platform || !serial || !server || PyErr_Occurred()) {
        print_backtrace();
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(platformModule);
        return -1;
    }

    LOG_DEBUG("platform %p type %s\n", platform, type_name);
    LOG_DEBUG("serial %p type %s\n", serial, type_name);
    LOG_DEBUG("server %p type %s\n", server, type_name);

    if (!PyCallable_Check(platform)) {
        LOG_ERROR("platform is not callable\n");
        Py_XDECREF(platformModule);
        return -1;
    }

    PyObject *platformInstance = PyObject_CallNoArgs(platform);
    if (!platformInstance || PyErr_Occurred()) {
        print_backtrace();
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(platformModule);
        return -1;
    }

    LOG_DEBUG("platformInstance %p type %s\n", platformInstance, Py_TYPE(platformInstance)->tp_name);

    PyObject* pArgs = PyTuple_New(1);
    PyObject* pKwargs = PyDict_New();
    PyTuple_SetItem(pArgs, 0, platformInstance);

    if (!pArgs || !pKwargs || PyErr_Occurred()) {
        print_backtrace();
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(platformInstance);
        Py_XDECREF(platformModule);
        return -1;
    }

    if (!PyCallable_Check(serial)) {
        LOG_ERROR("serial is not callable\n");
        Py_XDECREF(pArgs);
        Py_XDECREF(pKwargs);
        Py_XDECREF(platformInstance);
        Py_XDECREF(platformModule);
        return -1;
    }

    PyObject *serialInstance = PyObject_Call(serial, pArgs, pKwargs);
    if (!serialInstance || PyErr_Occurred()) {
        print_backtrace();
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pArgs);
        Py_XDECREF(pKwargs);
        Py_XDECREF(platformInstance);
        Py_XDECREF(platformModule);
        return -1;
    }

    LOG_DEBUG("serialInstance %p type %s\n", serialInstance, Py_TYPE(serialInstance)->tp_name);

    std::string portStr = "rfc2217:///dev/ttyUSB" + std::to_string(port);
    PyObject_SetAttrString(serialInstance, "port", PyUnicode_FromString(portStr.c_str()));

    PyObject* pArgs2 = PyTuple_New(1);
    PyObject* pKwargs2 = PyDict_New();
    PyTuple_SetItem(pArgs2, 0, serialInstance);

    PyDict_SetItemString(pKwargs2, "local_port", PyLong_FromLong(tcpPort));
    PyDict_SetItemString(pKwargs2, "verbosity", PyLong_FromLong(verbose));
    PyDict_SetItemString(pKwargs2, "r0", PyBool_FromLong(0));

    PyObject *serverInstance = PyObject_Call(server, pArgs2, pKwargs2);
    LOG_DEBUG("serverInstance %p type %s\n", serverInstance, Py_TYPE(serverInstance)->tp_name);

    if (!serverInstance || PyErr_Occurred()) {
        print_backtrace();
        PyErr_Print();
        PyErr_Clear();
        Py_XDECREF(pArgs2);
        Py_XDECREF(pKwargs2);
        Py_XDECREF(serialInstance);
        Py_XDECREF(platformInstance);
        Py_XDECREF(platformModule);
        return -1;
    }

    PyObject_CallMethod(serverInstance, "start_server", nullptr);
    if (PyErr_Occurred()) {
        print_backtrace();
        PyErr_Print();
        PyErr_Clear();
    }

    Py_XDECREF(pArgs2);
    Py_XDECREF(pKwargs2);
    Py_XDECREF(serverInstance);
    Py_XDECREF(serialInstance);
    Py_XDECREF(platformInstance);
    Py_XDECREF(platformModule);

    return 0;
}

extern "C" {
    int librfc2217_init_c(const char* binary_filename) {
        return librfc2217_init(binary_filename, 0);
    }
    int librfc2217_start_c(const int port, const int tcpPort, const int verbose) {
        return librfc2217_start(port, tcpPort, verbose);
    }
}
