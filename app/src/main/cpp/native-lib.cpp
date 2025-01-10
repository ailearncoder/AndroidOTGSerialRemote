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

#include <jni.h>
#include <string>
#include <cstdarg>
#include <functional>
#include <mutex>

#include "java_method.h"

#define LOG_LEVEL LOG_LEVEL_WARN
#include "log.h"

extern "C" {
    int librfc2217_init_c(const char* binary_filename);
    int librfc2217_start_c(const int port, const int tcpPort, const int verbose);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_cc_axyz_serialserver_SerialService_stringFromJNI(JNIEnv *env, jobject thiz) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_cc_axyz_serialserver_SerialService_rfc2217Init(JNIEnv *env, jobject thiz,
                                                   jstring lib_path) {
    const char *nativeString = env->GetStringUTFChars(lib_path, nullptr);
    setenv("LD_LIBRARY_PATH", nativeString, 1);
    setenv("DYLD_LIBRARY_PATH", nativeString, 1);
    setenv("PYTHONPATH", nativeString, 1);
    std::string binary_filename = nativeString;
    binary_filename.append("/main");
    librfc2217_init_c(binary_filename.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_cc_axyz_serialserver_SerialService_rfc2217Start(JNIEnv *env, jobject thiz, jint port, jint tcpPort, jint verbose) {
    librfc2217_start_c(port, tcpPort, verbose);
}

/*
 * This is called by the VM when the shared library is first loaded.
 */

typedef union {
    JNIEnv *env;
    void *venv;
} UnionJNIEnvToVoid;

static UnionJNIEnvToVoid uenv;
static JavaVM *g_vm;
static jobject classLoader;
static jclass serialClass;
static std::mutex mutex;

// https://zhuanlan.zhihu.com/p/157890838
// https://developer.android.com/training/articles/perf-jni#faq:-why-didnt-findclass-find-my-class
// https://cloud.tencent.com/developer/article/2379362
// https://blog.csdn.net/m0_59162559/article/details/138352702
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    uenv.venv = nullptr;
    jint result = -1;
    JNIEnv *env = nullptr;

    LOG_DEBUG("JNI_OnLoad");
    g_vm = vm;

    if ((*vm).GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK) {
        LOG_DEBUG("ERROR: GetEnv failed");
        return -1;
    }
    env = uenv.env;
    /*
    if (init_Exec(env) != JNI_TRUE) {
        LOGE("ERROR: init of Exec failed");
        goto bail;
    }

    if (init_FileCompat(env) != JNI_TRUE) {
        LOGE("ERROR: init of Exec failed");
        goto bail;
    }
*/
    result = JNI_VERSION_1_4;

    jclass clazz = env->FindClass("cc/axyz/serialserver/Serial");
    jmethodID methodId = env->GetStaticMethodID(clazz, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject loader = env->CallStaticObjectMethod(clazz, methodId);
    classLoader = env->NewGlobalRef(loader);

    jclass pClass = env->FindClass("cc/axyz/serialserver/Serial");
    serialClass = static_cast<jclass>(env->NewGlobalRef(pClass));

    return result;
}

static int get_env(JNIEnv **env) {
    int attached = 0;
    int status = (*g_vm).GetEnv((void **) env, JNI_VERSION_1_4);
    if (status < 0) {
        LOG_DEBUG("callback_handler:failed to get JNI environment assuming native thread");
        status = (*g_vm).AttachCurrentThread(env, nullptr);
        if (status < 0) {
            LOG_ERROR("callback_handler: failed to attach current thread");
            return 0;
        }
        attached = 1;
    }
    return attached;
}

template<typename T, typename... Args>
static T callMethod(const T ret_value, const char *method_name, const char *signature,
                    std::function<T(JNIEnv*, jclass, jmethodID, Args...)> callFunc,
                    Args... args) {
    T ret = ret_value;
    JNIEnv *env = nullptr;
    jclass pClass = nullptr;
    jmethodID method = nullptr;
    int attached = get_env(&env);

    LOG_DEBUG("Getting JNIEnv %p %d", env, attached);

    if (!env) {
        if (attached) {
            g_vm->DetachCurrentThread();
        }
        return ret;
    }
#if 0
    if(attached) {
        jclass classLoaderClass = env->GetObjectClass(classLoader);
        jmethodID loadClassMethod = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        jstring className = env->NewStringUTF("cc/axyz/serialserver/Serial");
        pClass = (jclass)env->CallObjectMethod(classLoader, loadClassMethod,
                                                     className);
    } else {
        pClass = env->FindClass("cc/axyz/serialserver/Serial");
    }
#else
    pClass = serialClass;
#endif
    if (!pClass) {
        LOG_ERROR("Failed to find class 'cc/axyz/serialserver/Serial' for method %s",
                  method_name);
        if (attached) {
            g_vm->DetachCurrentThread();
        }
        return ret;
    }

    LOG_DEBUG("Found class 'cc/axyz/serialserver/Serial' for method %s", method_name);

    method = env->GetStaticMethodID(pClass, method_name, signature);
    if (!method) {
        LOG_ERROR("Failed to find method %s with signature %s", method_name, signature);
        if (attached) {
            g_vm->DetachCurrentThread();
        }
        return ret;
    }

    LOG_DEBUG("Found method %s with signature %s", method_name, signature);

    ret = callFunc(env, pClass, method, args...);

    if (attached) {
        g_vm->DetachCurrentThread();
    }

    return ret;
}

extern "C" {
// cc.axyz.serialserver.Serial.openSerial(int id)
int JavaMethod_OpenSerial(int id) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("");
    std::function<int(JNIEnv *, jclass, jmethodID)> call_func = [id](JNIEnv *env, jclass cls,
                                                                        jmethodID mid) -> jint {
        return env->CallStaticIntMethod(cls, mid, id);
    };
    return callMethod(-65535, "openSerial", "(I)I", call_func);
}

int JavaMethod_CloseSerial(int id) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("");
    std::function<int(JNIEnv *, jclass, jmethodID)> call_func = [id](JNIEnv *env, jclass cls, jmethodID mid) -> jint {
        return env->CallStaticIntMethod(cls, mid, id);
    };
    return callMethod(-65535, "closeSerial", "(I)I", call_func);
}

// configureSerial(id: Int, baudRate: Int, dataBits: Int, stopBits: Float, parity: Char)
int JavaMethod_ConfigureSerial(int id, int baudRate, int dataBits, float stopBits, char parity) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("baudRate: %d, dataBits: %d, stopBits: %.2f, parity: %c", baudRate, dataBits, stopBits, parity);
    std::function<int(JNIEnv *, jclass, jmethodID)> call_func = [&](
            JNIEnv *env, jclass cls, jmethodID mid) -> jint {
        return env->CallStaticIntMethod(cls, mid, id, baudRate, dataBits, stopBits,parity);
    };
    return callMethod(-65535, "configureSerial", "(IIIFC)I", call_func);
}

// fun readSerial(id: Int, size: Int, timeout: Int) : ByteArray
int JavaMethod_ReadSerial(int id, int size, int timeout, int8_t **data) {
    std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("size: %d, timeout: %d", size, timeout);
    JNIEnv *env = nullptr;
    int attached = get_env(&env);
    *data = nullptr;
    jbyteArray result = nullptr;
    jmethodID pMethod = env->GetStaticMethodID(serialClass, "readSerial", "(III)[B");
    result = static_cast<jbyteArray>(env->CallStaticObjectMethod(serialClass, pMethod, id, size,
                                                                 timeout));
    if (result == nullptr) {
        LOG_ERROR("Failed to read serial with id %d", id);
        return -1;
    }
    LOG_DEBUG("result: %p", result);
    int length = 0;
    length = env->GetArrayLength(result);
    jbyte *bytes = env->GetByteArrayElements(result, nullptr);
    *data = (int8_t *) malloc(length);
    memcpy(*data, bytes, length);
    env->ReleaseByteArrayElements(result, bytes, JNI_OK);
    if (attached) {
        g_vm->DetachCurrentThread();
    }
    return length;
}

// fun writeSerial(id: Int, data : ByteArray, timeout: Int) : Int
int JavaMethod_WriteSerial(int id, int8_t *data, int length, int timeout) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("data: %p, length: %d, timeout: %d", data, length, timeout);
    jbyteArray j_data = nullptr;
    JNIEnv *env = nullptr;
    int attached = get_env(&env);
    j_data = env->NewByteArray(length);
    env->SetByteArrayRegion(j_data, 0, length, (const jbyte *) data);
    jmethodID pMethod = env->GetStaticMethodID(serialClass, "writeSerial", "(I[BI)I");
    int ret = env->CallStaticIntMethod(serialClass, pMethod, id, j_data, timeout);
    if (attached) {
        g_vm->DetachCurrentThread();
    }
    return ret;
}

