#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include "com_gwcd_sy_clib_LibTest.h"
#include "AndroidLog.h"
#include "JniEvent.h"

#define DEF_DELAY 1

/**
要解决的问题：jni使用多线程加载数据。
但JNIEnv是一个与线程相关的变量，即线程A有一个JNIEnv变量， 线程B也有一个JNIEnv变量，由于线程相关，所以A线程不能使用B线程的JNIEnv结构体变量。
方法：
1.在JNI_OnLoad中，保存JavaVM*，这是跨线程的，持久有效的，而JNIEnv*则是当前线程有效的。
一旦启动线程，用AttachCurrentThread方法获得在当前线程有效的env。
2.通过JavaVM*和JNIEnv可以查找到jclass。
3.把jclass转成全局引用，使其跨线程。
4.free掉创建的全局引用和调用DetachCurrentThread方法。
*/
static JavaVM* g_jvm = NULL;

// 全局使用的变量，在进入JNI接口时用NewGlobalRef保存起来
static jclass g_obj_class = NULL;

static jclass g_clazz = NULL;

static pthread_mutex_t g_mutex;

static jobjectArray g_pos_array;

/** 传给线程的参数结构体 */
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
void* long_time_task(void * args)
{

    if (0 != pthread_mutex_lock(&g_mutex)) {
        LOGE("锁定互斥锁失败");
    }


    LOGD("into long_time_func");
    JNIEnv *env = NULL;

    // 将当前线程附加到Java虚拟机上，并获得当前线程相关的JNIEnv指针
    if ((*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL) == 0) {
        LOGD("AttachCurrentThread success");
        thread_param_t* param = (thread_param_t*) args;

        LOGD("%s", param->thread_name);
        // 等待，模拟耗时操作
        sleep(DEF_DELAY);
        LOGD("sleep %ds finish", DEF_DELAY);

        int temp_size = 16000;
        /* 这样通过线程的env会找不到class
        jclass g_obj_class = (*env)->FindClass(env, "com/gwcd/sy/clib/LatLng");
        if (g_obj_class == NULL) {
            LOGE("obj_cls null");
        }*/
        jfieldID latID = (*env)->GetFieldID(env, g_obj_class, "latitude", "D");
        jfieldID lngID = (*env)->GetFieldID(env, g_obj_class, "longitude", "D");
        jmethodID initID = (*env)->GetMethodID(env, g_obj_class, "<init>", "()V");

        jobjectArray posArray = (*env)->NewObjectArray(env, temp_size, g_obj_class, NULL);
        int i;
        for (i = 0; i < temp_size; ++i) {
            jobject pos = (*env)->NewObject(env, g_obj_class, initID);
            (*env)->SetDoubleField(env, pos, latID, 30.69198167);
            (*env)->SetDoubleField(env, pos, lngID, 103.95621167);
            (*env)->SetObjectArrayElement(env, posArray, i, pos);

            (*env)->DeleteLocalRef(env, pos);
        }
        g_pos_array = (*env)->NewGlobalRef(env, posArray);
        (*env)->DeleteLocalRef(env, posArray);

        /** 回调上层，通知数据已经准备好了 */
        jmethodID methodID = (*env)->GetStaticMethodID(env, g_clazz, "JniCallback", "(III)V");
        (*env)->CallStaticVoidMethod(env, g_clazz, methodID, UE_GET_LATLNG, 1677217, param->err);

        // TODO:复杂的代码结构下，g_clazz/g_object全局变量的释放问题
        (*env)->DeleteGlobalRef(env, g_obj_class);
        (*env)->DeleteGlobalRef(env, g_clazz);

        // 将线程从Java虚拟机上剥离
        (*g_jvm)->DetachCurrentThread(g_jvm);
        free(param);
        LOGD("finish long_time_func");
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
    g_jvm = vm; // 保存JavaVM全局引用

    return JNI_VERSION_1_4;
}

/*
 * Class:     com_gwcd_sy_clib_LibTest
 * Method:    LongTimeTask
 * Signature: ()[Lcom/gwcd/sy/clib/LatLng;
 */
JNIEXPORT jobjectArray JNICALL Java_com_gwcd_sy_clib_LibTest_LongTimeTask
    (JNIEnv *env, jclass clazz)
{
    // 这个方法模拟一次在主线程上的耗时调用
    int temp_size = 16000;
    LOGD("into LongTimeTask");
    jclass obj_cls = (*env)->FindClass(env, "com/gwcd/sy/clib/LatLng");
    jfieldID latID = (*env)->GetFieldID(env, obj_cls, "latitude", "D");
    jfieldID lngID = (*env)->GetFieldID(env, obj_cls, "longitude", "D");
    jmethodID initID = (*env)->GetMethodID(env, obj_cls, "<init>", "()V");

    jobjectArray posArray = (*env)->NewObjectArray(env, temp_size, obj_cls, NULL);
    int i;
    for (i = 0; i < temp_size; ++i) {
        jobject pos = (*env)->NewObject(env, obj_cls, initID);
        (*env)->SetDoubleField(env, pos, latID, 30.69198167);
        (*env)->SetDoubleField(env, pos, lngID, 103.95621167);
        (*env)->SetObjectArrayElement(env, posArray, i, pos);

        (*env)->DeleteLocalRef(env, pos);
    }
    (*env)->DeleteLocalRef(env, obj_cls);

    sleep(DEF_DELAY);
    LOGD("finish LongTimeTask");
    return posArray;
}

/** 创建线程加载数据 */
void get_data_by_thread(JNIEnv *env, jclass clazz)
{
    // 缓存需要的全局引用
    g_clazz = (jclass)(*env)->NewGlobalRef(env, clazz);
    jclass obj_cls = (*env)->FindClass(env, "com/gwcd/sy/clib/LatLng");
    g_obj_class = (*env)->NewGlobalRef(env, obj_cls);
    /**** 构造线程相关参数 ****/
    thread_param_t* param = (thread_param_t*) malloc(sizeof(thread_param_t));
    memset(param, 0, sizeof(thread_param_t));
    param->thread_name = (unsigned char*)malloc(255);
    memset(param->thread_name, 0, 255);
    sprintf(param->thread_name, "thread id:1");
    param->err = 0;
    /**** 构造线程参数结束，创建子线程 ****/
    pthread_t thread;
    int n = pthread_create(&thread, NULL, (void *)long_time_task, (void *)param);
    if (n != 0) {
        LOGE("线程创建失败");
    } else {
        LOGD("线程创建成功");
    }
}

void get_data_directly(JNIEnv *env, jclass clazz)
{
    int temp_size = 16000;
    LOGD("into get_data_directly");
    jclass obj_cls = (*env)->FindClass(env, "com/gwcd/sy/clib/LatLng");
    jfieldID latID = (*env)->GetFieldID(env, obj_cls, "latitude", "D");
    jfieldID lngID = (*env)->GetFieldID(env, obj_cls, "longitude", "D");
    jmethodID initID = (*env)->GetMethodID(env, obj_cls, "<init>", "()V");

    jobjectArray posArray = (*env)->NewObjectArray(env, temp_size, obj_cls, NULL);
    int i;
    for (i = 0; i < temp_size; ++i) {
        jobject pos = (*env)->NewObject(env, obj_cls, initID);
        (*env)->SetDoubleField(env, pos, latID, 30.69198167);
        (*env)->SetDoubleField(env, pos, lngID, 103.95621167);
        (*env)->SetObjectArrayElement(env, posArray, i, pos);

        (*env)->DeleteLocalRef(env, pos);
    }
    (*env)->DeleteLocalRef(env, obj_cls);
    g_pos_array = (*env)->NewGlobalRef(env, posArray);
    (*env)->DeleteLocalRef(env, posArray);

    sleep(DEF_DELAY);

    /** 回调上层，通知数据已经准备好了 */
    jmethodID methodID = (*env)->GetStaticMethodID(env, clazz, "JniCallback", "(III)V");
    (*env)->CallStaticVoidMethod(env, clazz, methodID, UE_GET_LATLNG, 1677217, 0);
    LOGD("finish get_data_directly");
}

/*
 * Class:     com_gwcd_sy_clib_LibTest
 * Method:    LongTimeTask2
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_gwcd_sy_clib_LibTest_LongTimeTask2
    (JNIEnv *env, jclass clazz)
{
    LOGD("into LongTimeTask2.");
    //get_data_by_thread(env, clazz);
    get_data_directly(env, clazz);
    LOGD("LongTimeTask2 finish.");


    /**
    env->GetJavaVM(&g_jvm); //保存到全局变量中JVM
    //如果是传入jobject，应该调用以下函数，创建可以在其他线程中使用的引用:
    g_object=env->NewGlobalRef(obj);
    */
    // 创建全局引用
    /*

    LOGD("into LongTimeTask2");
    int i;
    pthread_t threads[10];
    for (i = 0; i < 10; ++i) {
        thread_param_t* param = (thread_param_t*) malloc(sizeof(thread_param_t));
        memset(param, 0, sizeof(thread_param_t));
        param->thread_name = (unsigned char*)malloc(255);
        sprintf(param->thread_name, "thread id:%d", (i + 1));
        param->err = (i + 1);
        pthread_t thread;
        pthread_attr_t p_attr;
        pthread_attr_init(&p_attr);
        int n = pthread_create(&threads[i], &p_attr, (void *)long_time_task, (void *)param);
        if ( geteuid() == 0 ) {
            // TODO:设置调度策略,用户虚拟机测试，虚拟机是root权限
            // 线程的调度有三种策略：SCHED_OTHER、SCHED_RR和SCHED_FIFO。默认是SCHED_OTHER，这种策略是抢占式的
            // SCHED_RR和SCHED_FIFO只能在超级用户下运行?
            struct sched_param *sch_param = (struct sched_param *) malloc(sizeof(struct sched_param));
            memset(sch_param, 0, sizeof(struct sched_param));
            sch_param->sched_priority = 50;
            pthread_setschedparam(threads[i], SCHED_FIFO, sch_param);
        } else {
            LOGE("不是超级用户");
        }
        pthread_attr_destroy(&p_attr);
        if (n != 0) {
            LOGE("线程创建失败");
        } else {
            LOGD("线程创建成功");
        }
    }
    */
    // pthread_join用来等待一个线程的结束,线程间同步的操作。如果有需要。但是join的后果还是会造成主线程阻塞。
    // pthread_join(pthread_t thread, void **retval);
    /*
    for (i = 0; i < 10; ++i) {
        pthread_join(threads[i], NULL);
    }*/
}

/*
 * Class:     com_gwcd_sy_clib_LibTest
 * Method:    getLatLngData
 * Signature: ()[Lcom/gwcd/sy/clib/LatLng;
 */
JNIEXPORT jobjectArray JNICALL Java_com_gwcd_sy_clib_LibTest_getLatLngData
  (JNIEnv *env, jclass clazz)
{
    return g_pos_array;
}

/*
 * Class:     com_gwcd_sy_clib_LibTest
 * Method:    simulateEvent
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_gwcd_sy_clib_LibTest_simulateEvent
  (JNIEnv *env, jclass clazz)
{
    // 模拟一个通知上层刷新数据的事件，由app按键触发
    jmethodID methodID = (*env)->GetStaticMethodID(env, clazz, "JniCallback", "(III)V");
    (*env)->CallStaticVoidMethod(env, clazz, methodID, UE_INFO_MODIFY, 1677217, 0);
}
