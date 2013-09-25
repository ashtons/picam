#ifndef _PICAM_H
#define _PICAM_H

#include <Python.h>
#include "interface/mmal/mmal.h"
uint8_t *takePhoto(int exposure, int meterMode, int imageFx, int awbMode, int ISO, long *sizeread);
uint8_t *takePhotoWithDetails(int width, int height, int quality, int exposure, int meterMode, int imageFx,int awbMode,int ISO, long *sizeread);
uint8_t *takeRGBPhotoWithDetails(int width, int height, int exposure, int meterMode, int imageFx, int awbMode, int ISO,long *sizeread); 
uint8_t *internelPhotoWithDetails(int width, int height, int quality,MMAL_FOURCC_T encoding,int exposure, int meterMode, int imageFx, int awbMode,int ISO, long *sizeread); 
void internelVideoWithDetails(char *filename, int width, int height, int duration); 
#endif // _PICAM_H
