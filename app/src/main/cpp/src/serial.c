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

#define PY_SSIZE_T_CLEAN
#include <stdbool.h>
#include <jni.h>
#include "serial.h"
#include "log.h"
#include "java_method.h"

typedef struct 
{
    int baudrate;
    char parity;
    int bytesize;
    int stopbits;
} SerialParams;

typedef struct
{
    PyObject_HEAD;
    bool opened; // 是否已打开
    int rts_state;        // RTS状态
    int dtr_state;        // DTR状态
    SerialParams params;
} SerialObject;

// 类方法定义 TODO:
static PyObject *Serial_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SerialObject *self;
    self = (SerialObject *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->opened = false;
        self->rts_state = 0;
        self->dtr_state = 0;
        memset(&self->params, 0, sizeof(SerialParams));
    }
    LOG_DEBUG("self:%p %p %p", self, args, kwds);
    return (PyObject *)self;
}

// TODO: 析构函数
static void Serial_dealloc(SerialObject *self)
{
    LOG_DEBUG("self:%p", self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

// def open(self, port:str) -> tuple[bool, str]:
static PyObject *Serial_open(SerialObject *self, PyObject *args)
{
    const char* port;
    if (!PyArg_ParseTuple(args, "s", &port)) {
        return NULL; // 参数解析失败时返回NULL
    }

    LOG_DEBUG("port:%s", port);

    // 模拟的函数逻辑
    int success = JavaMethod_OpenSerial(0);
    const char* message = (success == 1) ? "Operation successful" : "Operation failed";

    LOG_DEBUG("success:%d, message:%s", success, message);
    
//    if (success != 0) {
//        PyErr_SetString(PyExc_RuntimeError, message);
//        return NULL;
//    }

    self->opened = success == 1;

    // 构造并返回元组 (bool, str)
    PyObject* result = PyTuple_Pack(2, PyBool_FromLong(self->opened), PyUnicode_FromString(message));
    return result;
}

static PyObject *Serial_close(SerialObject *self, PyObject *args)
{
    LOG_DEBUG("%p", args);
    JavaMethod_CloseSerial(0); // 关闭串口
    self->opened = false;
    Py_RETURN_NONE;
}

// def reconfigure(baudrate:int, parity:str, bytesize:int, stopbits:int, xonxoff:bool, rtscts:bool, timeout:float):
static PyObject* Serial_reconfigure(SerialObject* self, PyObject* args) {
    const char* parity;
    int xonxoff;
    int rtscts;
    float timeout;
    SerialParams params = {0};
    
    if (!PyArg_ParseTuple(args, "isiiiif", &params.baudrate, &parity, &params.bytesize, &params.stopbits,
                          &xonxoff, &rtscts, &timeout)) {
        PyErr_SetString(PyExc_TypeError, "Invalid input parameters");
        return NULL;
    }
    params.parity = parity[0]; // 取第一个字符作为串行端口奇偶校验

    LOG_DEBUG("%p, baudrate: %d, parity: %s, bytesize: %d, stopbits: %d, xonxoff: %d, rtscts: %d, timeout: %f",
              self, params.baudrate, parity, params.bytesize, params.stopbits, xonxoff, rtscts, timeout);

    // Add your reconfiguration logic here
    if(memcmp((const void*)&self->params, (const void*)&params, sizeof(SerialParams)) != 0) {
        JavaMethod_ConfigureSerial(0, params.baudrate, params.bytesize, 1, params.parity); // xonxoff, rtscts, timeout
        self->params = params;
    }

    Py_RETURN_NONE;
}

// RTS属性
static PyObject *Serial_get_rts_state(SerialObject *self, void *closure)
{
    self->rts_state = JavaMethod_RtsSerialGet(0);
    LOG_DEBUG("%p %d", closure, self->rts_state);
    return PyBool_FromLong(self->rts_state);
}

static int Serial_set_rts_state(SerialObject *self, PyObject *value, void *closure)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete rts_state");
        return -1;
    }
    self->rts_state = PyObject_IsTrue(value);
    LOG_DEBUG("%p %d", closure, self->rts_state);
    JavaMethod_RtsSerialSet(0, self->rts_state);
    return 0;
}

// DTR属性
static PyObject *Serial_get_dtr_state(SerialObject *self, void *closure)
{
    self->dtr_state = JavaMethod_DtrSerialGet(0);
    LOG_DEBUG("%p %d", closure, self->dtr_state);
    return PyBool_FromLong(self->dtr_state);
}

