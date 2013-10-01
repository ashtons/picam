#include "picam.h"
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#define VERSION_STRING "v1.2"

#include "bcm_host.h"
#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

#include "RaspiCamControl.h"
#include "Imaging.h"
#include <semaphore.h>

/// Camera number to use - we only have one camera, indexed from 0.
#define CAMERA_NUMBER 0
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

// Stills format information
#define STILLS_FRAME_RATE_NUM 15
#define STILLS_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3
// Video format information
#define VIDEO_FRAME_RATE_NUM 30
#define VIDEO_FRAME_RATE_DEN 1

// Max bitrate we allow for recording
const int MAX_BITRATE = 30000000; // 30Mbits/s

/// Interval at which we check for an failure abort during capture
const int ABORT_INTERVAL = 100; // ms

int mmal_status_to_int(MMAL_STATUS_T status);


/** Structure containing all state information for the current run
 */
typedef struct
{  
   int width;                          /// Requested width of image
   int height;                         /// requested height of image
   int quality;                        /// JPEG quality setting (1-100)  
   uint8_t *filedata;
   long bytesStored;
   
   int videoEncode; 
   /* Video */
   int bitrate;                        /// Requested bitrate
   int framerate;                      /// Requested frame rate (fps)
   int intraperiod;                    /// Intra-refresh period (key frame rate)
   char *filename;                     /// filename of output file
   int immutableInput; 
   
   /* End Video */
   MMAL_FOURCC_T encoding;             /// Encoding to use for the output file.   
  
   
   RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters

   MMAL_COMPONENT_T *preview_component;    
   MMAL_COMPONENT_T *camera_component;    /// Pointer to the camera component
   MMAL_COMPONENT_T *encoder_component;   /// Pointer to the encoder component
   MMAL_COMPONENT_T *null_sink_component; /// Pointer to the null sink component
   MMAL_CONNECTION_T *preview_connection; /// Pointer to the connection from camera to preview
   MMAL_CONNECTION_T *encoder_connection; /// Pointer to the connection from camera to encoder

   MMAL_POOL_T *encoder_pool; /// Pointer to the pool of buffers used by encoder output port

} RASPISTILL_STATE;



/** Struct used to pass information in encoder port userdata to callback
 */
typedef struct
{   
   VCOS_SEMAPHORE_T complete_semaphore; /// semaphore which is posted when we reach end of frame (indicates end of capture or fault)
   RASPISTILL_STATE *pstate;            /// pointer to our state in case required in callback
   FILE *file_handle;                   /// File handle to write buffer data to.
   int abort;                           /// Set to 1 in callback if an error occurs to attempt to abort the capture
   
} PORT_USERDATA;


/**
 * Assign a default set of parameters to the state passed in
 *
 * @param state Pointer to state structure to assign defaults to
 */
static void default_status(RASPISTILL_STATE *state)
{
   if (!state) {
      vcos_assert(0);
      return;
   }   
   state->width = 2592;
   state->height = 1944;
   state->quality = 85;   
   state->bytesStored = 0l;
   /*Video*/
                    
   //state->bitrate = 17000000;
   state->bitrate = 5000000;
   state->framerate = VIDEO_FRAME_RATE_NUM;
   state->immutableInput = 0;
   state->intraperiod = 0;    // Not set
    
   state->camera_component = NULL;
   state->encoder_component = NULL;   
   state->encoder_connection = NULL;
   state->encoder_pool = NULL;
   state->encoding = MMAL_ENCODING_JPEG; //MMAL_ENCODING_BMP  
   raspicamcontrol_set_defaults(&state->camera_parameters);
   //state->camera_parameters.exposureMode = MMAL_PARAM_EXPOSUREMODE_NIGHT;
   //state->camera_parameters.exposureMeterMode = MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE;
}




/**
 *  buffer header callback function for camera control
 *
 *  No actions taken in current version
 *
 * @param port Pointer to port from which callback originated
 * @param buffer mmal buffer header pointer
 */
static void camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   if (buffer->cmd == MMAL_EVENT_PARAMETER_CHANGED) {
   } else {
      vcos_log_error("Received unexpected camera control callback event, 0x%08x", buffer->cmd);
   }

   mmal_buffer_header_release(buffer);
}

