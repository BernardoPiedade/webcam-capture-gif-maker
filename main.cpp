#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fstream>
#include <string>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <vector>
#include <iomanip>
#include <sstream>

int getFrame(int file_decriptor, char *buffer, int num)
{

	v4l2_buffer bufferInfo;
	memset(&bufferInfo, 0, sizeof(bufferInfo));
	bufferInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufferInfo.memory = V4L2_MEMORY_MMAP;
	bufferInfo.index = 0;

	int type = bufferInfo.type;
	if (ioctl(file_decriptor, VIDIOC_STREAMON, &type) < 0)
	{
		perror("Could not start streaming");
		return 1;
	}

	if (ioctl(file_decriptor, VIDIOC_QBUF, &bufferInfo) < 0)
	{
		perror("Could not queue buffer");
		return 1;
	}

	if (ioctl(file_decriptor, VIDIOC_DQBUF, &bufferInfo) < 0)
	{
		perror("Could not dequeue buffer");
		return 1;
	}

	std::cout << "Buffer has: " << (double)bufferInfo.bytesused / 1024 << " KBytes of data" << std::endl;

	std::ofstream outFile;
	std::string fileName;
	std::stringstream ss;

	ss << "pic" << std::setfill('0') << std::setw(4) << std::to_string(num) << ".jpeg";
	fileName = ss.str();

	outFile.open(fileName, std::ios::binary | std::ios::app);

	int bufPos = 0, outFileMemBlockSize = 0;
	int remainingBufferSize = bufferInfo.bytesused;
	char *outFileMemBlock = NULL;
	int itr = 0;
	while (remainingBufferSize > 0)
	{

		bufPos += outFileMemBlockSize;
		outFileMemBlockSize = 1024;
		outFileMemBlock = new char[sizeof(char) * outFileMemBlockSize];
		memcpy(outFileMemBlock, buffer + bufPos, outFileMemBlockSize);
		outFile.write(outFileMemBlock, outFileMemBlockSize);

		if (outFileMemBlockSize > remainingBufferSize)
			outFileMemBlockSize = remainingBufferSize;

		remainingBufferSize -= outFileMemBlockSize;

		std::cout << itr++ << " Remaining bytes: " << remainingBufferSize << std::endl;

		delete outFileMemBlock;
	}

	outFile.close();

	if (ioctl(file_decriptor, VIDIOC_STREAMOFF, &type) < 0)
	{
		perror("Could not end streaming");
		return 1;
	}
	return 0;
}

int queryBufferRawData(int file_decriptor, int num)
{

	v4l2_buffer queryBuffer = {0};
	queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	queryBuffer.memory = V4L2_MEMORY_MMAP;
	queryBuffer.index = 0;

	if (ioctl(file_decriptor, VIDIOC_QUERYBUF, &queryBuffer) < 0)
	{
		perror("Device did not return the buffer information");
		return 1;
	}

	char *buffer = (char *)mmap(NULL, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, file_decriptor, queryBuffer.m.offset);
	memset(buffer, 0, queryBuffer.length);

	getFrame(file_decriptor, buffer, num);
	return 0;
}

int requestBuffers(int file_decriptor, int num)
{

	v4l2_requestbuffers reqBuffers = {0};
	reqBuffers.count = 1;
	reqBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqBuffers.memory = V4L2_MEMORY_MMAP;

	if (ioctl(file_decriptor, VIDIOC_REQBUFS, &reqBuffers) < 0)
	{
		perror("Could not request buffer form device");
		return 1;
	}

	queryBufferRawData(file_decriptor, num);
	return 0;
}

int setImageFormat(int file_decriptor, int num)
{

	v4l2_format imgFormat;
	imgFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	imgFormat.fmt.pix.width = 1024;
	imgFormat.fmt.pix.height = 1024;
	imgFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	imgFormat.fmt.pix.field = V4L2_FIELD_NONE;

	if (ioctl(file_decriptor, VIDIOC_S_FMT, &imgFormat) < 0)
	{
		perror("Device could not set format");
		return 1;
	}

	requestBuffers(file_decriptor, num);
	return 0;
}

int askToCapture(int file_decriptor, int num)
{

	v4l2_capability cap;
	if (ioctl(file_decriptor, VIDIOC_QUERYCAP, &cap) < 0)
	{
		perror("Failed to get device capabilities");
		return 1;
	}

	if (num > 0)
	{
		requestBuffers(file_decriptor, num);
	}

	setImageFormat(file_decriptor, num);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc == 1)
	{
		printf("\nERROR:\n\n\tprogram must be used like this: ./main.exe <num_photo> .i.e.: ./main.exe 100\n\n");
		return 1;
	}

	if (std::stoi(argv[1]) > 3600)
	{
		printf("you cannot give a value bigger than 3600"); //3600 photos equals 10 minutes of video
		return 1;
	}

	int num = 0;
	int file_decriptor;
	file_decriptor = open("/dev/video0", O_RDWR);
	if (file_decriptor < 0)
	{
		perror("failed to open device");
		return 1;
	}

	while (true)
	{

		if (num == std::stoi(argv[1]))
		{
			system("ffmpeg -start_number 0 -framerate 25 -i pic%04d.jpeg -c:v libx264 -profile:v high -crf 20 -pix_fmt yuv420p _v_.mp4 && rm -rf pic*.jpeg");
			system("ffmpeg -i _v_.mp4 -filter:v \"setpts=3.8*PTS\" output.mp4 && rm -rf _v_.mp4");

			break;
		}

		askToCapture(file_decriptor, num);
		num++;
	}
	return 0;
}