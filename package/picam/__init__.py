# Copyright (c) 2013 Sean Ashton
# Licensed under the terms of the MIT License (see LICENSE.txt)
import _picam
import StringIO
from PIL import Image
import RPi.GPIO as GPIO

#add disable_camera_led=1 to config.txt to have control over the LED
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(5, GPIO.OUT, initial=False) 

def LEDOn():
    GPIO.output(5,True)
def LEDOff():
    GPIO.output(5,False)
    
def takePhoto():
    s = _picam.takePhoto()
    ss = StringIO.StringIO(s)
    i = Image.open(ss)
    return i     

def takePhotoWithDetails(width, height, quality):
    s = _picam.takePhotoWithDetails(width, height, quality)
    ss = StringIO.StringIO(s)
    i = Image.open(ss)
    return i 
