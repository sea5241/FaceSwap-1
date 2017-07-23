#include <jni.h>
#include <string>
#include <android/bitmap.h>

#include "FaceSwap.hpp"

//android编译dlib 使用gnustl_static round没有定义在std命名空间中 在image_transforms/random_cropper.h中定义
//把config.h 的 DLIB_NO_GUI_SUPPORT 打开




extern "C"{

void mat2Bitmap(JNIEnv *env, Mat mat, jobject bitmap) {
    AndroidBitmapInfo info;
    void *pixels = 0;
    CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
    CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
              info.format == ANDROID_BITMAP_FORMAT_RGB_565);
    CV_Assert(mat.dims == 2 && info.height == (uint32_t) mat.rows &&
              info.width == (uint32_t) mat.cols);
    CV_Assert(mat.type() == CV_8UC1 || mat.type() == CV_8UC3 || mat.type() == CV_8UC4);
    CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
    CV_Assert(pixels);
    if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
        Mat tmp(info.height, info.width, CV_8UC4, pixels);
        if (mat.type() == CV_8UC1) {
            cvtColor(mat, tmp, COLOR_GRAY2RGBA);
        } else if (mat.type() == CV_8UC3) {
            cvtColor(mat, tmp, COLOR_BGR2RGBA);
        } else if (mat.type() == CV_8UC4) {
            mat.copyTo(tmp);
        }
    } else {
        Mat tmp(info.height, info.width, CV_8UC2, pixels);
        if (mat.type() == CV_8UC1) {
            cvtColor(mat, tmp, COLOR_GRAY2BGR565);
        } else if (mat.type() == CV_8UC3) {
            cvtColor(mat, tmp, COLOR_RGB2BGR565);
        } else if (mat.type() == CV_8UC4) {
            cvtColor(mat, tmp, COLOR_RGBA2BGR565);
        }
    }
    AndroidBitmap_unlockPixels(env, bitmap);
}

FaceSwap* faceSwap = 0;


JNIEXPORT void JNICALL
Java_com_dongnao_faceswap_SwapHelper_createSwap(JNIEnv *env, jclass type, jstring detectModel_,
                                              jstring landmarkModel_) {
    const char *detectModel = env->GetStringUTFChars(detectModel_, 0);
    const char *landmarkModel = env->GetStringUTFChars(landmarkModel_, 0);
    faceSwap = new FaceSwap(detectModel,landmarkModel);
    env->ReleaseStringUTFChars(detectModel_, detectModel);
    env->ReleaseStringUTFChars(landmarkModel_, landmarkModel);
}


JNIEXPORT void JNICALL
Java_com_dongnao_faceswap_SwapHelper_startTracking(JNIEnv *env, jclass type){
    if (faceSwap && faceSwap->isInit){
        faceSwap->startTracking();
    }
}

JNIEXPORT void JNICALL
Java_com_dongnao_faceswap_SwapHelper_stopTracking(JNIEnv *env, jclass type){
    if (faceSwap && faceSwap->isInit){
        faceSwap->stopTracking();
    }
}


JNIEXPORT void JNICALL
Java_com_dongnao_faceswap_SwapHelper_processFrame(JNIEnv *env, jclass type, jbyteArray data_,
                                                  jint w, jint h,jobject cacheBitmap,jboolean isStart) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    Mat nv21Mat(h + h /2,w,CV_8UC1,data);
    Mat rgbMat;
    cvtColor(nv21Mat, rgbMat, CV_YUV2BGR_NV21);
    // if (faceSwap)
    // {
    //      LOGI("nv21 转 rgb %d %d",isStart,faceSwap->isInit);
    // } else{
    //      LOGI("nv21 转 rgb %d and faceswap is null",isStart);
    // }
    // rgbMat = imread("/sdcard/faceswap/test.png");
    // resize(rgbMat,rgbMat,Size(640,480));
    if (isStart && faceSwap && faceSwap->isInit){
        // Mat gratMat;
        // cvtColor(rgbMat, gratMat, CV_YUV2RGB_NV21);
        vector<Rect> faces;
        faceSwap->detectorFace(rgbMat,faces);
        LOGI("找到%d张脸",faces.size());
        if(faces.size() >= 2){
            faceSwap->swapFaces(rgbMat,faces[0],faces[1]);
        }
        for (int i = 0; i < faces.size(); ++i){
            Rect face = faces[i];
            rectangle(rgbMat, face.tl(),face.br(), Scalar(0,255,255));
        }
    } 
    // else {
    //     LOGI("没有初始化或者未开启");
    // }

    mat2Bitmap(env,rgbMat,cacheBitmap);
    // imwrite("/sdcard/faceswap/a.jpg",nv21Mat);
    //  imwrite("/sdcard/faceswap/b.jpg",rgbMat);
    nv21Mat.release();
    rgbMat.release();

   
    env->ReleaseByteArrayElements(data_, data, 0);
}


JNIEXPORT void JNICALL
Java_com_dongnao_faceswap_SwapHelper_destorySwap(JNIEnv *env, jclass type) {
    if(faceSwap){
        delete faceSwap;
        faceSwap = 0;
    }
}

}

