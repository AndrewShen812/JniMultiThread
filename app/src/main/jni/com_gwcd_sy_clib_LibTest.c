#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "com_gwcd_sy_clib_LibTest.h"
#include "AndroidLog.h"

#define DEF_DELAY 1

/**
要解决的问题：jni使用多线程加载数据。
但JNIEnv是一个与线程相关的变量，即线程A有一个JNIEnv变量， 线程B也有一个JNIEnv变量，由于线程相关，所以A线程不能使用B线程的JNIEnv结构体变量。
方法：
1.在JNI_OnLoad中，保存JavaVM*，这是跨线程的，持久有效的，而JNIEnv*则是当前线程有效的。
一旦启动线程，用AttachCurrentThread方法获得在当前线程有效的env。
2.通过JavaVM*和JNIEnv可以查找到jclass。
3.把jclass转成全局引用，使其跨线程。
4.delete掉创建的全局引用和调用DetachCurrentThread方法。
*/
static JavaVM* g_jvm = NULL;

// 全局使用的变量，在进入JNI接口时用NewGlobalRef保存起来
static jclass g_clazz = NULL;

// 全局使用的变量，在进入JNI接口时用NewGlobalRef保存起来
static jobject g_object = NULL;

static pthread_mutex_t g_mutex;

typedef struct long_time_func_param {

    unsigned char* thread_name;

    int err;

} thread_param_t;

/*
 * Class:     com_gwcd_sy_clib_LibTest
 * Method:    nativeInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_gwcd_sy_clib_LibTest_nativeInit
  (JNIEnv *env, jclass cls)
{
    if (0 != pthread_mutex_init(&g_mutex, NULL)) {
        LOGE("互斥锁初始化失败");
    } else {
        LOGD("互斥锁初始化成功");
    }
}

/*
 * Class:     com_gwcd_sy_clib_LibTest
 * Method:    nativeRelease
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_gwcd_sy_clib_LibTest_nativeRelease
    (JNIEnv *env, jclass cls)
{
    if(0 != pthread_mutex_destroy(&g_mutex)) {
        LOGE("互斥锁销毁失败");
    } else {
        LOGD("互斥锁销毁成功");
    }
}

/** 在子线程中执行的耗时方法 */
void* long_time_func(void * args)
{

    if (0 != pthread_mutex_lock(&g_mutex)) {
        LOGE("锁定互斥锁失败");
    }


    LOGD("into long_time_func");
    JNIEnv *env = NULL;

    // 必须调用,将当前线程附加到Java虚拟机上，并或得JNIEnv指针
    if ((*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL) == 0) {
        LOGD("AttachCurrentThread success");
        thread_param_t* param = (thread_param_t*) args;

        LOGD("%s", param->thread_name);
        // 等待3秒，模拟耗时操作
        sleep(DEF_DELAY);
        LOGD("sleep 3s finish");

        jmethodID methodID = (*env)->GetStaticMethodID(env, g_clazz, "JniCallback", "(III)V");
        (*env)->CallStaticVoidMethod(env, g_clazz, methodID, 4, 1677217, param->err); // 模拟返回一些测试数据



        // 将线程从Java虚拟机上剥离
        (*g_jvm)->DetachCurrentThread(g_jvm);
        free(param);
        // TODO:复杂的代码结构下，g_clazz/g_object全局变量的释放问题

        LOGD("finish LongTimeTask");
    } else {
        LOGE("AttachCurrentThread fail");
    }

    if (0 != pthread_mutex_unlock(&g_mutex)) {
        LOGE("解锁互斥锁失败");
    }

    return (void *)1;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGD("into JNI_OnLoad");
    g_jvm = vm;

    return JNI_VERSION_1_4;
}

/*
 * Class:     com_gwcd_sy_clib_LibTest
 * Method:    LongTimeTask
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_gwcd_sy_clib_LibTest_LongTimeTask(JNIEnv *env, jclass clazz)
{
    LOGD("into LongTimeTask");
    // 模拟耗时，会阻塞主线程
    sleep(DEF_DELAY);
    jmethodID methodID = (*env)->GetStaticMethodID(env, clazz, "JniCallback", "(III)V");
    (*env)->CallStaticVoidMethod(env, clazz, methodID, 4, 1677217, 1);
}

/*
 * Class:     com_gwcd_sy_clib_LibTest
 * Method:    LongTimeTask2
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_gwcd_sy_clib_LibTest_LongTimeTask2(JNIEnv *env, jclass clazz)
{
    /**
    env->GetJavaVM(&g_jvm); //保存到全局变量中JVM
    //如果是传入jobject，应该调用以下函数，创建可以在其他线程中使用的引用:
    g_object=env->NewGlobalRef(obj);
    */
    // 创建全局引用
    g_clazz = (jclass)(*env)->NewGlobalRef(env, clazz);
    LOGD("into LongTimeTask2");
    int i;
    for (i = 0; i < 10; ++i) {
        thread_param_t* param = (thread_param_t*) malloc(sizeof(thread_param_t));
        memset(param, 0, sizeof(thread_param_t));
        param->thread_name = (unsigned char*)malloc(255);
        sprintf(param->thread_name, "thread id:%d", (i + 1));
        param->err = (i + 1);
        pthread_t thread;
        pthread_attr_t p_attr;
        pthread_attr_init(&p_attr);
        // TODO:设置调度策略

        int n = pthread_create(&thread, &p_attr, (void *)long_time_func, (void *)param);//启动线程，调用run_task方法
        if (n != 0) {
            LOGE("线程创建失败");
        } else {
            LOGD("线程创建成功");
        }
    }
}
