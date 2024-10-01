/* g++ -o helloopencv helloopencv.cpp  `pkg-config --cflags --libs opencv4` */
#include <opencv2/highgui/highgui.hpp>

using namespace cv;

int main() 
{
    Mat image(300, 400, CV_8UC1, Scalar(255));
    imshow("Hello World!", image);
    waitKey(0);

    return 0;
}