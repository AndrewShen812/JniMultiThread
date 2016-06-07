package com.gwcd.sy.jnimultithread;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.gwcd.sy.clib.LatLng;
import com.gwcd.sy.clib.LibTest;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "SY";

    private TextView mTvPrint;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            String strMsg = "event:" + msg.what + ", handle:" + msg.arg1 + ", err_no:" + msg.arg2;
            Log.d(TAG, strMsg);
            mTvPrint.setText(strMsg);
            switch (msg.what) {
                case LibTest.UE_GET_LATLNG:
                    LatLng[] pos = LibTest.getLatLngData();
                    if (pos != null && pos.length > 0) {
                        Log.d(TAG, "getLatLngData! pos.length:" + pos.length);
                    }
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        LibTest.mHandler = mHandler;
        mTvPrint = (TextView) findViewById(R.id.tv_main_print);
        LibTest.nativeInit();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        LibTest.nativeRelease();
    }

    public void onCallJni(View view) {
        LatLng[] pos = LibTest.LongTimeTask();
        if (pos != null && pos.length > 0) {
            Log.d(TAG, "onCallJni: pos.length" + pos.length);
        }
    }

    public void onCallJniMT(View view) {
        LibTest.simulateEvent();
    }
}
