// ONI_capture.cpp : Defines the entry point for the application.
//

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <direct.h>
#include <ctime>
#include <Windows.h>

#include "OpenNI.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

using namespace std;

#define SAMPLE_READ_WAIT_TIMEOUT 5000 //2000ms


int main(int argc, char** argv)
{

	bool saveFrame = true;

	std::string outDir = "..\\CapturedSequence\\";
	std::time_t timestamp = std::time(0);
	std::stringstream ss;
	ss << timestamp;
	std::string ts = ss.str();
	std::string outputFolder = std::string(outDir + ts);

	std::string depthFolder = std::string(outputFolder + "\\depth\\");
	std::string colorFolder = std::string(outputFolder + "\\color\\");

	if (saveFrame) {

		if ((CreateDirectory(outputFolder.c_str(), NULL)) &&
			(CreateDirectory(depthFolder.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) &&
			(CreateDirectory(colorFolder.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()))
		{
			cout << "Saving frame to: " << outputFolder << "\n";
		}
		else {
			cout << "Fail to create output folder: " << outputFolder << "\n";
		}
	}

	openni::Status result = openni::STATUS_OK;

	//OpenNI2 Frame
	openni::VideoFrameRef oniDepthImg;
	cv::Mat mImageDepth_colormap, mImageDepth_scale, mImageColor, mImageOverlay;

	char key = 0;

	// Set up openni
	result = openni::OpenNI::initialize();
	if (result != openni::STATUS_OK)
	{
		printf("Initialize failed\n%s\n", openni::OpenNI::getExtendedError());
		return 1;
	}

	// Open camera device
	openni::Array<openni::DeviceInfo> deviceList;
	openni::OpenNI::enumerateDevices(&deviceList);
	int cc = deviceList.getSize();
	if (cc <= 0)
	{
		cout << "No device found\n";
		return -1;
	}
	const char* deviceUri;
	deviceUri = deviceList[0].getUri();
	cout << "Device URI: " << deviceUri << "\n";

	openni::Device device;
	result = device.open(deviceUri);

	// set depth video mode
	openni::VideoMode modeDepth;
	modeDepth.setResolution(640, 480);
	modeDepth.setFps(30);
	modeDepth.setPixelFormat(openni::PIXEL_FORMAT_DEPTH_1_MM);

	// create depth stream
	openni::VideoStream oniDepthStream;
	oniDepthStream.setVideoMode(modeDepth);
	oniDepthStream.create(device, openni::SENSOR_DEPTH);
	int changedStreamDummy;
	openni::VideoStream* pStream = &oniDepthStream;

	if (device.isImageRegistrationModeSupported(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR))
	{
		device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);
	}
	device.setDepthColorSyncEnabled(true);

	float vfov = oniDepthStream.getVerticalFieldOfView();
	float hfov = oniDepthStream.getHorizontalFieldOfView();

	cout << "FOV: " << vfov << " : " << hfov << "\n";

	// start depth stream
	result = oniDepthStream.start();  
	if (result != openni::STATUS_OK) {
		cout << "Fail to open depth stream\n";
	}

	// start color stream
	cv::VideoCapture colorStream(0, cv::CAP_DSHOW);
	if (!colorStream.isOpened()) {
		cout << "Fail to open color stream\n";
	}
	colorStream.set(cv::CAP_PROP_BUFFERSIZE, 3);
	
	cout << "FPS: " << colorStream.get(cv::CAP_PROP_FPS) << "\n";
	cout << "Buffer Size: " << colorStream.get(cv::CAP_PROP_BUFFERSIZE) << "\n";
	cout << "SAR: " << colorStream.get(cv::CAP_PROP_SAR_NUM) << "\n";

	string depthWinName = "Depth";
	string colorWinName = "Color";
	string overlayWinName = "Overlay";

	cv::namedWindow(depthWinName, 1);
	cv::namedWindow(colorWinName, 1);
	cv::namedWindow(overlayWinName, 1);

	int frameCount = 0;

	while ((key != 27)) // ESC
	{
		openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, SAMPLE_READ_WAIT_TIMEOUT);
		oniDepthStream.readFrame(&oniDepthImg);
		colorStream >> mImageColor;

		// Parser depth
		const cv::Mat mImageDepth(oniDepthImg.getHeight(), oniDepthImg.getWidth(), CV_16UC1, (void*)oniDepthImg.getData());
		cv::flip(mImageDepth, mImageDepth, 1);
		mImageDepth.convertTo(mImageDepth_scale, CV_8U, 255.0 / 8000.0);
		cv::threshold(mImageDepth_scale, mImageDepth_scale, 8000.0, 8000.0, cv::THRESH_TOZERO_INV);
		cv::applyColorMap(mImageDepth_scale, mImageDepth_colormap, cv::COLORMAP_JET);
		
		for (int v = 0; v < mImageDepth_colormap.rows; v++){
			uchar* p_colormap = mImageDepth_colormap.ptr<uchar>(v);
			uchar* p_scale= mImageDepth_scale.ptr<uchar>(v);
			for (int u = 0; u < mImageDepth_colormap.cols; u++) {
				if (p_scale[u] == 0) {
					p_colormap[u * 3 + 0] = 0;
					p_colormap[u * 3 + 1] = 0;
					p_colormap[u * 3 + 2] = 0;
				}
			}
		}
		
		// Show img
		cv::imshow(depthWinName, mImageDepth_colormap);
		cv::imshow(colorWinName, mImageColor);

		cv::addWeighted(mImageColor, 0.7, mImageDepth_colormap, 0.5, 0, mImageOverlay);
		cv::imshow(overlayWinName, mImageOverlay);

		// Save frame
		if (saveFrame) {
			std::stringstream fs;
			fs << std::setfill('0') << std::setw(6) << frameCount;
			std::string countStr = fs.str();
			std::string depthFileName = std::string(depthFolder + countStr + ".depth.png");
			std::string colorFileName = std::string(colorFolder + countStr + ".color.jpg");

			cv::imwrite(depthFileName, mImageDepth);
			cv::imwrite(colorFileName, mImageColor);
		}

		key = cv::waitKey(1);
		frameCount++;

	}

	//destroy
	oniDepthStream.destroy(); 
	device.close();
	openni::OpenNI::shutdown();
	colorStream.release();
	return 0;
}