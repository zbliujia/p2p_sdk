package com.cross.example;


import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.cross.Cross;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        TextView textView = findViewById(R.id.textView);
//        String url = "http://example.com?key2=value2&key3=value3&key1=VALUE1";
//        String result = Cross.signatureUrl(url);
//        textView.setText(url + "\n" + result);
        Thread thread = new Thread() {
            @Override
            public void run() {
                try {
                    Log. d("cross", "beginListen");
                    Cross.beginListen(9900);
                    Log. d("cross", "beginListen end");
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        };
        thread.start();
    }
}