static int Serial_set_dtr_state(SerialObject *self, PyObject *value, void *closure)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete dtr_state");
        return -1;
    }
    self->dtr_state = PyObject_IsTrue(value);
    LOG_DEBUG("%p %d", closure, self->dtr_state);
    JavaMethod_DtrSerialSet(0, self->dtr_state);
    return 0;
}

// in_waiting属性
static PyObject *Serial_get_in_waiting(SerialObject *self, void *closure)
{
    int size = JavaMethod_InWaitingSerial(0);
    LOG_DEBUG("%p %p size: %d", closure, self, size);
    return PyLong_FromLong(size);
}

// TODO: out_waiting属性
static PyObject *Serial_get_out_waiting(SerialObject *self, void *closure)
{
    LOG_DEBUG("%p size: %d", closure, 0);
    return PyLong_FromLong(0);
}

// 状态线属性
static PyObject *Serial_get_cts(SerialObject *self, void *closure)
{
    int status = JavaMethod_StatusSerial(0, "cts");
    if (-1 == status) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get CTS status, Serial port not open");
        LOG_WARN("Failed to get CTS status, Serial port not open");
        return NULL;
    }
    LOG_DEBUG("%p %p %d", self, closure, status);
    return PyBool_FromLong(status);
}

static PyObject *Serial_get_dsr(SerialObject *self, void *closure)
{
    int status = JavaMethod_StatusSerial(0, "dsr");
    if (-1 == status) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get DSR status, Serial port not open");
        LOG_WARN("Failed to get DSR status, Serial port not open");
        return NULL;
    }
    LOG_DEBUG("%p %p %d", self, closure, status);
    return PyBool_FromLong(status);
}

static PyObject *Serial_get_ri(SerialObject *self, void *closure)
{
    int status = JavaMethod_StatusSerial(0, "ri");
    if (-1 == status) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get RI status, Serial port not open");
        LOG_WARN("Failed to get RI status, Serial port not open");
        return NULL;
    }
    LOG_DEBUG("%p %p %d", self, closure, status);
    return PyBool_FromLong(status);
}

static PyObject *Serial_get_cd(SerialObject *self, void *closure)
{
    int status = JavaMethod_StatusSerial(0, "cd");
    if (-1 == status) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get CD status, Serial port not open");
        LOG_WARN("Failed to get CD status, Serial port not open");
        return NULL;
    }
    LOG_DEBUG("%p %p %d", self, closure, status);
    return PyBool_FromLong(status);
}

// 读写方法
static PyObject *Serial_read(SerialObject *self, PyObject *args, PyObject *kwds)
{
    int size = 1;
    PyObject *timeout_obj = Py_None;
    if (!self->opened) {
        LOG_WARN("Serial port not open");
        PyErr_SetString(PyExc_RuntimeError, "Serial port not open");
        return NULL;
    }
    LOG_DEBUG("enter");
    static char *kwlist[] = {"size", "timeout", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iO", kwlist, &size, &timeout_obj)) {
        LOG_WARN("Invalid input parameters");
        PyErr_SetString(PyExc_TypeError, "Invalid input parameters, size is not an int");
        return NULL;
    }
    float timeout = 0.0f;
    if (timeout_obj != Py_None) {
        if(PyFloat_Check(timeout_obj)) {
            timeout = (float)PyFloat_AsDouble(timeout_obj);
        } else if(PyLong_Check(timeout_obj)) {
            timeout = (float)PyLong_AsLong(timeout_obj);
        } else {
            LOG_WARN("Invalid input parameters, timeout type: %s", Py_TYPE(timeout_obj)->tp_name);
            PyErr_SetString(PyExc_TypeError, "Invalid input parameters");
            return NULL;
        }
    }
    PyObject* res = NULL;
    int8_t *data = NULL;
    int read_size = 0;
    Py_BEGIN_ALLOW_THREADS
    read_size = JavaMethod_ReadSerial(0, size, (int)(timeout * 1000), &data);
    Py_END_ALLOW_THREADS
    if (read_size < 0) {
        LOG_WARN("Read error");
        PyErr_SetString(PyExc_RuntimeError, "Read error");
        return NULL;
    }
    res = PyBytes_FromStringAndSize((const char*)data, read_size);
    if (data) {
        free(data);
        data = NULL;
    }
#if 0
    LOG_DEBUG("size: %d/%d, timeout: %.2f", size, self->buffer_size - self->position, timeout);
    PyObject* res = NULL;
    if (self->position < self->buffer_size) {
        // 防止越界
        if (self->position + size > self->buffer_size) {
            size = self->buffer_size - self->position;
        }
        res = PyBytes_FromStringAndSize(self->buffer + self->position, size);
        self->position += size;
    } else {
        res = PyBytes_FromStringAndSize("", 0);
        // Save the current thread state and release the GIL（全局解释器锁）
        // PyThreadState* _save = PyEval_SaveThread();
        Py_BEGIN_ALLOW_THREADS
        sleep(timeout);
        usleep((int)((timeout - (int)timeout) * 1000000));
        Py_END_ALLOW_THREADS
        // PyEval_RestoreThread(_save);
    }
#endif
    return res;
}

