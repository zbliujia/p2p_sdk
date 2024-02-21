package com.cross;

public class Cross {
    static {
        System.loadLibrary("cross");
    }
    public static native String startServer(String user, String device);
    public static native void endServer();
}
