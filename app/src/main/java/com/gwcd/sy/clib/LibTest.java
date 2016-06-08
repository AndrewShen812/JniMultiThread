/**
 * 项目名称：JniMultiThread
 * 创建日期：2016年06月06日
 * Copyright 2016 GALAXYWIND Network Systems Co.,Ltd.All rights reserved.
 */
package com.gwcd.sy.clib;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

/**
 * 类描述：<br>
 * 创建者：shenyong<br>
 * 创建时间：2016/6/6<br>
 * 修改记录：<br>
 */
public class LibTest {

    public static final int UE_BEGIN = 0;

    public static final int UE_INFO_MODIFY = UE_BEGIN + 4;
    public static final int UE_GET_LATLNG = UE_BEGIN + 5;

    static {
        System.loadLibrary("test");
    }

    public static Handler mHandler;

    private static void JniCallback(int event, int handle, int err_no) {
        Log.d("JniCallback", "JniCallback event:" + event);
        if (UE_INFO_MODIFY == event) {
            DataThread dataTh = new DataThread();
            dataTh.start();
//            LongTimeTask2();
            return;
        }
        passEvent(event, handle, err_no);
    }

    private static void passEvent(int event, int handle, int err_no) {
        if (mHandler != null) {
            Message msg = mHandler.obtainMessage();
            msg.what = event;
            msg.arg1 = handle;
            msg.arg2 = err_no;
            mHandler.sendMessage(msg);
        }
    }

    public static native void nativeInit();

    public static native LatLng[] LongTimeTask();

    public static native void LongTimeTask2();

    public static native LatLng[] getLatLngData();

    public static native void simulateEvent();

    public static native void nativeRelease();

    private static class DataThread extends Thread {
        @Override
        public void run() {
            Log.d("SY", "DataThread run.");
            LongTimeTask2();
        }
    }
}
