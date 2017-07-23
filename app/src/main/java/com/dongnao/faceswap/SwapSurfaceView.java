package com.dongnao.faceswap;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by xiang on 2017/7/9.
 * 动脑学院 版权所有
 */

public class SwapSurfaceView extends SurfaceView implements SurfaceHolder.Callback, Camera
        .PreviewCallback {

    private int mCameraId = Camera.CameraInfo.CAMERA_FACING_BACK;
    private Camera mCamera;
    private boolean mPreviewRunning;
    private int mScreen;
    private int  width;
    private int  height;
     private int  realWidth;
    private int  realHeight;
    private byte[] mBuffer;
    private byte[] mData;
    

    private boolean mFrameReady;
    private byte[][] mFrameChain;
    private int mChainIdx;

    private boolean mStopThread;
    private Thread mThread;
    private Bitmap mCacheBitmap;

    private boolean mStartSwap;
    private SurfaceTexture mSurfaceTexture;



    public SwapSurfaceView(Context context) {
        super(context);
        getHolder().addCallback(this);
    }

    public SwapSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        getHolder().addCallback(this);
    }

    public SwapSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        getHolder().addCallback(this);
    }

    public void switchCamera() {
        if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
            mCameraId = Camera.CameraInfo.CAMERA_FACING_FRONT;
        } else {
            mCameraId = Camera.CameraInfo.CAMERA_FACING_BACK;
        }
        stopPreview();
        startPreview();
    }

    public void startSwaping() {
        if (!mStartSwap) {
            SwapHelper.startTracking();
            mStartSwap = true;
            stopPreview();
            startPreview();
        }
    }


    public void stopSwaping() {
        if (mStartSwap) {
            SwapHelper.stopTracking();
            mStartSwap = false;
            stopPreview();
            startPreview();
        }
    }




    private void stopPreview() {
        try {
            mStopThread = true;
            synchronized (this) {
                this.notify();
            }
            if (mThread != null)
                mThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            mThread = null;
        }
        if (null != mCacheBitmap) {
           mCacheBitmap.recycle();
           mCacheBitmap = null;   
        }
        //线程消亡再置为false
        mFrameReady = false;
        if (mCamera != null) {
            mCamera.setPreviewCallback(null);
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
        }
        mPreviewRunning = false;
    }

    @SuppressWarnings("deprecation")
    private void startPreview() {
        if (mPreviewRunning) {
            return;
        }
        mStopThread = false;
        mThread = new Thread(new CameraWorker());
        mThread.start();
        try {
            mCamera = Camera.open(mCameraId);
            Camera.Parameters parameters = mCamera.getParameters();
            parameters.setPreviewFormat(ImageFormat.NV21);
            mCamera.setParameters(parameters);
            Camera.Size previewSize = parameters.getPreviewSize();

            width = previewSize.width;
            height = previewSize.height;

            // width = 1124;
            // height = 718;
            setPreviewOrientation();
             System.out.println("width:"+width+"  height:"+height);
            //获得nv21的像素数
            //y : w*h
            //u : w*h/4
            //v : w*h/4
            int bitsPerPixel = ImageFormat.getBitsPerPixel(ImageFormat.NV21);
            mBuffer = new byte[realWidth * realHeight * bitsPerPixel / 8];
            mData = new byte[mBuffer.length];
            mFrameChain = new byte[2][mBuffer.length];
            mCacheBitmap = Bitmap.createBitmap(realWidth , realHeight, Bitmap
                    .Config.ARGB_8888);
            mCamera.addCallbackBuffer(mBuffer);
            mCamera.setPreviewCallbackWithBuffer(this);
            mSurfaceTexture = new SurfaceTexture(10);
            mCamera.setPreviewTexture(mSurfaceTexture);
            mCamera.startPreview();
            mPreviewRunning = true;
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    private void setPreviewOrientation() {
        int rotation = ((Activity)getContext()).getWindowManager().getDefaultDisplay().getRotation();
        mScreen = rotation;
        if (mScreen == Surface.ROTATION_0) { 
            //如果竖屏需要向右旋转90度
            realWidth = height;
            realHeight = width;
        }else{
            realWidth = width;
            realHeight = height;
        }
    }

    


    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        stopPreview();
        startPreview();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopPreview();
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mPreviewRunning) {
            //同步
            synchronized (this) {
                if (mScreen == Surface.ROTATION_0){
                    portraitData(data);
                }else if(mScreen == Surface.ROTATION_270){
                   landscapeData(data);
                }else{
                    mData = data;
                }
                mFrameChain[mChainIdx] = mData;
                mFrameReady = true;
                this.notify();
            }
        }
        if (mCamera != null)
            mCamera.addCallbackBuffer(mBuffer);
    }

    private void landscapeData(byte[] data) {
        int y_len = width * height;
        int k = 0;
        // y数据倒叙插入raw中
        for (int i = y_len - 1; i > -1; i--) {
            mData[k] = data[i];
            k++;
        }
        // System.arraycopy(data, y_len, mData, y_len, uv_len);
        // v1 u1 v2 u2
        // v3 u3 v4 u4
        // 需要转换为:
        // v4 u4 v3 u3
        // v2 u2 v1 u1
        int maxpos = data.length - 1;
        int uv_len = y_len >> 2; // 4:1:1
        for (int i = 0; i < uv_len; i++) {
            int pos = i << 1;
            mData[y_len + i * 2] = data[maxpos - pos - 1];
            mData[y_len + i * 2 + 1] = data[maxpos - pos];
        }
    }

    private void portraitData(byte[] data) {
        int y_len = width * height;
        int uvHeight = height >> 1; // uv数据高为y数据高的一半
        int k = 0;
        if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
            for (int j = 0; j < width; j++) {
                for (int i = height - 1; i >= 0; i--) {
                    //最后一列数据加入进来
                    //第二次j的循环添加倒数第二列
                    mData[k++] = data[width * i + j];
                }
            }
            for (int j = 0; j < width; j += 2) {
                for (int i = uvHeight - 1; i >= 0; i--) {
                    mData[k++] = data[y_len + width * i + j];
                    mData[k++] = data[y_len + width * i + j + 1];
                }
            }
        } else {
            for (int i = 0; i < width; i++) {
                int nPos = width - 1;
                for (int j = 0; j < height; j++) {
                    mData[k] = data[nPos - i];
                    k++;
                    nPos += width;
                }
            }
            for (int i = 0; i < width; i += 2) {
                int nPos = y_len + width - 1;
                for (int j = 0; j < uvHeight; j++) {
                    mData[k] = data[nPos - i - 1];
                    mData[k + 1] = data[nPos - i];
                    k += 2;
                    nPos += width;
                }
            }
        }
    }


    private void drawCanvas() {
        Canvas canvas = getHolder().lockCanvas();
        if (canvas != null && null != mCacheBitmap ) {
            // new Rect((canvas.getWidth() - mCacheBitmap.getWidth()) / 2,
            //                 (canvas.getHeight() - mCacheBitmap.getHeight()) / 2,
            //                 (canvas.getWidth() - mCacheBitmap.getWidth()) / 2 + mCacheBitmap
            //                         .getWidth(),
            //                 (canvas.getHeight() - mCacheBitmap.getHeight()) / 2 + mCacheBitmap
            //                         .getHeight())
            canvas.drawColor(0, android.graphics.PorterDuff.Mode.CLEAR);
            canvas.drawBitmap(mCacheBitmap, new Rect(0, 0, mCacheBitmap.getWidth(), mCacheBitmap
                            .getHeight()),
                    new Rect(0,0,canvas.getWidth(),canvas.getHeight()), null);
            getHolder().unlockCanvasAndPost(canvas);
        }
    }

    private class CameraWorker implements Runnable {

        @Override
        public void run() {
            do {
                boolean hasFrame = false;
                synchronized (SwapSurfaceView.this) {
                    try {
                        while (!mFrameReady && !mStopThread) {
                            SwapSurfaceView.this.wait();
                        }
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    if (mFrameReady) {
                        // 0-1之间切换
                        mChainIdx = 1 - mChainIdx;
                        mFrameReady = false;
                        hasFrame = true;
                    }
                }
                //防止处理数据占用太长时间 放到外面
                if (!mStopThread && hasFrame) {
                    //切换回来 拿数据
                    if (mFrameChain[1 - mChainIdx] != null) {
                        SwapHelper.processFrame(mFrameChain[1 - mChainIdx], realWidth,
                                realHeight, mCacheBitmap, mStartSwap);
                        drawCanvas();
                    }
                    mFrameChain[1 - mChainIdx] = null;
                }
            } while (!mStopThread);

        }
    }
}
