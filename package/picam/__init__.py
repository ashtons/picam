# Copyright (c) 2013 Sean Ashton
# Licensed under the terms of the MIT License (see LICENSE.txt)
import _picam
import StringIO
from PIL import Image
import ImageDraw
import RPi.GPIO as GPIO
import os
GPIO_AVAILABLE = True

try:
    #add disable_camera_led=1 to config.txt to have control over the LED
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(5, GPIO.OUT, initial=False) 
except:
    GPIO_AVAILABLE = False

def LEDOn():
    if GPIO_AVAILABLE:
        GPIO.output(5,True)
def LEDOff():
    if GPIO_AVAILABLE:
        GPIO.output(5,False)
        
def difference(list1,list2, tolerance):
    return _picam.difference(list1,list2, tolerance)
   
def takeRGBPhotoWithDetails(width, height):
    return _picam.takeRGBPhotoWithDetails(width, height)
        
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
    
def recordVideoWithDetails(filename, width, height, duration):
    directory = os.path.dirname(filename)
    if os.path.exists(directory):
        _picam.recordVideoWithDetails(filename, width, height, duration)
    else:
        raise Exception("Path does not exist!")
    
def saveRGBToImage(rgb_list, filename, width, height):
    im = Image.new("RGB", (width, height), "white")
    draw  =  ImageDraw.Draw(im)
    for i in range(0, len(rgb_list)):
        rgb = rgb_list[i]
        r = (rgb>>16) & 0x0ff
        g = (rgb>>8) & 0x0ff
        b = (rgb)    & 0x0ff
        x = i % width
        y = i / height
        color = (r,g,b)
        draw.point((x,(height-1)-y),color)
    im.save(filename)
