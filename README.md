picam
=====

Raspberry Pi (RPi) Camera Module Python Library


This module will allow some basic functionality to use the camera module from within Python without starting a new process to take the photo.

Based largely on a stripped down version of the raspistill and raspivid code, with a python wrapper.

Returns a PIL Image object


    import picam
    import time
    
    i = picam.takePhoto()
    i.save('/tmp/test.jpg')
    
    # (width, height, jpg quality)
    ii = picam.takePhotoWithDetails(640,480, 85) 
    
    filename = "/tmp/picam-%s.h264" % time.strftime("%Y%m%d-%H%M%S")
    
    # (width, height, duration = 5s)
    picam.recordVideoWithDetails(filename,640,480,5000) 
    
    #RGB pixel info
    frame1 = picam.takeRGBPhotoWithDetails(width,height)
    frame2 = picam.takeRGBPhotoWithDetails(width,height)
    
    #returns RGB pixel list with modified pixels, and the quantity of changed pixels
    (modified,q) = picam.difference(frame1,frame2,THRESHOLD)
    
    picam.LEDOn()
    picam.LEDOff()
    
    picam.config.imageFX = picam.MMAL_PARAM_IMAGEFX_WATERCOLOUR
    picam.config.exposure = picam.MMAL_PARAM_EXPOSUREMODE_AUTO
    picam.config.meterMode = picam.MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE
    picam.config.awbMode = picam.MMAL_PARAM_AWBMODE_SHADE
    picam.config.ISO = 400
    picam.config.ISO = 800
    
    picam.config.sharpness = 0               # -100 to 100
    picam.config.contrast = 0                # -100 to 100
    picam.config.brightness = 50             #  0 to 100
    picam.config.saturation = 0              #  -100 to 100
    picam.config.videoStabilisation = 0      # 0 or 1 (false or true)
    picam.config.exposureCompensation  = 0   # -10 to +10 ?
    picam.config.rotation = 90               # 0-359
    picam.config.hflip = 1                   # 0 or 1
    picam.config.vflip = 0                   # 0 or 1
    
    
Installation
------------
Download the  folder and run the setup command to install the script

    python setup.py install
    
    or

    pip install https://github.com/ashtons/picam/zipball/master#egg=picam

Constants
------------
    MMAL_PARAM_EXPOSUREMODE_OFF
    MMAL_PARAM_EXPOSUREMODE_AUTO
    MMAL_PARAM_EXPOSUREMODE_NIGHT
    MMAL_PARAM_EXPOSUREMODE_NIGHTPREVIEW
    MMAL_PARAM_EXPOSUREMODE_BACKLIGHT
    MMAL_PARAM_EXPOSUREMODE_SPOTLIGHT
    MMAL_PARAM_EXPOSUREMODE_SPORTS
    MMAL_PARAM_EXPOSUREMODE_SNOW
    MMAL_PARAM_EXPOSUREMODE_BEACH
    MMAL_PARAM_EXPOSUREMODE_VERYLONG
    MMAL_PARAM_EXPOSUREMODE_FIXEDFPS
    MMAL_PARAM_EXPOSUREMODE_ANTISHAKE
    MMAL_PARAM_EXPOSUREMODE_FIREWORKS

    MMAL_PARAM_AWBMODE_OFF
    MMAL_PARAM_AWBMODE_AUTO
    MMAL_PARAM_AWBMODE_SUNLIGHT
    MMAL_PARAM_AWBMODE_CLOUDY
    MMAL_PARAM_AWBMODE_SHADE
    MMAL_PARAM_AWBMODE_TUNGSTEN
    MMAL_PARAM_AWBMODE_FLUORESCENT
    MMAL_PARAM_AWBMODE_INCANDESCENT
    MMAL_PARAM_AWBMODE_FLASH
    MMAL_PARAM_AWBMODE_HORIZON   

    MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE
    MMAL_PARAM_EXPOSUREMETERINGMODE_SPOT
    MMAL_PARAM_EXPOSUREMETERINGMODE_BACKLIT
    MMAL_PARAM_EXPOSUREMETERINGMODE_MATRIX     

    MMAL_PARAM_IMAGEFX_NONE
    MMAL_PARAM_IMAGEFX_NEGATIVE
    MMAL_PARAM_IMAGEFX_SOLARIZE
    MMAL_PARAM_IMAGEFX_SKETCH
    MMAL_PARAM_IMAGEFX_DENOISE
    MMAL_PARAM_IMAGEFX_EMBOSS
    MMAL_PARAM_IMAGEFX_OILPAINT
    MMAL_PARAM_IMAGEFX_HATCH
    MMAL_PARAM_IMAGEFX_GPEN
    MMAL_PARAM_IMAGEFX_PASTEL
    MMAL_PARAM_IMAGEFX_WATERCOLOUR
    MMAL_PARAM_IMAGEFX_BLUR
    MMAL_PARAM_IMAGEFX_SATURATION
    MMAL_PARAM_IMAGEFX_COLOURSWAP
    MMAL_PARAM_IMAGEFX_WASHEDOUT
    MMAL_PARAM_IMAGEFX_POSTERISE
    MMAL_PARAM_IMAGEFX_COLOURPOINT
    MMAL_PARAM_IMAGEFX_COLOURBALANCE
    MMAL_PARAM_IMAGEFX_CARTOON  
