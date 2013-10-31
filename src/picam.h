#ifndef _PICAM_H
#define _PICAM_H

#include <Python.h>
#include "interface/mmal/mmal.h"
typedef struct {      
    int exposure;
    int meterMode;
    int imageFX;
    int awbMode;
    int ISO;
    int sharpness;             /// -100 to 100
    int contrast;              /// -100 to 100
    int brightness;            ///  0 to 100
    int saturation;            ///  -100 to 100
    int videoStabilisation;    /// 0 or 1 (false or true)
    int exposureCompensation;  /// -10 to +10 ?
    int rotation;
    int hflip;                 /// 0 or 1
    int vflip;                 /// 0 or 1
    int shutter_speed;          //0 for auto, otherwise the shutter speed in ms
} PicamParams;

uint8_t *takePhoto(PicamParams *parms, long *sizeread);
uint8_t *takePhotoWithDetails(int width, int height, int quality, PicamParams *parms, long *sizeread);
uint8_t *takeRGBPhotoWithDetails(int width, int height, PicamParams *parms,long *sizeread); 
uint8_t *internelPhotoWithDetails(int width, int height, int quality,MMAL_FOURCC_T encoding,PicamParams *parms, long *sizeread); 
void internelVideoWithDetails(char *filename, int width, int height, int duration); 
#endif // _PICAM_H
