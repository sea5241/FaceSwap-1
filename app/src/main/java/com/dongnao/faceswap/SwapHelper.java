package com.dongnao.faceswap;

import android.graphics.Bitmap;

/**
 * Created by xiang on 2017/7/9.
 * 动脑学院 版权所有
 */

public class SwapHelper {

    static {
        System.loadLibrary("opencv_java3");
        System.loadLibrary("native-lib");
    }

    //这里不做成类了 static方便调用了
 	public static native void createSwap(String detectModel,String landmarkModel);
  	public static native void startTracking();
  	public static native void stopTracking();
    public static native void destorySwap();
   
    public static native void processFrame(byte[] data, int w, int h, Bitmap cacheBitmap,boolean isStart);

}
