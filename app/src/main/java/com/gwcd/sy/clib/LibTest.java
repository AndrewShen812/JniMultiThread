/**
 * 项目名称：JniMultiThread
 * 创建日期：2016年06月06日
 * Copyright 2016 GALAXYWIND Network Systems Co.,Ltd.All rights reserved.
 */
package com.gwcd.sy.clib;

import android.os.Handler;
import android.os.Message;

/**
 * 类描述：<br>
 * 创建者：shenyong<br>
 * 创建时间：2016/6/6<br>
 * 修改记录：<br>
 */
public class LibTest {

    static {
        System.loadLibrary("test");
    }

    public static Handler mHandler;

    private static void JniCallback(int event, int handle, int err_no) {
        if (mHandler != null) {
            Message msg = mHandler.obtainMessage();
            msg.what = event;
            msg.arg1 = handle;
            msg.arg2 = err_no;
            mHandler.sendMessage(msg);
        }
    }

    public static native void nativeInit();

    public static native void LongTimeTask();

    public static native void LongTimeTask2();

    public static native void nativeRelease();
}
