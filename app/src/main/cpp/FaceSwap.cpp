//
//  FaceSwap.cpp
//  OpenCVLearn
//
//  Created by liuxiang on 2017/7/7.
//  Copyright © 2017年 liuxiang. All rights reserved.
//


//http://blog.csdn.net/chenhuaijin/article/details/9292811
//Color transfer between images


#include "FaceSwap.hpp"


FaceSwap::FaceSwap(const char* detectModel,const char* landmarkModel){
//    camera = makePtr<VideoCapture>(0);
    //用于对象检测的级联分类器类。
   try {
        LOGI("加载特征模型: %s 完成",landmarkModel);
        Ptr<CascadeDetectorAdapter> mainDetector = makePtr<CascadeDetectorAdapter>(
            makePtr<CascadeClassifier>(detectModel));
        Ptr<CascadeDetectorAdapter> trackingDetector = makePtr<CascadeDetectorAdapter>(
            makePtr<CascadeClassifier>(detectModel));
        DetectionBasedTracker::Parameters detectorParams;
        tracker = makePtr<DetectionBasedTracker>(mainDetector, trackingDetector, detectorParams);
        LOGI("开始加载特征模型: %s ",landmarkModel);
        dlib::deserialize(landmarkModel) >> shapePredictor;
        isInit = true;
    } catch(cv::Exception& e){
        LOGI("加载模型: %s 失败",detectModel);
    }

   
    // cascadeClassifier = makePtr<CascadeClassifier>(detectModel);
    // if (cascadeClassifier->empty()){
    //     LOGI("加载脸部检测模型: %s 失败",detectModel);
    //     return;
    // }
    // LOGI("加载脸部检测模型: %s 完成",detectModel);   
}

FaceSwap::~FaceSwap(){
    if (tracker.get()){
        tracker->stop();
    }
}

void FaceSwap::startTracking(){
    tracker->run();
}

void FaceSwap::stopTracking(){
    tracker->stop();
}


void FaceSwap::detectorFace(Mat& mat,vector<Rect>& faces){
//    *camera >> frame;
    Mat grayMat;
    cvtColor(mat, grayMat, CV_BGR2GRAY);
    //直方图均衡化  增强对比效果
    equalizeHist(grayMat, grayMat);
    // cascadeClassifier->detectMultiScale(grayMat, faces, 1.1, 2, 0,
    //                                     Size(10, 10),
    //                                     Size(grayMat.cols*.9f , grayMat.rows*.9f));
    // grayMat.release();


    tracker->process(grayMat);
    tracker->getObjects(faces);
    grayMat.release();
}


