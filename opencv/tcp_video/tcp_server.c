/* LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libcamera/v4l2-compat.so  */
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <opencv2/highgui/highgui.hpp>
#include <sys/socket.h>
#include <unistd.h>

#define CAM_WIDTH 800
#define CAM_HEIGHT 600
#define PORT 5000

int main() {
  cv::VideoCapture vc(0);
  if (!vc.isOpened()) {
    std::cerr << "Error: Cannot open webcam" << std::endl;
    return EXIT_FAILURE;
  }

  vc.set(cv::CAP_PROP_FRAME_WIDTH, CAM_WIDTH);
  vc.set(cv::CAP_PROP_FRAME_HEIGHT, CAM_HEIGHT);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Error: Cannot create socket" << std::endl;
    return EXIT_FAILURE;
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    std::cerr << "Error: setsockopt failed" << std::endl;
    return EXIT_FAILURE;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << "Error: Bind failed" << std::endl;
    return EXIT_FAILURE;
  }

  if (listen(server_fd, 1) < 0) {
    std::cerr << "Error: Listen failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Waiting for client connection..." << std::endl;

  int addrlen = sizeof(address);
  int client_socket =
      accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
  if (client_socket < 0) {
    std::cerr << "Error: Accept failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Client connected: " << inet_ntoa(address.sin_addr) << ":"
            << ntohs(address.sin_port) << std::endl;

  cv::Mat frame(CAM_HEIGHT, CAM_WIDTH, CV_8UC3,
                cv::Scalar(255)); /* 영상을 위한 변수 */
  const size_t frame_size = CAM_WIDTH * CAM_HEIGHT * 3; // 3 channels for BGR

  while (true) {
    if (!vc.isOpened()) {
      perror("OpenCV : open WebCam\n");
      return EXIT_FAILURE;
    }
    vc >> frame;
    if (frame.empty()) {
      std::cerr << "Error: Frame is empty" << std::endl;
      break;
    }
    if (frame.total() * frame.elemSize() != frame_size) {
      std::cerr << "Error: Unexpected frame size" << std::endl;
      break;
    }
    /* print how much data sent per frame. */
    std::cout << "Frame data size: " << frame_size << std::endl;

    size_t total_sent = 0;
    while (total_sent < frame_size) {
      ssize_t sent = send(client_socket, frame.data + total_sent,
                          frame_size - total_sent, 0);
      if (sent < 0) {
        std::cerr << "Error: Failed to send frame data" << std::endl;
        break;
      }
      total_sent += sent;
    }

    if (total_sent != frame_size) {
      std::cerr << "Error: Failed to send complete frame" << std::endl;
      break;
    }
  }

  close(client_socket);
  close(server_fd);
  vc.release();

  return 0;
}