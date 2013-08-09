picam
=====

Raspberry Pi (RPi) Camera Module Python Library


This module will allow some basic functionality to use the camera module from within Python without starting a new process to take the photo.

Based largely on a stripped down version of the raspistill and raspivid code, with a python wrapper.

Returns a PIL Image object


    import picam
    import time
    
    i = picam.takePhoto()
    
    ii = picam.takePhotoWithDetails(640,480, 85)
    
    filename = "/tmp/picam-%s.h264" % time.strftime("%Y%m%d-%H%M%S")
    picam.recordVideoWithDetails(filename,640,480,5000)
    
    picam.LEDOn()
    picam.LEDOff()
    
    picam.config.imageFX = picam.MMAL_PARAM_IMAGEFX_WATERCOLOUR
    picam.config.exposure = picam.MMAL_PARAM_EXPOSUREMODE_AUTO
    picam.config.meterMode = picam.MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE
    picam.config.awbMode = picam.MMAL_PARAM_AWBMODE_SHADE

Installation
------------
Download the package folder and run the setup command to install the script

    python setup.py install
