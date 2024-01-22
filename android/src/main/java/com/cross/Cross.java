package com.cross;

public class Cross {
    static {
        System.loadLibrary("cross");
    }
    public static native void beginListen(int port);
}
