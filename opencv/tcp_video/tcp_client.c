#include <errno.h>
#include <fcntl.h> /* low-level i/o */
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define RECV_PORT 5000

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/fb.h>

#include <asm/types.h> /* for videodev2.h */
#include <linux/videodev2.h>
#include <opencv4/opencv2/core/core.hpp>
#include <opencv4/opencv2/highgui/highgui.hpp>
#include <opencv4/opencv2/imgproc/imgproc.hpp>
#include <opencv4/opencv2/objdetect/objdetect.hpp>

#define FBDEV "/dev/fb0" /* 프레임 버퍼를 위한 디바이스 파일 */
#define VIDEODEV "/dev/video0"
#define CAMERA_COUNT 1000000
#define WIDTH 800 /* 캡쳐받을 영상의 크기 */
#define HEIGHT 600
using namespace cv;

const static char *cascade_name =
    "/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml";

/* Video4Linux에서 사용할 영상 저장을 위한 버퍼 */
struct buffer {
  void *start;
  size_t length;
};

struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;
static struct fb_var_screeninfo
    vinfo; /* 프레임버퍼의 정보 저장을 위한 구조체 */

static void mesg_exit(const char *s) {
  fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
  exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg) {
  int r;
  do
    r = ioctl(fd, request, arg);
  while (-1 == r && EINTR == errno);
  return r;
}
using namespace std;

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("not found args!!!");
    exit(0);
  }
  // arg
  int pSock;
  pSock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in cliaddr;
  // connect server Socket
  memset(&cliaddr, 0, sizeof(cliaddr));
  cliaddr.sin_family = AF_INET;
  // recv 소켓 생성
  // inet_pton(AF_INET, "localhost", &(cliaddr.sin_addr.s_addr));
  inet_pton(AF_INET, "192.168.0.42", &(cliaddr.sin_addr.s_addr));
  cliaddr.sin_port = htons(RECV_PORT);
  if (connect(pSock, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0) {
    printf("connect error");
    perror("connect()");
    return -1;
  }

  int fbfd = -1;  /* 프레임버퍼의 파일 디스크립터 */
  int camfd = -1; /* 카메라의 파일 디스크립터 */

  unsigned char *buffer;
  short *pfbmap;

  VideoCapture vc(0);
  CascadeClassifier cascade;

  // casecade
  if (!cascade.load(cascade_name)) {
    perror("load()");
    return EXIT_FAILURE;
  }

  if (!vc.isOpened()) {
    perror("OpenCV: open WebCam\n");
    return EXIT_FAILURE;
  }
  vc.set(CAP_PROP_FRAME_WIDTH, WIDTH);
  vc.set(CAP_PROP_FRAME_HEIGHT, HEIGHT);

  /* open FrameBuffer */
  fbfd = open(FBDEV, O_RDWR);
  if (-1 == fbfd) {
    perror("open( ) : framebuffer device");
    return EXIT_FAILURE;
  }

  /* get FrameBuffer Infomation */
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
    perror("Error reading variable information.");
    return EXIT_FAILURE;
  }

  /* mmap( ) : alloc memory */

  printf("vinfo xres = %d\n", vinfo.xres);
  printf("vinfo yres = %d\n", vinfo.yres);
  printf("vinfo bits_per_pixel = %d\n", vinfo.bits_per_pixel);

  long screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8.;
  pfbmap = (short *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                         fbfd, 0);
  if (pfbmap == (short *)-1) {
    perror("mmap() : framebuffer device to memory");
    return EXIT_FAILURE;
  }

  memset(pfbmap, 0, screensize);

  /* 카메라 장치 열기 */
  camfd = open(VIDEODEV, O_RDWR | O_NONBLOCK, 0);
  if (-1 == camfd) {
    fprintf(stderr, "Cannot open '%s': %d, %s\n", VIDEODEV, errno,
            strerror(errno));
    return EXIT_FAILURE;
  }

  Mat frame(HEIGHT, WIDTH, CV_8UC3, Scalar(255)); // openCV Mat
  printf("frame.col %d\n", frame.cols);           // 800	width
  printf("frame.row %d\n", frame.rows);           // 600	height

  socklen_t clen = sizeof(cliaddr);
  Point pt1, pt2;

  char nextFrame[BUFSIZ] = {
      0,
  };
  int remainSize = 0;

  for (int i = 0; i < CAMERA_COUNT; i++) {
    char mesg[800 * 600 * 3] = {
        0,
    };
    int bufIdx = 0;
    size_t n;

    if (remainSize > 0) {
      memcpy(mesg, nextFrame, remainSize);
      bufIdx += remainSize;
      memset(nextFrame, 0, BUFSIZ);
      remainSize = 0;
    }

    for (i = 0;; i++) {
      char temp[BUFSIZ] = {
          0,
      };
      int size = recv(pSock, temp, BUFSIZ, 0);
      if (bufIdx + size >= 800 * 600 * 3) {
        // size = (800*600*3) - bufIdx;
        int a = (800 * 600 * 3) - bufIdx;
        remainSize = size - a;
        memcpy(nextFrame, temp + size + 1, remainSize);
      }
      memcpy(mesg + bufIdx, temp, size);
      bufIdx += size;
      if (remainSize > 0)
        break;
    }

    memcpy(frame.data, mesg, 800 * 600 * 3);

    Mat image(HEIGHT, WIDTH, CV_8UC1, Scalar(255));
    cvtColor(frame, image, COLOR_BGR2GRAY);
    equalizeHist(image, image);

    std::vector<Rect> faces;
    cascade.detectMultiScale(image, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE,
                             Size(30, 30));
    // find face
    for (int j = 0; j < faces.size(); j++) {
      pt1.x = faces[j].x;
      pt2.x = faces[j].x + faces[j].width;
      pt1.y = faces[j].y;
      pt2.y = faces[j].y + faces[j].height;
      rectangle(frame, pt1, pt2, Scalar(255, 0, 0), 3, 8);
    }

    // print frameBuffer
    buffer = (unsigned char *)frame.data;
    for (int y = 0, location = 0; y < frame.rows; y++) {
      for (int x = 0; x < vinfo.xres; x++) {
        if (x >= frame.cols) {
          location++;
          continue;
        }

        unsigned char b = (*(buffer + (y * frame.cols + x) * 3 + 0)) >> 3;
        unsigned char g = (*(buffer + (y * frame.cols + x) * 3 + 1)) >> 2;
        unsigned char r = (*(buffer + (y * frame.cols + x) * 3 + 2)) >> 3;

        short fbColor = (r << 11) | (g << 5) | (b);

        pfbmap[location++] = fbColor;
      }
    }
  }

  munmap(pfbmap, screensize);
  close(fbfd);
  return 0;

  /* 캡쳐 중단 */
  enum v4l2_buf_type type;
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == xioctl(camfd, VIDIOC_STREAMOFF, &type))
    mesg_exit("VIDIOC_STREAMOFF");

  /* 메모리 정리 */
  for (int i = 0; i < n_buffers; ++i)
    if (-1 == munmap(buffers[i].start, buffers[i].length))
      mesg_exit("munmap");
  free(buffers);

  // munmap(fbp, screensize);

  /* 장치 닫기 */
  if (-1 == close(camfd) && -1 == close(fbfd))
    mesg_exit("close");

  return EXIT_SUCCESS;
}