// fun rtsSerialSet(id: Int, state: Boolean) : Int
int JavaMethod_RtsSerialSet(int id, bool state) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("state: %d", state);
    std::function<jint(JNIEnv *, jclass, jmethodID)> call_func = [&](
            JNIEnv *env, jclass cls, jmethodID mid) -> jint {
        return env->CallStaticIntMethod(cls, mid, id, (jboolean)state);
    };
    return callMethod(-65535, "rtsSerialSet", "(IZ)I", call_func);
}

// fun rtsSerialGet(id: Int) : Boolean
bool JavaMethod_RtsSerialGet(int id) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("");
    std::function<jboolean(JNIEnv *, jclass, jmethodID)> call_func = [&](
            JNIEnv *env, jclass cls, jmethodID mid) -> jboolean {
        return env->CallStaticBooleanMethod(cls, mid, id);
    };
    jboolean result = JNI_FALSE;
    result = callMethod(result, "rtsSerialGet", "(I)Z", call_func);
    return (result == JNI_TRUE);
}

// fun dtrSerialSet(id: Int, state: Boolean) : Int
int JavaMethod_DtrSerialSet(int id, bool state) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("state: %d", state);
    std::function<jint(JNIEnv *, jclass, jmethodID)> call_func = [&](
            JNIEnv *env, jclass cls, jmethodID mid) -> jint {
        return env->CallStaticIntMethod(cls, mid, id, (jboolean)state);
    };
    return callMethod(-65535, "dtrSerialSet", "(IZ)I", call_func);
}

