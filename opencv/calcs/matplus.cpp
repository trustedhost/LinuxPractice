#include <opencv2/highgui/highgui.hpp>

using namespace cv;

int main() {
    Mat image = imread("mandrill.jpg", IMREAD_COLOR);
    
    image += 50;

    imshow("Image show", image);

    waitKey(0);

    return 0;


}