void FaceSwap::faceLandmarkDetection(Mat frame, Rect rect, vector<Point2i>& landmark){
    dlib::rectangle rectangle = dlib::rectangle(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
    dlib::cv_image<dlib::bgr_pixel> faceFrame(frame);
    dlib::full_object_detection shape = shapePredictor(faceFrame, rectangle);
    for (size_t i = 0; i < shape.num_parts(); ++i){
        landmark.push_back(Point2i(shape.part(i).x(),shape.part(i).y()));
    }
}

dlib::full_object_detection FaceSwap::faceLandmarkDetection(Mat mat, Rect rect){
    dlib::rectangle rectangle = dlib::rectangle(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
    dlib::cv_image<dlib::bgr_pixel> faceFrame(mat);
    dlib::full_object_detection shape = shapePredictor(faceFrame, rectangle);
    return shape;
}




//由于人脸朝向 大小等不同 所以需要进行仿射变换
//构成一个平面至少需要3个点 根据凸点获得三角部分 来进行仿射变换
void FaceSwap::delaunayTriangulation(vector<Point2i> hull,Point2f* triangles,Rect rect){
    Subdiv2D subdiv(rect);
    //将点插入到subdiv中
    for (int i = 0; i < hull.size(); i++){
        Point2f point2f = hull[i];
        if(point2f.x>0&& point2f.y>0&&point2f.x < rect.width && point2f.y < rect.height){
            subdiv.insert(hull[i]);
        }
    }
    //获取Delaunay三角形的列表
    vector<Vec6f> triangleList;
    subdiv.getTriangleList(triangleList);
    //    return triangleList;
    
    int maxArea = 0;
    int index = 0;
    for (int i = 0; i < triangleList.size(); ++i){
        Vec6f t = triangleList[i];
        //三角形的三个顶点
        //只需要在脸部的三角形
        vector<Point2f> triangle;
        Point2f p1(t[0], t[1]);
        Point2f p2(t[2], t[3]);
        Point2f p3(t[4], t[5]);
        if (rect.contains(p1) && rect.contains(p2) && rect.contains(p3)){
            triangle.push_back(p1);
            triangle.push_back(p2);
            triangle.push_back(p3);
            Rect rect = boundingRect(triangle);
            int area = rect.area();
            if (maxArea < area) {
                maxArea = area;
                index = i;
            }
        }
        triangle.clear();
    }
    Vec6f t = triangleList[index];
    Point2f p1(t[0], t[1]);
    Point2f p2(t[2], t[3]);
    Point2f p3(t[4], t[5]);
    triangles[0] = p1;
    triangles[1] = p2;
    triangles[2] = p3;
    
}


void FaceSwap::swapFaces(Mat& mat,Rect &rect_face1, Rect &rect_face2){
    //为了减少计算矩形大小...
    //取并集
    Rect bounding_rect = rect_face1 | rect_face2;
    //中心点不变扩大
    bounding_rect -= Point(50, 50);
    bounding_rect += Size(100, 100);
    //取交集 防止超出图片大小
    bounding_rect &= cv::Rect(0, 0, mat.cols, mat.rows);
    //相对于新矩形的位置
    Rect new_rect_face1 = rect_face1 - bounding_rect.tl();
    Rect new_rect_face2 = rect_face2 - bounding_rect.tl();
    
    Mat frame =  mat(bounding_rect);
    Size frame_size = Size(frame.cols, frame.rows);
    
   
#if 0
    //提取面部特征
    vector<Point2i> landMarks1;
    vector<Point2i> landMarks2;
    faceLandmarkDetection(frame,new_rect_face1,landMarks1);
    faceLandmarkDetection(frame,new_rect_face2,landMarks2);
    //在面部特征点中获得凸包下标集合
    vector<int> hullIndex1;
    convexHull(landMarks1, hullIndex1, false, false);
    vector<Point2i> hulls1,hulls2;
    for(size_t i = 0; i < hullIndex1.size(); ++i){
        int index = hullIndex1[i];
        hulls1.push_back(landMarks1[index]);
        hulls2.push_back(landMarks2[index]);
    }
    //face2 找对应的点
    vector<Point2i> hulls1,hulls2;
    for(size_t i = 0; i < hullIndex1.size(); ++i){
        int index = hullIndex1[i];
        hulls1.push_back(landMarks1[index]);
        hulls2.push_back(landMarks2[index]);
    }
    
    //获得三角部分的点 保存最大的三角形
    // Point2f triangles1[3], triangles2[3];
    // Rect rect(0, 0,  frame.cols, frame.rows);
    // delaunayTriangulation(hulls1,triangles1,rect);
    // int index = 0;
    // for(size_t i = 0 ; i < hulls1.size(); ++i){
    //     Point2f hull = hulls1[i];
    //     if(triangles1[0] == hull || triangles1[1] == hull || triangles1[2] == hull){
    //         triangles2[index] = hulls2[i];
    //         index++;
    //     }
    // }
#else
    //android上性能与效果测试
    //获得特征点
    
    dlib::full_object_detection landMarks1 = faceLandmarkDetection(frame,new_rect_face1);
    dlib::full_object_detection landMarks2 = faceLandmarkDetection(frame,new_rect_face2);
    //获得对应点
    auto getPoint1 = [&](int index) -> Point2i {
        auto p = landMarks1.part(index);
        return Point2i(p.x(),p.y());
    };
    auto getPoint2 = [&](int index) -> Point2i {
        auto p = landMarks2.part(index);
        return Point2i(p.x(),p.y());
    };
    
    //根据资料 人脸68个特征点是固定的 所以不求三角形 直接拿点
    Point2f triangles1[3], triangles2[3];
    triangles1[0] =  getPoint1(8) ; //下巴
    triangles1[1] =  getPoint1(17) ;//左眼左侧
    triangles1[2] =  getPoint1(26);//右眼右侧
    
    triangles2[0] =  getPoint2(8) ;
    triangles2[1] =  getPoint2(17) ;
    triangles2[2] =  getPoint2(26) ;
    
    //手动获取凸包
    vector<Point2i> hulls1,hulls2;
    //逆时针
    hulls1.push_back(getPoint1(0));
    hulls1.push_back(getPoint1(3));
    hulls1.push_back(getPoint1(5));
    hulls1.push_back(getPoint1(7));
    hulls1.push_back(getPoint1(9));
    hulls1.push_back(getPoint1(11));
    hulls1.push_back(getPoint1(13));
    hulls1.push_back(getPoint1(16));
    //鼻子高度
    Point2i nose_length = getPoint1(27) - getPoint1(30);
    //眼睛左右两侧
    hulls1.push_back(getPoint1(26) + nose_length);
    hulls1.push_back(getPoint1(24) + nose_length);
    
    hulls2.push_back(getPoint2(0));
    hulls2.push_back(getPoint2(3));
    hulls2.push_back(getPoint2(5));
    hulls2.push_back(getPoint2(7));
    hulls2.push_back(getPoint2(9));
    hulls2.push_back(getPoint2(11));
    hulls2.push_back(getPoint2(13));
    hulls2.push_back(getPoint2(16));
    //鼻子高度
     nose_length = getPoint2(27) - getPoint2(30);
    //眼睛左右两侧
    hulls2.push_back(getPoint2(26) + nose_length);
    hulls2.push_back(getPoint2(17) + nose_length);

    
    
    
    
    
#endif
    
    
    
    //===================仿射变换获得两个脸图==================================
    //仿射变换
    Mat trans1 = getAffineTransform(triangles1, triangles2);
    Mat trans2;
    invertAffineTransform(trans1, trans2);
   
    
    //画出凸包连接的多边形
    Mat mask1 = Mat::zeros(frame_size.height, frame_size.width, CV_8UC1);
    Mat mask2 = Mat::zeros(frame_size.height, frame_size.width, CV_8UC1);
    
    fillConvexPoly(mask1, hulls1, Scalar::all(255));
    fillConvexPoly(mask2, hulls2, Scalar::all(255));
    

   
    
//    imshow("a", mask1);
//    imshow("b", mask2);
//    waitKey();
    
    //根据凸包 拿到face1和face2图像
    Mat face1,face2;
    frame.copyTo(face1, mask1);
    frame.copyTo(face2, mask2);
   

    
    //拿到变换后的face1 face2
    Mat warpped_face1,warpped_face2;
    warpAffine(face1, warpped_face1, trans1, frame_size);
    warpAffine(face2, warpped_face2, trans2, frame_size);
    
    
    //==============需要把两个脸图拷贝到一张图,所以需要一个掩码================
    // 对face1多边形区域与face2多边形区域 仿射变换
    Mat warpped_mask1,warpped_mask2;
    warpAffine(mask1, warpped_mask1, trans1, frame_size);
    warpAffine(mask2, warpped_mask2, trans2, frame_size);
   
    
    //拷贝到一张图中
    Mat warpped_faces(frame_size, CV_8UC3, Scalar::all(0));
    warpped_face1.copyTo(warpped_faces, warpped_mask1);
    warpped_face2.copyTo(warpped_faces, warpped_mask2);
    
    
    
    /**
     * ======================优化部分===============================
     * 1、需要放置在face1区域的: rect1  face2的为:rect2
     * 2、rect1是将face2仿射变换而来 rect2相反
     * 3、可能会出现rect1与face1区域边缘不兼容 等情况
     * 4、使用与运算 只有重合的部分保留下来放入refined_warpped1中
     */
    
    //mask1是变换前face1区域 对应变换后的warpped_mask2
    //mat中face区域是255 其他都是0
    //进行与运算 得到变换前和变换后的不为0的重合区域mask
    Mat refined_warpped1,refined_warpped2;
    bitwise_and(mask1, warpped_mask2, refined_warpped1);
    bitwise_and(mask2, warpped_mask1, refined_warpped2);
    

    
    
    
   
    
    Size elementSize;
    //68个特征中 0在最左边 16在最右边
    int size = norm(hulls1[0] - hulls1[6]);
    elementSize.width = elementSize.height = size / 8;
    
   
//    Mat refined_mask1 = refined_masks(new_rect_face1);
//    Mat refined_mask2 = refined_masks(new_rect_face2);
   
   
    
    Mat element  = getStructuringElement(MORPH_RECT, elementSize);
    //虚化边缘
    erode(refined_warpped1, refined_warpped1, element);
    blur(refined_warpped1, refined_warpped1, elementSize);
   
    erode(refined_warpped2, refined_warpped2, element);
    blur(refined_warpped2, refined_warpped2, elementSize);
    
    
    //根据掩码拷贝像素
    //mast为0则不拷贝 不为0则拷贝src到dst dst创建出来全都0
    //把face1 变换前后的重叠部分 和 face2变换前后的重叠部分都放在一个mat中
    Mat refined_masks(frame_size, CV_8UC1, Scalar(0));
    refined_warpped1.copyTo(refined_masks, refined_warpped1);
    refined_warpped2.copyTo(refined_masks, refined_warpped2);

    
    for (int i = 0; i < frame.rows; ++i){
        uchar* frame_pixel = frame.row(i).data;
        uchar* faces_pixel = warpped_faces.row(i).data;
        uchar* masks_pixel = refined_masks.row(i).data;
        
        for (int j = 0; j < frame.cols; j++){
            //如果是0就是黑色 只要对不为0的部分进行操作
            if (*masks_pixel != 0){
                //数据为bgr
                //b
                //x+y x：扣掉masks_pixel之后的原图像素 + 脸的像素 = 合并像素
                *frame_pixel = ((0xff - *masks_pixel) * (*frame_pixel) + (*masks_pixel) * (*faces_pixel)) >> 8;
                //g
                *(frame_pixel + 1) = ((0xff - *(masks_pixel + 1)) * (*(frame_pixel + 1)) + (*(masks_pixel + 1)) * (*(faces_pixel + 1))) >> 8;
                //r
                *(frame_pixel + 2) = ((0xff - *(masks_pixel + 2)) * (*(frame_pixel + 2)) + (*(masks_pixel + 2)) * (*(faces_pixel + 2))) >> 8;
            }
            frame_pixel += 3;
            faces_pixel += 3;
            masks_pixel++;
        }
    }
    
    
   
   
    element.release();
    trans1.release();
    trans2.release();
    mask1.release();
    mask2.release();
    face1.release();
    face2.release();
    warpped_face1.release();
    warpped_face2.release();
    warpped_mask1.release();
    warpped_mask2.release();
    warpped_faces.release();
    refined_warpped1.release();
    refined_warpped2.release();
    refined_masks.release();
    //    imshow("1", mask1);
    //    imshow("2", warpped_faces);
    //    imshow("3", warpped_face1);
    //    imshow("4", warpped_face2);
    //    imshow("5", face1);
    //    imshow("6", face2);
    //    waitKey();
    
}






