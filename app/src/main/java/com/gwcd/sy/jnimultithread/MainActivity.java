package com.gwcd.sy.jnimultithread;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

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
        LibTest.LongTimeTask();
    }

    public void onCallJniMT(View view) {
        LibTest.LongTimeTask2();
    }
}
