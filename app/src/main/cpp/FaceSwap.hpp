//
//  FaceSwap.hpp
//  OpenCVLearn
//
//  Created by liuxiang on 2017/7/7.
//  Copyright © 2017年 liuxiang. All rights reserved.
//

#ifndef FaceDetector_hpp
#define FaceDetector_hpp

#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include "dlib/opencv.h"
#include "dlib/image_processing.h"
#include "common.hpp"

using namespace cv;
using namespace std;





//发现检测速度上存在问题 使用DetectionBasedTracker
//在后台线程中执行慢速检测，因此大大减少了处理时间。
class CascadeDetectorAdapter: public DetectionBasedTracker::IDetector{
public:
    CascadeDetectorAdapter(cv::Ptr<CascadeClassifier> detector):
            IDetector(),
            detector(detector){
        CV_Assert(detector);
    }

    void detect(const Mat &Image, vector<Rect> &objects){
        detector->detectMultiScale(Image, objects, scaleFactor, minNeighbours, 0, minObjSize, maxObjSize);
    }

    ~CascadeDetectorAdapter() {
        
    }

private:
    CascadeDetectorAdapter();
    Ptr<CascadeClassifier> detector;
};


//struct Face{
//    //凸点
//    vector<Point2i> hulls;
//    vector<Point2i> swapHulls;
//    //脸部区域 相对于缩放后
//    Rect rect;
//    //面部三角部分
//    vector<Point2i> triangles;
//    vector<Point2i> swaptriangles;
//};

class FaceSwap{
public:
    FaceSwap(const char* detectModel,const char* landmarkModel);
    ~FaceSwap();
    void detectorFace(Mat& mat,vector<Rect>& faces);
    void swapFaces(Mat& mat,Rect &rect_face1, Rect &rect_face2);
    void startTracking();
    void stopTracking();
public:
    bool isInit = false;
private:
    
    void delaunayTriangulation(vector<Point2i> hull,Point2f* triangles,Rect rect);
    void faceLandmarkDetection(Mat frame, Rect rectangle, vector<Point2i>& landmark);
    dlib::full_object_detection faceLandmarkDetection(Mat mat, Rect rectangle);
private:
    //xcode 开发时使用的摄像头 android不需要
//    Ptr<VideoCapture> camera;
    Ptr<DetectionBasedTracker> tracker;
    //dlib 人脸特征提取
    dlib::shape_predictor shapePredictor;

};

#endif /* FaceDetector_hpp */