/**
 *  buffer header callback function for encoder
 *
 *  Callback will dump buffer data to the specific file
 *
 * @param port Pointer to port from which callback originated
 * @param buffer mmal buffer header pointer
 */
static void encoder_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   int complete = 0;

   // We pass our file handle and other stuff in via the userdata field.

   PORT_USERDATA *pData = (PORT_USERDATA *)port->userdata;
   
   if (pData) {
        
       RASPISTILL_STATE *state = pData->pstate;   
       int bytes_written = buffer->length;
       if (state->videoEncode == 1) {          
           vcos_assert(pData->file_handle);
           if (buffer->length) {
               mmal_buffer_header_mem_lock(buffer);
               bytes_written = fwrite(buffer->data, 1, buffer->length, pData->file_handle);
               mmal_buffer_header_mem_unlock(buffer);
           }
           if (bytes_written != buffer->length) {
               vcos_log_error("Failed to write buffer data (%d from %d)- aborting", bytes_written, buffer->length);
               pData->abort = 1;
           }
       } else {    
          if (buffer->length) {
              mmal_buffer_header_mem_lock(buffer);             
              if ((state->bytesStored > 0) && (bytes_written > 0)) {
                  long newLen = state->bytesStored + bytes_written;
                  uint8_t *new_buffer = malloc(newLen);
                  memcpy(new_buffer, state->filedata, state->bytesStored);
                  memcpy(new_buffer + state->bytesStored, buffer->data, bytes_written);
              
                  free(state->filedata); //Free previous buffer
              
                  state->filedata = new_buffer;             
                  state->bytesStored =  state->bytesStored + bytes_written;             
              } else {
                  state->filedata = malloc(bytes_written);
                  memcpy(state->filedata, buffer->data, bytes_written);
                  state->bytesStored = bytes_written;
              }         
              mmal_buffer_header_mem_unlock(buffer);                 
          }
       }
       // Now flag if we have completed
       if (buffer->flags & (MMAL_BUFFER_HEADER_FLAG_FRAME_END | MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED))
         complete = 1;
   } else {
      vcos_log_error("Received a encoder buffer callback with no state");
   }

   // release buffer back to the pool
   mmal_buffer_header_release(buffer);

   // and send one back to the port (if still open)
   if (port->is_enabled) {
      MMAL_STATUS_T status = MMAL_SUCCESS;
      MMAL_BUFFER_HEADER_T *new_buffer;

      new_buffer = mmal_queue_get(pData->pstate->encoder_pool->queue);

      if (new_buffer) {
         status = mmal_port_send_buffer(port, new_buffer);
      }
      if (!new_buffer || status != MMAL_SUCCESS)
         vcos_log_error("Unable to return a buffer to the encoder port");
   }

   if (complete)
      vcos_semaphore_post(&(pData->complete_semaphore));

}


/**
 * Create the camera component, set up its ports
 *
 * @param state Pointer to state control struct
 *
 * @return MMAL_SUCCESS if all OK, something else otherwise
 *
 */
