// demo for the use of houghPeak to detect n-gon in the point array
// given the cv::Mat src,cv::Rect ROI ,bool debugflag for debuging

#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>
#include <cmath>
#include "houghPeak.h"
struct LinePolar
{
        float rho;
        float angle;
};

bool debugflag = 1;
cv::Mat src;
cv::Rect rect;
// first , to enlarge the background then put the ROI on the center
int main()
{
    cv::Mat regionsOfinterest(src,rect);
    cv::Mat img;
    regionsOfinterest.copyTo(img);
    cv::Mat roi_enlarged(rect.height*2, rect.width *2,CV_8UC1,cv::Scalar(0));
    cv::Mat roi_enlarged_temp(roi_enlarged,cv::Rect(rect.width - rect.width/2,
                                                    rect.height - rect.height/2,
                                                    rect.width,rect.height));

    if(img.channels() == 3)
        cv::cvtColor(img,img,CV_RGB2GRAY);
    if(img.channels() != 1)
        std::cout << "something wrong with the channel" <<  std::endl;
    if(debugflag)
        cv::imshow("boundingrect",img);\
    cv::Canny(img,img,40,120);
    if(debugflag)
        cv::imshow("canny",img);
    img.copyTo(roi_enlarged_temp);
    if(debugflag)
        cv::imshow("canny,roi enlarged",roi_enlarged);

// morpholo then adaptiveThreshold to extract the contour
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 5) );
    cv::Mat img_close;
    cv::Mat img_morpholo;
    cv::morphologyEx(roi_enlarged, img_close, CV_MOP_CLOSE, element);
    cv::morphologyEx(img_close, roi_enlarged, CV_MOP_OPEN, element);
    if(debugflag)
        cv::imshow("canny->morpholo",roi_enlarged);

    cv::adaptiveThreshold(roi_enlarged,roi_enlarged,255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY,5,0);
    if(debugflag)
        cv::imshow("outerline",roi_enlarged);
        

// sometimes the copyTo fun may change the type of the pixels
    cv::Mat dst(roi_enlarged.size(),CV_8UC3,cv::Scalar(0,0,0));
    if(roi_enlarged.channels() == 3)
        cv::cvtColor(roi_enlarged,roi_enlarged,CV_RGB2GRAY);
    if(roi_enlarged.channels() != 1)
        std::cout << "channels of roi_enlarged error" << std::endl;



    //extract all the edge points to the edgepoints container
    cv::MatIterator_<uchar> it, itbeg, itend;
    itbeg = roi_enlarged.begin<uchar>();
    itend = roi_enlarged.end<uchar>();
    std::vector<cv::Point> edgepoints;
    for( it = itbeg;it != itend; ++it)
        if( *it != 0)
            edgepoints.push_back(cv::Point( (it - itbeg)%roi_enlarged.step,
                                            (it - itbeg)/roi_enlarged.step));
    std::vector<std::vector<cv::Point>> linespoint;
    std::vector<LinePolar> lines;
    std::vector<cv::Point2f> vertexes;
    
    //
    double distance = 3;       
    unsigned int ngon =4;      // n - gon to be detected
    LinePolar linepolar;
    for(int i = 0;i< ngon;++i){
        HoughLinesPeak( linepolar,edgepoints,roi_enlarged.size(), 1, 0.001, 0., CV_PI );
        lines.push_back(linepolar);
        float rho = lines[i].rho;
        float theta = lines[i].angle;
        double a = cos(theta), b = sin(theta);
        double x0 = a*rho, y0 = b*rho;
        cv::Point imax(cvRound(x0 + 1000*(-b)),  cvRound(y0 + 1000*(a))); // get two points for the line
        cv::Point jmax(cvRound(x0 - 1000*(-b)),  cvRound(y0 - 1000*(a)));
        cv::line( dst, imax, jmax, cv::Scalar(0,0,255), 1, 8 );
        //all the points within the distance are to be eliminated
        auto iter = edgepoints.begin();
        while(iter != edgepoints.end()){
            double dis = fabs((jmax.x - imax.x)*((*iter).y - imax.y) -
                                    (jmax.y - imax.y)*((*iter).x - imax.x))
                         / sqrt((jmax.x - imax.x)*(jmax.x - imax.x)
                                 + (jmax.y - imax.y)*(jmax.y - imax.y));
            if(dis < distance)
                iter = edgepoints.erase(iter);
            else ++iter;
        }
    }

    cv::imshow("houghline",dst);
    
    for(size_t i = 0;i < lines.size(); ++i){
        for(size_t j = i+1; j < lines.size();++j){
            cv::Matx22f A(std::cos(lines[i].angle), std::sin(lines[i].angle),
                          std::cos(lines[j].angle), std::sin(lines[j].angle));
            cv::Matx21f b(lines[i].rho,lines[j].rho),x;
            if( std::fabs(lines[i].angle- lines[j].angle) > CV_PI/4){
                cv::solve(A,b,x,cv::DECOMP_LU);         //方程组Ax = b 求x
                cv::Point vertex;
                vertex.x = cvRound(x(0,0));
                vertex.y = cvRound(x(0,1));
                vertexes.push_back(vertex);
            }
        }
    }
    // sort vertex in the order of up-left -> up-right -> down-right -> down-left
    // first to find one point in the quadrangle, then to determine where all the relative positions of vertexes are
    cv::Point central = cv::Point(0,0);
    for(auto a : vertexes){
        central.x += a.x;
        central.y += a.y;
    }
    central = cv::Point(central.x /4.0 , central.y/4.0);
    std::vector<cv::Point2f> vertexes_sorted = vertexes;

    for(auto a :vertexes){
        if (a.x < central.x && a.y < central.y)
            vertexes_sorted[0] = a;
        if (a.x > central.x && a.y < central.y)
            vertexes_sorted[1] = a;
        if (a.x > central.x && a.y > central.y)
            vertexes_sorted[2] = a;
        if (a.x < central.x && a.y > central.y)
            vertexes_sorted[3] = a;
    }
    for(auto a :vertexes_sorted)  std::cout << a <<std::endl;
    for(auto i : vertexes_sorted)
        cv::circle(dst,i,5,cv::Scalar(0,255,0));
    cv::imshow("houghNgon",dst);
    cv::waitKey();
}
