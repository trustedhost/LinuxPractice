/* 칼라로 된 bmp 파일을 흑백으로 바꾸는 코드. */
#include <limits.h> /* USHRT_MAX 상수를 위해서 사용한다. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmpHeader.h"

/* 이미지 데이터의 경계 검사를 위한 매크로 */
#define LIMIT_UBYTE(n) ((n) > UCHAR_MAX) ? UCHAR_MAX : ((n) < 0) ? 0 : (n)

typedef unsigned char ubyte;

int main(int argc, char **argv) {
  FILE *fp;
  BITMAPFILEHEADER bmpHeader;     /* BMP FILE INFO */
  BITMAPINFOHEADER bmpInfoHeader; /* BMP IMAGE INFO */
  RGBQUAD *palrgb;
  ubyte *inimg, *outimg;
  int x, y, z, imageSize;

  if (argc != 3) {
    fprintf(stderr, "usage : %s input.bmp output.bmp\n", argv[0]);
    return -1;
  }

  /***** read bmp *****/
  if ((fp = fopen(argv[1], "rb")) == NULL) {
    fprintf(stderr, "Error : Failed to open file...₩n");
    return -1;
  }

  /* BITMAPFILEHEADER 구조체의 데이터 */
  fread(&bmpHeader, sizeof(BITMAPFILEHEADER), 1, fp);

  /* BITMAPINFOHEADER 구조체의 데이터 */
  fread(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

  /* 트루 컬러를 지원하면 변환할 수 없다. */
  if (bmpInfoHeader.biBitCount != 24) {
    perror("This image file doesn't supports 24bit color\n");
    fclose(fp);
    return -1;
  }

  int elemSize = bmpInfoHeader.biBitCount / 8;
  int size = bmpInfoHeader.biWidth * elemSize;
  imageSize = size * bmpInfoHeader.biHeight;

  /* 이미지의 해상도(넓이 × 깊이) */
  printf("Resolution : %d x %d\n", bmpInfoHeader.biWidth,
         bmpInfoHeader.biHeight);
  printf("Bit Count : %d(%d)\n", bmpInfoHeader.biBitCount,
         elemSize); /* 픽셀당 비트 수(색상) */
  printf("Image Size : %d\n", imageSize);

  inimg = (ubyte *)malloc(sizeof(ubyte) * imageSize);
  outimg = (ubyte *)malloc(sizeof(ubyte) * bmpInfoHeader.biWidth *
                           bmpInfoHeader.biHeight);
  fread(inimg, sizeof(ubyte), imageSize, fp);

  fclose(fp);

  for (y = 0; y < bmpInfoHeader.biHeight; y++) {
    for (x = 0; x < size; x += elemSize) {
      ubyte b = inimg[x + y * size + 0];
      ubyte g = inimg[x + y * size + 1];
      ubyte r = inimg[x + y * size + 2];
      // outimg[x/elemSize+y*bmpInfoHeader.biWidth]=((66*r+129*g+25*b+128)>>8)+16;
      outimg[x / elemSize + y * bmpInfoHeader.biWidth] =
          (r * 0.299F) + (g * 0.587F) + (b * 0.114F);
    }
  }

  /***** write bmp *****/
  if ((fp = fopen(argv[2], "wb")) == NULL) {
    fprintf(stderr, "Error : Failed to open file...₩n");
    return -1;
  }

  palrgb = (RGBQUAD *)malloc(sizeof(RGBQUAD) * 256);
  for (x = 0; x < 256; x++) {
    palrgb[x].rgbBlue = palrgb[x].rgbGreen = palrgb[x].rgbRed = x;
    palrgb[x].rgbReserved = 0;
  }

  bmpInfoHeader.biBitCount = 8;
  bmpInfoHeader.SizeImage = bmpInfoHeader.biWidth * bmpInfoHeader.biHeight *
                            bmpInfoHeader.biBitCount / 8;
  bmpInfoHeader.biCompression = 0;
  bmpInfoHeader.biClrUsed = 0;
  bmpInfoHeader.biClrImportant = 0;

  bmpHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
                        sizeof(RGBQUAD) * 256;
  bmpHeader.bfSize = bmpHeader.bfOffBits + bmpInfoHeader.SizeImage;

  /* BITMAPFILEHEADER 구조체의 데이터 */
  fwrite(&bmpHeader, sizeof(BITMAPFILEHEADER), 1, fp);

  /* BITMAPINFOHEADER 구조체의 데이터 */
  fwrite(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

  fwrite(palrgb, sizeof(RGBQUAD), 256, fp);

  // fwrite(inimg, sizeof(ubyte), imageSize, fp);
  fwrite(outimg, sizeof(ubyte), bmpInfoHeader.biWidth * bmpInfoHeader.biHeight,
         fp);

  fclose(fp);

  free(inimg);
  free(outimg);

  return 0;
}
