package com.dongnao.faceswap;

import android.app.Activity;
import android.app.ProgressDialog;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends Activity {


    private ProgressDialog pd;
    private SwapSurfaceView swapSurfaceView;

    private void showLoading() {
        if (null == pd) {
            pd = new ProgressDialog(this);
            pd.setIndeterminate(true);
        }
        pd.show();
    }

    private void dismissLoading() {
        if (null != pd) {
            pd.dismiss();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        swapSurfaceView = (SwapSurfaceView) findViewById(R.id.swapSurfaceView);
        new AsyncTask<Void, Void, Void>() {

            @Override
            protected Void doInBackground(Void... params) {
                try {
                    File dir = new File("/sdcard/faceswap");
                    copyAssetsFile("haarcascade_frontalface_alt.xml", dir);
                    copyAssetsFile("shape_predictor_68_face_landmarks.dat", dir);
                    SwapHelper.createSwap("/sdcard/faceswap/haarcascade_frontalface_alt.xml"
                            , "/sdcard/faceswap/shape_predictor_68_face_landmarks.dat");
                } catch (IOException e) {
                    e.printStackTrace();
                }
                return null;
            }

            @Override
            protected void onPreExecute() {
                showLoading();
            }

            @Override
            protected void onPostExecute(Void aVoid) {
                dismissLoading();
            }
        }.execute();
    }


    private void copyAssetsFile(String name, File dir) throws IOException {
        if (!dir.exists()) {
            dir.mkdirs();
        }
        File file = new File(dir, name);
        if (!file.exists()){
            InputStream is = getAssets().open(name);
            FileOutputStream fos = new FileOutputStream(file);
            int len;
            byte[] buffer = new byte[2048];
            while ((len = is.read(buffer)) != -1)
                fos.write(buffer, 0, len);
            fos.close();
            is.close();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        SwapHelper.destorySwap();
    }

    public void switchCamera(View view) {
        swapSurfaceView.switchCamera();
    }

    public void startSwap(View view) {
        swapSurfaceView.startSwaping();
    }

    public void stopSwap(View view) {
        swapSurfaceView.stopSwaping();
    }
}
