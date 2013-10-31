#include <Python.h>
#include "structmember.h"
#include <stdio.h>
#include <stdlib.h>
#include "picam.h"
#include "encode.h"
#include "interface/mmal/mmal.h"

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define DICT_SET(dict,val) PyModule_AddIntConstant(dict, #val, val);


typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
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
    int rotation;              /// 0-359
    int hflip;                 /// 0 or 1
    int vflip;                 /// 0 or 1
    int shutter_speed;         /// 0 = auto, otherwise the shutter speed in ms
} _PicamConfig;

static void PicamConfig_dealloc(_PicamConfig* self) {    
    self->ob_type->tp_free((PyObject*)self);
}

static int Picam_init(_PicamConfig *self, PyObject *args, PyObject *kwds) {
    return 0;
}

static PyObject *Picam_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    _PicamConfig *self;

    self = (_PicamConfig *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->exposure = MMAL_PARAM_EXPOSUREMODE_AUTO;       
        self->meterMode = MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE;       
        self->imageFX = MMAL_PARAM_IMAGEFX_NONE;    
        self->awbMode = MMAL_PARAM_AWBMODE_AUTO;       
        self->ISO = 0;
        self->sharpness = 0;           
        self->contrast = 0;              
        self->brightness= 50;          
        self->saturation = 0;           
        self->videoStabilisation = 0;    /// 0 or 1 (false or true)
        self->exposureCompensation = 0 ; 
        self->rotation = 0;
        self->hflip = 0;
        self->vflip = 0;
        self->shutter_speed = 0;
    }
    return (PyObject *)self;
}
static PyMemberDef PicamConfig_members[] = {
    {"exposure", T_INT, offsetof(_PicamConfig, exposure), 0, "exposure"},
    {"meterMode",T_INT, offsetof(_PicamConfig, meterMode), 0, "meterMode"},
    {"imageFX",  T_INT, offsetof(_PicamConfig, imageFX), 0, "imageFX"},
    {"awbMode",  T_INT, offsetof(_PicamConfig, awbMode), 0, "awbMode"},   
    {"ISO",  T_INT, offsetof(_PicamConfig, ISO), 0, "ISO"},       
    {"sharpness",  T_INT, offsetof(_PicamConfig, sharpness), 0, "sharpness"},       
    {"contrast",  T_INT, offsetof(_PicamConfig, contrast), 0, "contrast"},       
    {"brightness",  T_INT, offsetof(_PicamConfig, brightness), 0, "brightness"},       
    {"saturation",  T_INT, offsetof(_PicamConfig, saturation), 0, "saturation"},       
    {"videoStabilisation",  T_INT, offsetof(_PicamConfig, videoStabilisation), 0, "videoStabilisation"}, 
    {"exposureCompensation",  T_INT, offsetof(_PicamConfig, exposureCompensation), 0, "exposureCompensation"},     
    {"rotation",  T_INT, offsetof(_PicamConfig, rotation), 0, "rotation"},     
    {"hflip",  T_INT, offsetof(_PicamConfig, hflip), 0, "hflip"},     
    {"vflip",  T_INT, offsetof(_PicamConfig, vflip), 0, "vflip"},     
    {"shutterSpeed",  T_INT, offsetof(_PicamConfig, shutter_speed), 0, "0 = auto, otherwise the shutter speed in ms"},     
    
    {NULL}  /* Sentinel */
};
static PyTypeObject PicamConfigType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "picam.Config",             /*tp_name*/
    sizeof(_PicamConfig),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PicamConfig_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "PicamConfig objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                      /* tp_methods */
    PicamConfig_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Picam_init,      /* tp_init */
    0,                         /* tp_alloc */
    Picam_new,                 /* tp_new */
};
static _PicamConfig* picam_newconfig()
{
    _PicamConfig* o = PyObject_New(_PicamConfig, &PicamConfigType);

    if (o != 0) {        
        o->exposure = MMAL_PARAM_EXPOSUREMODE_AUTO;       
        o->meterMode = MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE;       
        o->imageFX = MMAL_PARAM_IMAGEFX_NONE;    
        o->awbMode = MMAL_PARAM_AWBMODE_AUTO; 
        o->ISO = 0;  
        o->sharpness = 0;           
        o->contrast = 0;              
        o->brightness= 50;          
        o->saturation = 0;           
        o->videoStabilisation = 0;    /// 0 or 1 (false or true)
        o->exposureCompensation = 0 ; 
        o->rotation = 0;
        o->hflip = 0;
        o->vflip = 0;
        o->shutter_speed = 0;
    }    
    return o;
}