static MMAL_STATUS_T create_video_camera_component(RASPISTILL_STATE *state)
{
   MMAL_COMPONENT_T *camera = 0;
   MMAL_ES_FORMAT_T *format;
   MMAL_PORT_T  *video_port = NULL, *still_port = NULL;
   MMAL_STATUS_T status;

   /* Create the component */
   status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);

   if (status != MMAL_SUCCESS) {
      vcos_log_error("Failed to create camera component");
      goto error;
   }

   if (!camera->output_num) {
      status = MMAL_ENOSYS;
      vcos_log_error("Camera doesn't have output ports");
      goto error;
   }

   
   video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
   still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

   // Enable the camera, and tell it its control callback function
   status = mmal_port_enable(camera->control, camera_control_callback);

   if (status != MMAL_SUCCESS)  {
      vcos_log_error("Unable to enable control port : error %d", status);
      goto error;
   }

   //  set up the camera configuration
   
   MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
      {
         { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
         .max_stills_w = state->width,
         .max_stills_h = state->height,
         .stills_yuv422 = 0,
         .one_shot_stills = 1,
         .max_preview_video_w = 640,
         .max_preview_video_h = 480,
         .num_preview_video_frames = 3,
         .stills_capture_circular_buffer_height = 0,
         .fast_preview_resume = 0,
         .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
      };
   mmal_port_parameter_set(camera->control, &cam_config.hdr);
   
   raspicamcontrol_set_all_parameters(camera, &state->camera_parameters);
   // Now set up the port formats

   // Set the encode format on the video  port

   format = video_port->format;
   format->encoding_variant = MMAL_ENCODING_I420;

   format->encoding = MMAL_ENCODING_OPAQUE;
   format->es->video.width = state->width;
   format->es->video.height = state->height;
   format->es->video.crop.x = 0;
   format->es->video.crop.y = 0;
   format->es->video.crop.width = state->width;
   format->es->video.crop.height = state->height;
   format->es->video.frame_rate.num = state->framerate;
   format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;

   status = mmal_port_format_commit(video_port);

   if (status != MMAL_SUCCESS) {
      vcos_log_error("camera video format couldn't be set");
      goto error;
   }

   // Ensure there are enough buffers to avoid dropping frames
   if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;


   // Set the encode format on the still  port

   format = still_port->format;

   format->encoding = MMAL_ENCODING_OPAQUE;
   if (state->videoEncode == 1) {
       format->encoding_variant = MMAL_ENCODING_I420;
   }
   format->es->video.width = state->width;
   format->es->video.height = state->height;
   format->es->video.crop.x = 0;
   format->es->video.crop.y = 0;
   format->es->video.crop.width = state->width;
   format->es->video.crop.height = state->height;
   format->es->video.frame_rate.num = STILLS_FRAME_RATE_NUM;
   format->es->video.frame_rate.den = STILLS_FRAME_RATE_DEN;

   status = mmal_port_format_commit(still_port);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("camera still format couldn't be set");
      goto error;
   }

   /* Ensure there are enough buffers to avoid dropping frames */
   if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

   /* Enable component */
   status = mmal_component_enable(camera);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("camera component couldn't be enabled");
      goto error;
   }

   

   state->camera_component = camera;

   

   return status;

error:

   if (camera)
      mmal_component_destroy(camera);

   return status;
}
/**
 * Destroy the camera component
 *
 * @param state Pointer to state control struct
 *
 */
static void destroy_camera_component(RASPISTILL_STATE *state)
{
   if (state->camera_component) {
      mmal_component_destroy(state->camera_component);
      state->camera_component = NULL;
   }
}



/**
 * Create the encoder component, set up its ports
 *
 * @param state Pointer to state control struct. encoder_component member set to the created camera_component if successfull.
 *
 * @return a MMAL_STATUS, MMAL_SUCCESS if all OK, something else otherwise
 */
static MMAL_STATUS_T create_encoder_component(RASPISTILL_STATE *state)
{
   MMAL_COMPONENT_T *encoder = 0;
   MMAL_PORT_T *encoder_input = NULL, *encoder_output = NULL;
   MMAL_STATUS_T status;
   MMAL_POOL_T *pool;

   status = mmal_component_create(MMAL_COMPONENT_DEFAULT_IMAGE_ENCODER, &encoder);

   if (status != MMAL_SUCCESS) {
      vcos_log_error("Unable to create JPEG encoder component");
      goto error;
   }

   if (!encoder->input_num || !encoder->output_num) {
      status = MMAL_ENOSYS;
      vcos_log_error("JPEG encoder doesn't have input/output ports");
      goto error;
   }

   encoder_input = encoder->input[0];
   encoder_output = encoder->output[0];

   // We want same format on input and output
   mmal_format_copy(encoder_output->format, encoder_input->format);

   // Specify out output format
   encoder_output->format->encoding = state->encoding;

   encoder_output->buffer_size = encoder_output->buffer_size_recommended;

   if (encoder_output->buffer_size < encoder_output->buffer_size_min)
      encoder_output->buffer_size = encoder_output->buffer_size_min;

   encoder_output->buffer_num = encoder_output->buffer_num_recommended;

   if (encoder_output->buffer_num < encoder_output->buffer_num_min)
      encoder_output->buffer_num = encoder_output->buffer_num_min;

   // Commit the port changes to the output port
   status = mmal_port_format_commit(encoder_output);

   if (status != MMAL_SUCCESS) {
      vcos_log_error("Unable to set format on video encoder output port");
      goto error;
   }

   // Set the JPEG quality level
   status = mmal_port_parameter_set_uint32(encoder_output, MMAL_PARAMETER_JPEG_Q_FACTOR, state->quality);

   if (status != MMAL_SUCCESS)  {
      vcos_log_error("Unable to set JPEG quality");
      goto error;
   }

   
   //  Enable component
   status = mmal_component_enable(encoder);

   if (status  != MMAL_SUCCESS) {
      vcos_log_error("Unable to enable video encoder component");
      goto error;
   }

   /* Create pool of buffer headers for the output port to consume */
   pool = mmal_port_pool_create(encoder_output, encoder_output->buffer_num, encoder_output->buffer_size);

   if (!pool) {
      vcos_log_error("Failed to create buffer header pool for encoder output port %s", encoder_output->name);
   }

   state->encoder_pool = pool;
   state->encoder_component = encoder;

   return status;

   error:

   if (encoder)
      mmal_component_destroy(encoder);

   return status;
}
/**
 * Create the encoder component, set up its ports
 *
 * @param state Pointer to state control struct
 *
 * @return MMAL_SUCCESS if all OK, something else otherwise
 *
 */