// def write(self, data, timeout=None):
static PyObject *Serial_write(SerialObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *data;
    PyObject *timeout_obj = Py_None;

    static char *kwlist[] = {"data", "timeout", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &data, &timeout_obj)) {
        PyErr_SetString(PyExc_TypeError, "Invalid input parameters");
        return NULL;
    }
    if (!PyBytes_Check(data)) {
        PyErr_SetString(PyExc_TypeError, "Data must be bytes");
        return NULL;
    }
    float timeout = 0.0f;
    if (timeout_obj != Py_None) {
        if(PyFloat_Check(timeout_obj)) {
            timeout = (float)PyFloat_AsDouble(timeout_obj);
        } else if(PyLong_Check(timeout_obj)) {
            timeout = (float)PyLong_AsLong(timeout_obj);
        } else {
            LOG_WARN("Invalid input parameters, timeout type: %s", Py_TYPE(timeout_obj)->tp_name);
            PyErr_SetString(PyExc_TypeError, "Timeout must be float or int");
            return NULL;
        }
    }
    int size = (int)PyBytes_Size(data);
    if (size > 0) {
        void *data_ptr = PyBytes_AsString(data);
        LOG_DEBUG("%p data:%p size: %d, timeout: %.2f", self, data_ptr, size, timeout);
        Py_BEGIN_ALLOW_THREADS
        size = JavaMethod_WriteSerial(0, data_ptr, size, (int)(timeout * 1000));
        Py_END_ALLOW_THREADS
    }
    if (size < 0) {
        LOG_WARN("Write error");
        PyErr_SetString(PyExc_RuntimeError, "Write error");
        return NULL;
    }
#if 0
    // print data hex for debug
    for (int i = 0; i < size; ++i) {
        printf("%.2x ", ((unsigned char *)data_ptr)[i]);
    }
    printf("\n");
#endif
    return PyLong_FromLong(size);
}

// TODO: 其他控制方法
static PyObject *Serial_cancel_read(SerialObject *self, PyObject *Py_UNUSED(args))
{
    LOG_DEBUG("%p", self);
    Py_RETURN_NONE;
}

// TODO: 其他控制方法
static PyObject *Serial_cancel_write(SerialObject *self, PyObject *Py_UNUSED(args))
{
    LOG_DEBUG("%p", self);
    Py_RETURN_NONE;
}

// TODO: 其他控制方法
static PyObject *Serial_flush(SerialObject *self, PyObject *Py_UNUSED(args))
{
    LOG_DEBUG("%p", self);
    Py_RETURN_NONE;
}

// 清除串口接收
static PyObject *Serial_reset_input_buffer(SerialObject *self, PyObject *Py_UNUSED(args))
{
    LOG_DEBUG("%p", self);
    JavaMethod_ResetInputBufferSerial(0);
    Py_RETURN_NONE;
}

// TODO: 其他控制方法
static PyObject *Serial_reset_output_buffer(SerialObject *self, PyObject *Py_UNUSED(args))
{
    LOG_DEBUG("%p", self);
#if 0
    self->buffer = "hello\n"; // 模拟数据
    self->position = 0;
    self->buffer_size = 6;
#endif
    Py_RETURN_NONE;
}

// TODO: 其他控制方法
static PyObject * Serial_send_break(SerialObject *self, PyObject *args)
{
    LOG_DEBUG("%p", self);
    double duration = 0.25;
    if (!PyArg_ParseTuple(args, "|d", &duration))
        return NULL;
    Py_RETURN_NONE;
}