static _PicamConfig *picamConfig = NULL;

static PyObject * picam_listtest(PyObject *self, PyObject *args) {   
    PyObject *V = PyList_New(3);
    PyList_SetItem(V, 0,PyInt_FromLong(1L));
    PyList_SetItem(V, 1, PyInt_FromLong(2L));
    PyList_SetItem(V, 2, PyInt_FromLong(3L));       
    return Py_BuildValue("N", V);
}

static void fillParms(PicamParams *parms) {   
    parms->exposure = picamConfig->exposure;
    parms->meterMode = picamConfig->meterMode;
    parms->imageFX = picamConfig->imageFX;
    parms->awbMode = picamConfig->awbMode;
    parms->ISO = picamConfig->ISO;    
    parms->sharpness = picamConfig->sharpness;             /// -100 to 100
    parms->contrast = picamConfig->contrast;              /// -100 to 100
    parms->brightness = picamConfig->brightness;            ///  0 to 100
    parms->saturation = picamConfig->saturation;            ///  -100 to 100
    parms->videoStabilisation = picamConfig->videoStabilisation;    /// 0 or 1 (false or true)
    parms->exposureCompensation = picamConfig->exposureCompensation;  /// -10 to +10 ?
    parms->rotation = picamConfig->rotation;
    parms->hflip =  picamConfig->hflip;
    parms->vflip = picamConfig->vflip;     
    parms->shutter_speed = picamConfig->shutter_speed;
}

static PyObject * picam_takephoto(PyObject *self, PyObject *args) {
    PyObject *result = Py_None;                           
    long bufsize = 0l;    
    //printf("%d %d %d %d %d\n",picamConfig->exposure, picamConfig->meterMode, picamConfig->imageFX, picamConfig->awbMode, picamConfig->ISO); 
    PicamParams parms;
    fillParms(&parms);
    char *buffer = (char *)takePhoto(&parms, &bufsize);                    
    result = Py_BuildValue("s#", buffer, bufsize);
    free(buffer);      
    return result;
}

static PyObject *picam_difference(PyObject *self, PyObject *args) { 
    PyObject *list1 = Py_None;
    PyObject *list2 = Py_None;
    int threshold;
    if (!PyArg_ParseTuple(args,"OOi",&list1,&list2,&threshold)) {
       return NULL;
    }
    Py_ssize_t size1 =  PyList_Size(list1);
    Py_ssize_t size2 =  PyList_Size(list2);
    long quantity = 0l;
    if (size1 == size2) {    
        PyObject *listResult = PyList_New(size1);
        int loop = 0;
        long rgb1 = 0;
        long rgb2 = 0;
        int r1,g1,b1;
        int r2,g2,b2;
        long val = 0;
        for (loop=0;loop<size1;loop++) {
            
            rgb1 = PyInt_AsLong(PyList_GetItem(list1, loop));
            rgb2 = PyInt_AsLong(PyList_GetItem(list2, loop)); 
                                
            r1 = (rgb1>>16) & 0x0ff;
            g1 = (rgb1>>8) & 0x0ff;
            b1 = (rgb1)    & 0x0ff;
            
            r2 = (rgb2>>16) & 0x0ff;
            g2 = (rgb2>>8) & 0x0ff;
            b2 = (rgb2)    & 0x0ff;
            
            int i1 = abs(r1-r2);
            int i2 = abs(g1-g2);
            int i3 = abs(b1-b2);
            
            if (threshold != 0) {
                if ((i1 > threshold) || (i2 > threshold) || (i3 > threshold) ) {
                    i1 = 255;
                    i2 = 255;
                    i3 = 255;   
                    quantity++;                 
                }
            }
            
            int valR = CLAMP(i1,0,255);
            int valG = CLAMP(i2,0,255);
            int valB = CLAMP(i3,0,255);
            val = (valR << 16) + (valG << 8) + valB;
            PyList_SetItem(listResult, loop, PyInt_FromLong(val));
            
        }
        
        PyObject *result = Py_BuildValue("NN", listResult, PyInt_FromLong(quantity));  
        return result;
        
    }
    return NULL;
}