static MMAL_STATUS_T create_video_encoder_component(RASPISTILL_STATE *state)
{
   MMAL_COMPONENT_T *encoder = 0;
   MMAL_PORT_T *encoder_input = NULL, *encoder_output = NULL;
   MMAL_STATUS_T status;
   MMAL_POOL_T *pool;

   status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER, &encoder);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to create video encoder component");
      goto error;
   }

   if (!encoder->input_num || !encoder->output_num)
   {
      status = MMAL_ENOSYS;
      vcos_log_error("Video encoder doesn't have input/output ports");
      goto error;
   }

   encoder_input = encoder->input[0];
   encoder_output = encoder->output[0];

   // We want same format on input and output
   mmal_format_copy(encoder_output->format, encoder_input->format);

   // Only supporting H264 at the moment

   encoder_output->format->encoding = MMAL_ENCODING_H264;
   
   encoder_output->format->bitrate = state->bitrate;

   encoder_output->buffer_size = encoder_output->buffer_size_recommended;

   if (encoder_output->buffer_size < encoder_output->buffer_size_min)
      encoder_output->buffer_size = encoder_output->buffer_size_min;

   encoder_output->buffer_num = encoder_output->buffer_num_recommended;

   if (encoder_output->buffer_num < encoder_output->buffer_num_min)
      encoder_output->buffer_num = encoder_output->buffer_num_min;

   // Commit the port changes to the output port
   status = mmal_port_format_commit(encoder_output);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to set format on video encoder output port");
      goto error;
   }


   // Set the rate control parameter
   if (0)
   {
      MMAL_PARAMETER_VIDEO_RATECONTROL_T param = {{ MMAL_PARAMETER_RATECONTROL, sizeof(param)}, MMAL_VIDEO_RATECONTROL_DEFAULT};
      status = mmal_port_parameter_set(encoder_output, &param.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set ratecontrol");
         goto error;
      }

   }

   if (state->intraperiod)
   {
      MMAL_PARAMETER_UINT32_T param = {{ MMAL_PARAMETER_INTRAPERIOD, sizeof(param)}, state->intraperiod};
      status = mmal_port_parameter_set(encoder_output, &param.hdr);
      if (status != MMAL_SUCCESS)
      {
         vcos_log_error("Unable to set intraperiod");
         goto error;
      }

   }

   if (mmal_port_parameter_set_boolean(encoder_input, MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT, state->immutableInput) != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to set immutable input flag");
      // Continue rather than abort..
   }

   //  Enable component
   status = mmal_component_enable(encoder);

   if (status != MMAL_SUCCESS)
   {
      vcos_log_error("Unable to enable video encoder component");
      goto error;
   }

   /* Create pool of buffer headers for the output port to consume */
   pool = mmal_port_pool_create(encoder_output, encoder_output->buffer_num, encoder_output->buffer_size);

   if (!pool)
   {
      vcos_log_error("Failed to create buffer header pool for encoder output port %s", encoder_output->name);
   }

   state->encoder_pool = pool;
   state->encoder_component = encoder;

   

   return status;

   error:
   if (encoder)
      mmal_component_destroy(encoder);

   return status;
}
/**
 * Destroy the encoder component
 *
 * @param state Pointer to state control struct
 *
 */