// TODO: 其他控制方法
static PyObject * Serial_set_input_flow_control(SerialObject *self, PyObject *args)
{
    LOG_DEBUG("%p", self);
    int enable = 1;
    if (!PyArg_ParseTuple(args, "|p", &enable))
        return NULL;
    Py_RETURN_NONE;
}

// TODO: 其他控制方法
static PyObject * Serial_set_output_flow_control(SerialObject *self, PyObject *args)
{
    LOG_DEBUG("%p", self);
    int enable = 1;
    if (!PyArg_ParseTuple(args, "|p", &enable))
        return NULL;
    Py_RETURN_NONE;
}

static PyObject * Serial_log_print(SerialObject *self, PyObject *args)
{
    const char *msg;
    if (!PyArg_ParseTuple(args, "s", &msg) || self == NULL) {
        PyErr_SetString(PyExc_TypeError, "Invalid input parameters");
        return NULL;
    }
    __android_log_print(ANDROID_LOG_INFO, "python", "%s", msg);
    Py_RETURN_NONE;
}

// 属性定义
static PyGetSetDef Serial_getsetters[] = {
    {"rts_state", (getter)Serial_get_rts_state, (setter)Serial_set_rts_state, "RTS state", NULL},
    {"dtr_state", (getter)Serial_get_dtr_state, (setter)Serial_set_dtr_state, "DTR state", NULL},
    {"in_waiting", (getter)Serial_get_in_waiting, NULL, "Bytes in input buffer", NULL},
    {"out_waiting", (getter)Serial_get_out_waiting, NULL, "Bytes in output buffer", NULL},
    {"cts", (getter)Serial_get_cts, NULL, "CTS state", NULL},
    {"dsr", (getter)Serial_get_dsr, NULL, "DSR state", NULL},
    {"ri", (getter)Serial_get_ri, NULL, "RI state", NULL},
    {"cd", (getter)Serial_get_cd, NULL, "CD state", NULL},
    {NULL}};

// 方法定义
static PyMethodDef Serial_methods[] = {
    {"open", (PyCFunction)Serial_open, METH_VARARGS, "Open a port and return a tuple (bool, str)."},
    {"close", (PyCFunction)Serial_close, METH_NOARGS, "Close port"},
    {"reconfigure", (PyCFunction)Serial_reconfigure, METH_VARARGS, "Reconfigure port"},
    {"read", (PyCFunction)Serial_read, METH_VARARGS | METH_KEYWORDS, "Read data"},
    {"write", (PyCFunction)Serial_write, METH_VARARGS | METH_KEYWORDS, "Write data"},
    {"cancel_read", (PyCFunction)Serial_cancel_read, METH_NOARGS, "Cancel read"},
    {"cancel_write", (PyCFunction)Serial_cancel_write, METH_NOARGS, "Cancel write"},
    {"flush", (PyCFunction)Serial_flush, METH_NOARGS, "Flush buffers"},
    {"reset_input_buffer", (PyCFunction)Serial_reset_input_buffer, METH_NOARGS, "Clear input buffer"},
    {"reset_output_buffer", (PyCFunction)Serial_reset_output_buffer, METH_NOARGS, "Clear output buffer"},
    {"send_break", (PyCFunction)Serial_send_break, METH_VARARGS, "Send break"},
    {"set_input_flow_control", (PyCFunction)Serial_set_input_flow_control, METH_VARARGS, "Set input flow control"},
    {"set_output_flow_control", (PyCFunction)Serial_set_output_flow_control, METH_VARARGS, "Set output flow control"},
    {"log_print", (PyCFunction)Serial_log_print, METH_VARARGS, "Log print"},
    {NULL}};

// 类型定义
static PyTypeObject SerialType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "android.Serial",
    .tp_doc = "Serial port class",
    .tp_basicsize = sizeof(SerialObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = Serial_new,
    .tp_dealloc = (destructor)Serial_dealloc,
    .tp_methods = Serial_methods,
    .tp_getset = Serial_getsetters,
};

// 模块定义
static PyModuleDef serialmodule = {
    PyModuleDef_HEAD_INIT,
    "android",
    "Android Serial port module",
    -1,
};

// 模块初始化
PyMODINIT_FUNC PyInit_android(void)
{
    PyObject *m;

    if (PyType_Ready(&SerialType) < 0)
        return NULL;

    m = PyModule_Create(&serialmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&SerialType);
    if (PyModule_AddObject(m, "Serial", (PyObject *)&SerialType) < 0)
    {
        Py_DECREF(&SerialType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
