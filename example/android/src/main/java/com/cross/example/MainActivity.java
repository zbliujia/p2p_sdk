package com.cross.example;


import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.TextView;

import com.cross.Cross;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        TextView textView = findViewById(R.id.textView);
        final WebView webView = findViewById(R.id.webView);
        webView.setWebViewClient(new WebViewClient(){
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                //使用WebView加载显示url
                view.loadUrl(url);
                //返回true
                return true;
            }
        });
        Button button = findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
//                webView.loadUrl("https://www.baidu.com");
                webView.loadUrl("http://127.0.0.1:8081/index.html");
//                webView.loadUrl("http://116.162.85.228:8080/GetDeviceInfo.php?token=token");
            }
        });
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
