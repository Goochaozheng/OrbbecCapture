from datetime import datetime
import numpy as np
import time
import os
import cv2

from glob import glob
from os.path import join
from openni import openni2
from openni import _openni2 as c_api
import openni

width = 640
height = 480

def colormap(depth):
    # Apply colormap
    depth[depth > 10000.] = 0
    depthColored = cv2.convertScaleAbs(depth, alpha=(255.0/10000.0))
    depthColored = cv2.applyColorMap(depthColored, cv2.COLORMAP_JET)
    depthColored[depth==0] = np.array([0,0,0])
    return depthColored


if __name__ == "__main__":

    save = False

    # Set the path to the orbbec SDK openni lib
    libpath = "D:/Orbbec/Tools/Windows_V2.3.0.66/windows/SDK/x64/Redist"
    openni2.initialize(libpath)
    dev = openni2.Device.open_any()
    print(dev.get_device_info())

    dev.set_depth_color_sync_enabled(True)
    # fail to set registration mode
    # dev.set_image_registration_mode(openni2.IMAGE_REGISTRATION_DEPTH_TO_COLOR)

    depth_stream = dev.create_depth_stream()
    depth_stream.start()

    color_capture = cv2.VideoCapture(0)
    print("Backend: ", color_capture.get(cv2.CAP_PROP_BACKEND))

    if save:
        outDir = join("../", str(time.time()))
        os.mkdir(outDir)
        os.mkdir(join(outDir, 'rgb'))
        os.mkdir(join(outDir, 'depth'))
        
        frameCount = 0
        timestampList = []  

    while True:

        # Read frame from buffer
        print("Depth Timestamp: ", time.time())
        depthBuf = depth_stream.read_frame().get_buffer_as_uint16()
        print("Color Timestamp: ", time.time())
        ret, color = color_capture.read()

        # Parser depth frame
        depth = np.frombuffer(depthBuf, dtype=np.uint16).reshape(height, width) 
        depth = cv2.flip(depth, 1)
        
        if save:
            # Save Image
            timestamp = time.time()
            depthFilename = os.path.join(outDir, "depth", "{:.06f}.png".format(timestamp))
            colorFilename = os.path.join(outDir, "rgb", "{:.06f}.png".format(timestamp))
            cv2.imwrite(depthFilename, depth.astype(np.uint16))
            cv2.imwrite(colorFilename, color)
            timestampList.append(timestamp)
            print("Frame Saved: {:06d}".format(frameCount))
            frameCount += 1
        
        # Display
        cv2.imshow('depth', colormap(depth))
        cv2.imshow('color', color)
        overlay = cv2.addWeighted(color, 0.7, colormap(depth), 0.5, 0)
        cv2.imshow('Rectified', overlay)        

        key = cv2.waitKey(10) & 0xFF
        if key == 27: # Esc
            break

    if save:
        # Create depth.txt & rgb.txt  
        depthFile = open(join(outDir, "depth.txt"), 'w')
        rgbFile = open(join(outDir, "rgb.txt"), 'w')
        associationFile = open(join(outDir, "association.txt"), 'w')

        for timestamp in timestampList:

            depthPath = "depth/{:.06f}.png".format(timestamp)
            rgbPath = "rgb/{:.06f}.png".format(timestamp)

            depthFile.write("{:.06f} {}\n".format(timestamp, depthPath))
            rgbFile.write("{:.06f} {}\n".format(timestamp, rgbPath))
            associationFile.write("{:.06f} {} {:.06f} {}\n".format(timestamp, depthPath, timestamp, rgbPath))

        depthFile.close()
        rgbFile.close()
        associationFile.close()

    depth_stream.close()
    color_capture.release()
    dev.close()
