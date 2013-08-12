cmake_minimum_required(VERSION 2.8)

SET(COMPILE_DEFINITIONS -Werror)


set(VCOS_PLATFORM pthreads)


add_definitions(-fno-strict-aliasing)

include_directories(${CMAKE_SOURCE_DIR}/host_applications/linux/libs/bcm_host/include)
include_directories( ../../../.. 
                     ../../../../interface/vcos
                     ../../../../interface/vcos/${VCOS_PLATFORM} )


FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})


ADD_LIBRARY(picam SHARED picammodule.c picam.c RaspiCamControl.c)
target_link_libraries(picam mmal_core mmal_util mmal_vc_client vcos bcm_host  ${PYTHON_LIBRARIES})
SET_TARGET_PROPERTIES(picam PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES picam)

#ADD_LIBRARY(picam SHARED picammodule.c picam.c RaspiCamControl.c encode.c )
#target_link_libraries(picam mmal_core mmal_util mmal_vc_client vcos bcm_host openmaxil /opt/vc/src/hello_pi/libs/ilclient/libilclient.a ${PYTHON_LIBRARIES})

set_target_properties(picam PROPERTIES PREFIX "_")
SET_TARGET_PROPERTIES(picam PROPERTIES LINKER_LANGUAGE C)

