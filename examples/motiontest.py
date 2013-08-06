import picam
from PIL import Image
from array import array
import time
width = 100
height = 100

THRESHOLD = 15
QUANITY_MIN = 50
frame1 = picam.takeRGBPhotoWithDetails(width,height)
while True:
    frame2 = picam.takeRGBPhotoWithDetails(width,height)
    (_,q) = picam.difference(frame1,frame2,THRESHOLD)
    print q
    if q > QUANITY_MIN:
        picam.LEDOn()
    else:
        picam.LEDOff()
    frame1 = frame2
    