static PyObject *picam_takergbphotowithdetails(PyObject *self, PyObject *args) {   
    int width;
    int height;   
    if (!PyArg_ParseTuple(args,"ii",&width,&height)) {
       return NULL;
    }
    long bufsize = 0l;
    PicamParams parms;
    fillParms(&parms);
    char *buffer = (char *)takeRGBPhotoWithDetails(width, height,&parms, &bufsize); 
     
    long bufMinusHeader = bufsize-54;    
    long listSize = bufMinusHeader / 3;   
    PyObject *listResult = PyList_New(listSize);
    int i;
    int ii = 0;
    for (i=54;i<bufsize;i+=3) {        
        long val = (buffer[i+2] << 16) | (buffer[i+1] << 8) | buffer[i];                    
        PyList_SetItem(listResult, (i-54)/3,PyInt_FromLong(val));        
        ii++;
    }
    free(buffer);    
    
    return Py_BuildValue("N", listResult);   
}

static PyObject * picam_takephotowithdetails(PyObject *self, PyObject *args) {
    PyObject *result = Py_None;
    int width;
    int height;
    int quality;    
    if (!PyArg_ParseTuple(args,"iii",&width,&height,&quality)) {
       return NULL;
    }
    long bufsize = 0l;
    PicamParams parms;
    fillParms(&parms);
    char *buffer = (char *)takePhotoWithDetails(width, height, quality, &parms, &bufsize);   
    result = Py_BuildValue("s#", buffer, bufsize);
    free(buffer);      
    return result;
}

static PyObject * picam_recordvideowithdetails(PyObject *self, PyObject *args) {
    PyObject *result = Py_None;
    int width;
    int height;
    int duration;
    char *filename;
    if (!PyArg_ParseTuple(args,"siii",&filename, &width,&height,&duration)) {
       return NULL;
    }
    internelVideoWithDetails(filename, width, height, duration);  
    Py_INCREF(result);
    return result;
}

static PyMethodDef PiCamMethods[] = {
    
    {"takePhoto",  picam_takephoto, METH_VARARGS, "Take a basic photo."},  
    {"takePhotoWithDetails",  picam_takephotowithdetails, METH_VARARGS, "Take a  photo with width, height and quality."},    
    {"takeRGBPhotoWithDetails",  picam_takergbphotowithdetails, METH_VARARGS, "Take a photo and return as RGB array."}, 
    {"difference",  picam_difference, METH_VARARGS, "Difference between 2 RGB arrays."}, 
    {"recordVideoWithDetails",  picam_recordvideowithdetails, METH_VARARGS, "Record a video with width, height and duration."}, 
    {"listTest", picam_listtest,  METH_VARARGS, "Test returning a list"}, 
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

void setupExposureConstants(PyObject *module_dict) {   
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_OFF);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_AUTO);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_NIGHT);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_NIGHTPREVIEW);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_BACKLIGHT);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_SPOTLIGHT);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_SPORTS);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_SNOW);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_BEACH);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_VERYLONG);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_FIXEDFPS);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_ANTISHAKE);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMODE_FIREWORKS);
}
void setupAWBConstants(PyObject *module_dict) {   
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_OFF);
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_AUTO);
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_SUNLIGHT);
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_CLOUDY);
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_SHADE);
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_TUNGSTEN);
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_FLUORESCENT);
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_INCANDESCENT);
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_FLASH);
    DICT_SET(module_dict,MMAL_PARAM_AWBMODE_HORIZON);   
}

void setupMeteringConstants(PyObject *module_dict) {   
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMETERINGMODE_SPOT);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMETERINGMODE_BACKLIT);
    DICT_SET(module_dict,MMAL_PARAM_EXPOSUREMETERINGMODE_MATRIX);     
}
void setupImageFXConstants(PyObject *module_dict) {    
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_NONE);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_NEGATIVE);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_SOLARIZE);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_SKETCH);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_DENOISE);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_EMBOSS);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_OILPAINT);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_HATCH);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_GPEN);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_PASTEL);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_WATERCOLOUR);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_BLUR);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_SATURATION);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_COLOURSWAP);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_WASHEDOUT);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_POSTERISE);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_COLOURPOINT);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_COLOURBALANCE);
    DICT_SET(module_dict,MMAL_PARAM_IMAGEFX_CARTOON);  
}


PyMODINIT_FUNC
init_picam(void)
{
    PyObject *module;  
    
    if (PyType_Ready(&PicamConfigType) < 0)
        return;
    module = Py_InitModule("_picam", PiCamMethods);         
    setupExposureConstants(module);    
    setupAWBConstants(module);
    setupMeteringConstants(module);
    setupImageFXConstants(module);
    picamConfig = picam_newconfig();
    Py_INCREF(picamConfig);
    PyModule_AddObject(module, "config", (PyObject *)picamConfig); 
    //http://docs.python.org/2/extending/newtypes.html
}