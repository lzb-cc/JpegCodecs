
#include <stdio.h>
#include "JpegDecoder.h"
#include "BmpEncoder.h"

using namespace JpegCodec;

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Usage: main input.jpg out.bmp\n");
		return 0;
	}

	JpegDecoder decoder(argv[1]);
	Matrix mat;
    	decoder.Decoder(mat);

	/* ����Ϊ Bitmap ��ʽͼ�� */
	int size;
	unsigned char *bitmap = Encoder(mat.data, mat.rows, mat.cols, size);
	Write(argv[2], bitmap, size);
	return 0;
}