// fun dtrSerialGet(id: Int) : Boolean
bool JavaMethod_DtrSerialGet(int id) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("");
    std::function<jboolean(JNIEnv *, jclass, jmethodID)> call_func = [&](
            JNIEnv *env, jclass cls, jmethodID mid) -> jboolean {
        return env->CallStaticBooleanMethod(cls, mid, id);
    };
    jboolean result = JNI_FALSE;
    result = callMethod(result, "dtrSerialGet", "(I)Z", call_func);
    return (result == JNI_TRUE);
}

// fun statusSerial(id: Int, name: String) : Boolean
int JavaMethod_StatusSerial(int id, const char *name) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("name: %s", name);
    jstring jName = nullptr;
    JNIEnv *env = nullptr;
    int attached = get_env(&env);
    jName = env->NewStringUTF(name);
    jmethodID pMethod = env->GetStaticMethodID(serialClass, "statusSerial", "(ILjava/lang/String;)I");
    jint result = env->CallStaticIntMethod(serialClass, pMethod, id, jName);
    if (attached) {
        g_vm->DetachCurrentThread();
    }
    return result;
}

// fun inWaiting(id: Int) : Int
int JavaMethod_InWaitingSerial(int id) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("");
    jmethodID pMethod = nullptr;
    JNIEnv *env = nullptr;
    int attached = get_env(&env);
    pMethod = env->GetStaticMethodID(serialClass, "inWaitingSerial", "(I)I");
    jint ret = env->CallStaticIntMethod(serialClass, pMethod, id);
    if (attached) {
        g_vm->DetachCurrentThread();
    }
    return ret;
}

// fun resetInputBuffer(id: Int) : Boolean
bool JavaMethod_ResetInputBufferSerial(int id) {
    // std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("");
    jmethodID pMethod = nullptr;
    JNIEnv *env = nullptr;
    int attached = get_env(&env);
    pMethod = env->GetStaticMethodID(serialClass, "resetInputBufferSerial", "(I)Z");
    jboolean ret = env->CallStaticBooleanMethod(serialClass, pMethod, id);
    if (attached) {
        g_vm->DetachCurrentThread();
    }
    return (ret == JNI_TRUE);
}

}