static void destroy_encoder_component(RASPISTILL_STATE *state)
{
   // Get rid of any port buffers first
   if (state->encoder_pool) {
      mmal_port_pool_destroy(state->encoder_component->output[0], state->encoder_pool);
   }

   if (state->encoder_component) {
      mmal_component_destroy(state->encoder_component);
      state->encoder_component = NULL;
   }
}


/**
 * Connect two specific ports together
 *
 * @param output_port Pointer the output port
 * @param input_port Pointer the input port
 * @param Pointer to a mmal connection pointer, reassigned if function successful
 * @return Returns a MMAL_STATUS_T giving result of operation
 *
 */
static MMAL_STATUS_T connect_ports(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection)
{
   MMAL_STATUS_T status;

   status =  mmal_connection_create(connection, output_port, input_port, MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);

   if (status == MMAL_SUCCESS) {
      status =  mmal_connection_enable(*connection);
      if (status != MMAL_SUCCESS)
         mmal_connection_destroy(*connection);
   }

   return status;
}

/**
 * Checks if specified port is valid and enabled, then disables it
 *
 * @param port  Pointer the port
 *
 */
static void check_disable_port(MMAL_PORT_T *port)
{
   if (port && port->is_enabled)
      mmal_port_disable(port);
}

uint8_t *takePhoto(PicamParams *parms, long *sizeread) { 
    long test = 0l;    
    uint8_t *tmp = internelPhotoWithDetails(2592,1944,85, MMAL_ENCODING_JPEG, parms, &test);        
    *sizeread = test;   
    return tmp;
}

uint8_t *takePhotoWithDetails(int width, int height, int quality, PicamParams *parms, long *sizeread) {
    long test = 0l;    
    uint8_t *tmp = internelPhotoWithDetails(width,height,quality,MMAL_ENCODING_JPEG,parms, &test);        
    *sizeread = test;   
    return tmp;
}

uint8_t *takeRGBPhotoWithDetails(int width, int height, PicamParams *parms, long *sizeread) {
    long test = 0l;    
    uint8_t *tmp = internelPhotoWithDetails(width,height,100, MMAL_ENCODING_BMP, parms, &test);            
    *sizeread = test;   
    return tmp;
}

uint8_t *internelPhotoWithDetails(int width, int height, int quality,MMAL_FOURCC_T encoding, PicamParams *parms, long *sizeread) {
   RASPISTILL_STATE state;   
   MMAL_STATUS_T status = MMAL_SUCCESS;   
   
   MMAL_PORT_T *preview_input_port = NULL;
   MMAL_PORT_T *camera_preview_port = NULL;
   MMAL_PORT_T *camera_still_port = NULL;
   MMAL_PORT_T *encoder_input_port = NULL;
   MMAL_PORT_T *encoder_output_port = NULL;  
   if (width > 2592) {
       width = 2592;
   } else if (width < 20) {
       width = 20;
   }
   if (height > 1944) {
       height = 1944;       
   } else if (height < 20) {
       height = 20;
   }
   if (quality > 100) {
       quality = 100;
   } else if (quality < 0) {
       quality = 85; 
   }
   bcm_host_init();          
   default_status(&state);   
   state.width = width;
   state.height = height;
   state.quality = quality;
   state.encoding = encoding;
   state.videoEncode = 0;
   state.camera_parameters.exposureMode = parms->exposure;
   state.camera_parameters.exposureMeterMode = parms->meterMode;
   state.camera_parameters.awbMode = parms->awbMode;
   state.camera_parameters.imageEffect = parms->imageFX;   
   state.camera_parameters.ISO = parms->ISO;
   state.camera_parameters.sharpness = parms->sharpness;           
   state.camera_parameters.contrast = parms->contrast;              
   state.camera_parameters.brightness= parms->brightness;          
   state.camera_parameters.saturation = parms->saturation;           
   state.camera_parameters.videoStabilisation = parms->videoStabilisation;    /// 0 or 1 (false or true)
   state.camera_parameters.exposureCompensation = parms->exposureCompensation; 
   state.camera_parameters.rotation = parms->rotation;
   state.camera_parameters.hflip = parms->hflip;
   state.camera_parameters.vflip = parms->vflip;
      
   MMAL_COMPONENT_T *preview = 0;
   
   if ((status = create_video_camera_component(&state)) != MMAL_SUCCESS) {       
      vcos_log_error("%s: Failed to create camera component", __func__);
   } else if ((status = mmal_component_create("vc.null_sink", &preview)) != MMAL_SUCCESS)  {
      vcos_log_error("%s: Failed to create preview component", __func__);
      destroy_camera_component(&state);   
   } else if ((status = create_encoder_component(&state)) != MMAL_SUCCESS) {     
      vcos_log_error("%s: Failed to create encode component", __func__);      
      destroy_camera_component(&state);
   } else {       
      status = mmal_component_enable(preview);
      state.preview_component = preview;            
      PORT_USERDATA callback_data;    
      camera_preview_port = state.camera_component->output[MMAL_CAMERA_PREVIEW_PORT];
      camera_still_port   = state.camera_component->output[MMAL_CAMERA_CAPTURE_PORT];
      encoder_input_port  = state.encoder_component->input[0];
      encoder_output_port = state.encoder_component->output[0];
      preview_input_port  = state.preview_component->input[0];
      
      status = connect_ports(camera_preview_port, preview_input_port, &state.preview_connection);
    
      VCOS_STATUS_T vcos_status;
         

        // Now connect the camera to the encoder
      status = connect_ports(camera_still_port, encoder_input_port, &state.encoder_connection);      
      if (status != MMAL_SUCCESS) {
          vcos_log_error("%s: Failed to connect camera video port to encoder input", __func__);
          goto error;
      }
      
      // Set up our userdata - this is passed though to the callback where we need the information.
      // Null until we open our filename     
      callback_data.pstate = &state;      
      vcos_status = vcos_semaphore_create(&callback_data.complete_semaphore, "picam-sem", 0);
     
      vcos_assert(vcos_status == VCOS_SUCCESS);

      if (status != MMAL_SUCCESS) {
          vcos_log_error("Failed to setup encoder output");
          goto error;
      }
            
       
      int num, q;                                                
      // Enable the encoder output port
      encoder_output_port->userdata = (struct MMAL_PORT_USERDATA_T *)&callback_data;

      // Enable the encoder output port and tell it its callback function
      status = mmal_port_enable(encoder_output_port, encoder_buffer_callback);

      // Send all the buffers to the encoder output port
      num = mmal_queue_length(state.encoder_pool->queue);

      for (q=0;q<num;q++) {
         MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(state.encoder_pool->queue);

         if (!buffer)
            vcos_log_error("Unable to get a required buffer %d from pool queue", q);

         if (mmal_port_send_buffer(encoder_output_port, buffer)!= MMAL_SUCCESS)
            vcos_log_error("Unable to send a buffer to encoder output port (%d)", q);
      }

      

      if (mmal_port_parameter_set_boolean(camera_still_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS) {
         vcos_log_error("%s: Failed to start capture", __func__);
      } else {
         // Wait for capture to complete
         // For some reason using vcos_semaphore_wait_timeout sometimes returns immediately with bad parameter error
         // even though it appears to be all correct, so reverting to untimed one until figure out why its erratic
         vcos_semaphore_wait(&callback_data.complete_semaphore);                
      }                       
      // Disable encoder output port
      status = mmal_port_disable(encoder_output_port);
      
      *sizeread = state.bytesStored;
           
       vcos_semaphore_delete(&callback_data.complete_semaphore);      
    }
error:    
    mmal_status_to_int(status);
     
    // Disable all our ports that are not handled by connections     
    check_disable_port(encoder_output_port);  
    mmal_connection_destroy(state.encoder_connection);

    if (state.encoder_component)
        mmal_component_disable(state.encoder_component);
     
    if (state.preview_component) {
         mmal_component_disable(state.preview_component);        
         mmal_component_destroy(state.preview_component);
         state.preview_component = NULL;    
    }
    if (state.camera_component)
        mmal_component_disable(state.camera_component);
    
    
    destroy_encoder_component(&state);     
    destroy_camera_component(&state);
     

    if (status != MMAL_SUCCESS)
        raspicamcontrol_check_configuration(128);
    
    return state.filedata;
}

void internelVideoWithDetails(char *filename, int width, int height, int duration) {
   RASPISTILL_STATE state;   
   MMAL_STATUS_T status = MMAL_SUCCESS;   
   
   
   MMAL_PORT_T *camera_video_port = NULL;
   MMAL_PORT_T *encoder_input_port = NULL;
   MMAL_PORT_T *encoder_output_port = NULL; 
   FILE *output_file = NULL;
    
   if (width > 1920) {
       width = 1920;
   } else if (width < 20) {
       width = 20;
   }
   if (height > 1080) {
       height = 1080;       
   } else if (height < 20) {
       height = 20;
   }
   
   bcm_host_init();       

   default_status(&state);   
   state.width = width;
   state.height = height;
   state.quality = 0;
   state.videoEncode = 1;
   state.filename = filename;
   if ((status = create_video_camera_component(&state)) != MMAL_SUCCESS) {       
      vcos_log_error("%s: Failed to create camera component", __func__);
   } else if ((status = create_video_encoder_component(&state)) != MMAL_SUCCESS) {     
      vcos_log_error("%s: Failed to create encode component", __func__);      
      destroy_camera_component(&state);
   } else {       
      PORT_USERDATA callback_data;    
      camera_video_port   = state.camera_component->output[MMAL_CAMERA_VIDEO_PORT];
      encoder_input_port  = state.encoder_component->input[0];
      encoder_output_port = state.encoder_component->output[0];

        // Now connect the camera to the encoder
      status = connect_ports(camera_video_port, encoder_input_port, &state.encoder_connection);  
     
      if (status != MMAL_SUCCESS) {
          vcos_log_error("%s: Failed to connect camera video port to encoder input", __func__);
          goto error;
      }
      
      // Set up our userdata - this is passed though to the callback where we need the information.
      // Null until we open our filename     
      callback_data.pstate = &state;      
     

      if (status != MMAL_SUCCESS) {
          vcos_log_error("Failed to setup encoder output");
          goto error;
      }
      
      if (state.filename) {
          output_file = fopen(state.filename, "wb");
          callback_data.file_handle = output_file;
          callback_data.pstate = &state;
          callback_data.abort = 0;
      }

      encoder_output_port->userdata = (struct MMAL_PORT_USERDATA_T *)&callback_data;
                                                     
      int wait;

      // Enable the encoder output port and tell it its callback function
      status = mmal_port_enable(encoder_output_port, encoder_buffer_callback);
      if (mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS) {
          goto error;
      }
     // Send all the buffers to the encoder output port
     {
          int num = mmal_queue_length(state.encoder_pool->queue);
          int q;
          for (q=0;q<num;q++) {
             MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(state.encoder_pool->queue);

             if (!buffer)
                vcos_log_error("Unable to get a required buffer %d from pool queue", q);

             if (mmal_port_send_buffer(encoder_output_port, buffer)!= MMAL_SUCCESS)
                vcos_log_error("Unable to send a buffer to encoder output port (%d)", q);

          }
       }
       for (wait = 0; wait < duration; wait+= ABORT_INTERVAL)  {
          vcos_sleep(ABORT_INTERVAL);
          if (callback_data.abort)
             break;
       }
     
     
     
     
        
      // Disable encoder output port
      status = mmal_port_disable(encoder_output_port);
      
      
           
       vcos_semaphore_delete(&callback_data.complete_semaphore);      
    }
error:    
    mmal_status_to_int(status);
     
    // Disable all our ports that are not handled by connections 
    check_disable_port(encoder_output_port);  
    if (output_file && output_file != stdout)
         fclose(output_file);
      
    mmal_connection_destroy(state.encoder_connection);

    if (state.encoder_component)
        mmal_component_disable(state.encoder_component);
     

    if (state.camera_component)
        mmal_component_disable(state.camera_component);

    destroy_encoder_component(&state);     
    destroy_camera_component(&state);
     

    if (status != MMAL_SUCCESS)
        raspicamcontrol_check_configuration(128);
    
}