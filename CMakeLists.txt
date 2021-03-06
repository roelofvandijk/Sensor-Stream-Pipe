cmake_minimum_required(VERSION 3.10.2)
project(davp-baseline)

include_directories("$ENV{HOME}/libs/include")
link_directories("$ENV{HOME}/libs/lib")

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" "$ENV{HOME}/libs/lib")
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/libs/lib")

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(FFmpeg REQUIRED)

if (FFMPEG_FOUND)
    #  FFMPEG_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
    #  FFMPEG_LIBRARIES     - Link these to use the required ffmpeg components.
    #  FFMPEG_DEFINITIONS   - Compiler switches required for using the required ffmpeg components.
    message("FFMPEG_INCLUDE_DIRS = ${FFMPEG_INCLUDE_DIRS} ")
    message("FFMPEG_LIBRARIES = ${FFMPEG_LIBRARIES} ")
    message("FFMPEG_DEFINITIONS = ${FFMPEG_DEFINITIONS} ")
endif ()

set(EXECUTABLE_OUTPUT_PATH bin)

option(SSP_WITH_KINECT_SUPPORT "This is settable from the command line" ON)
option(SSP_WITH_K4A_BODYTRACK "This is settable from the command line" ON)
option(SSP_WITH_NVPIPE_SUPPORT "This is settable from the command line" ON)
option(SSP_WITH_ZMQ_POLLING "This is settable from the command line" OFF)


if (SSP_WITH_NVPIPE_SUPPORT)
    add_definitions(-DSSP_WITH_NVPIPE_SUPPORT=1)
endif ()

if (SSP_WITH_KINECT_SUPPORT)
    add_definitions(-DSSP_WITH_KINECT_SUPPORT=1)
endif ()

if (SSP_WITH_K4A_BODYTRACK)
    add_definitions(-DSSP_WITH_K4A_BODYTRACK=1)
endif ()

if (SSP_WITH_ZMQ_POLLING)
    add_definitions(-DSSP_WITH_ZMQ_POLLING=1)
endif ()

set(SOURCE_FILES readers/image_reader.cc readers/multi_image_reader.cc structs/frame_struct.hpp utils/utils.cc utils/video_utils.cc utils/image_decoder.cc utils/image_converter.cc)
set(DECODER_SOURCE_FILES decoders/libav_decoder.cc decoders/zdepth_decoder.cc)
set(ENCODER_SOURCE_FILES encoders/zdepth_encoder.cc encoders/null_encoder.cc encoders/libav_encoder.cc)
set(READER_SOURCE_FILES readers/video_file_reader.cc readers/image_reader.cc)

set(KINECT_READER_SOURCE_FILES readers/kinect_reader.cc utils/kinect_utils.cc)
set(NVPIPE_ENCODER_SOURCE_FILES encoders/nv_encoder.cc)
set(NVPIPE_DECODER_SOURCE_FILES decoders/nv_decoder.cc)

if (SSP_WITH_NVPIPE_SUPPORT)
    set(DECODER_SOURCE_FILES ${DECODER_SOURCE_FILES} ${NVPIPE_DECODER_SOURCE_FILES})
    set(ENCODER_SOURCE_FILES ${ENCODER_SOURCE_FILES} ${NVPIPE_ENCODER_SOURCE_FILES})
endif ()

if (SSP_WITH_KINECT_SUPPORT)
    set(READER_SOURCE_FILES ${READER_SOURCE_FILES} readers/kinect_reader.cc utils/kinect_utils.cc)
    set(CLIENT_EXTRA_SOURCE_FILES ${CLIENT_EXTRA_SOURCE_FILES} utils/kinect_utils.cc)
endif ()

add_executable(ssp_client_opencv clients/ssp_client_opencv.cc readers/network_reader.cc ${DECODER_SOURCE_FILES} ${SOURCE_FILES} ${CLIENT_SOURCE_FILES})
add_executable(ssp_client_template clients/ssp_client_template.cc readers/network_reader.cc ${DECODER_SOURCE_FILES} ${SOURCE_FILES} ${CLIENT_SOURCE_FILES})
add_executable(ssp_server servers/ssp_server.cc ${READER_SOURCE_FILES} ${DECODER_SOURCE_FILES} ${ENCODER_SOURCE_FILES} ${SOURCE_FILES})
add_executable(ssp_tester testers/ssp_tester.cc utils/similarity_measures.cc ${READER_SOURCE_FILES} ${DECODER_SOURCE_FILES} ${ENCODER_SOURCE_FILES} ${SOURCE_FILES})

set(LIB_FILES zmq zdepth zstd yaml-cpp swscale)

set(NVPIPE_LIB_FILES NvPipe)
set(KINECT_LIB_FILES k4a k4arecord depthengine)
set(K4A_BODYTRACK_LIB_FILES k4abt)

target_link_libraries(ssp_client_opencv ${LIB_FILES})
target_link_libraries(ssp_client_opencv ${FFMPEG_LIBRARIES})
target_link_libraries(ssp_client_opencv ${OpenCV_LIBS})

target_link_libraries(ssp_client_template ${LIB_FILES})
target_link_libraries(ssp_client_template ${FFMPEG_LIBRARIES})
target_link_libraries(ssp_client_template ${OpenCV_LIBS})

target_link_libraries(ssp_server ${LIB_FILES})
target_link_libraries(ssp_server ${FFMPEG_LIBRARIES})
target_link_libraries(ssp_server ${OpenCV_LIBS})

target_link_libraries(ssp_tester ${LIB_FILES})
target_link_libraries(ssp_tester ${FFMPEG_LIBRARIES})
target_link_libraries(ssp_tester ${OpenCV_LIBS})

if (SSP_WITH_NVPIPE_SUPPORT)
    target_link_libraries(ssp_client_opencv ${NVPIPE_LIB_FILES})
    target_link_libraries(ssp_client_template ${NVPIPE_LIB_FILES})
    target_link_libraries(ssp_server ${NVPIPE_LIB_FILES})
    target_link_libraries(ssp_tester ${NVPIPE_LIB_FILES})
endif ()

if (SSP_WITH_KINECT_SUPPORT)
    target_link_libraries(ssp_server ${KINECT_LIB_FILES})
    target_link_libraries(ssp_tester ${KINECT_LIB_FILES})
endif ()

if (SSP_WITH_K4A_BODYTRACK)
    add_executable(ssp_client_k4a clients/ssp_client_k4a.cc utils/kinect_utils.cc readers/network_reader.cc ${DECODER_SOURCE_FILES} ${SOURCE_FILES} ${CLIENT_SOURCE_FILES})
    add_executable(ssp_client_pointcloud clients/ssp_client_pointcloud.cc utils/kinect_utils.cc readers/network_reader.cc ${DECODER_SOURCE_FILES} ${SOURCE_FILES} ${CLIENT_SOURCE_FILES})
    if (SSP_WITH_NVPIPE_SUPPORT)
        target_link_libraries(ssp_client_k4a ${NVPIPE_LIB_FILES})
        target_link_libraries(ssp_client_pointcloud ${NVPIPE_LIB_FILES})
    endif ()
    target_link_libraries(ssp_client_k4a ${LIB_FILES})
    target_link_libraries(ssp_client_k4a ${FFMPEG_LIBRARIES})
    target_link_libraries(ssp_client_k4a ${OpenCV_LIBS})
    target_link_libraries(ssp_client_k4a ${KINECT_LIB_FILES})
    target_link_libraries(ssp_client_k4a ${K4A_BODYTRACK_LIB_FILES})

    target_link_libraries(ssp_client_pointcloud ${LIB_FILES})
    target_link_libraries(ssp_client_pointcloud ${FFMPEG_LIBRARIES})
    target_link_libraries(ssp_client_pointcloud ${OpenCV_LIBS})
    target_link_libraries(ssp_client_pointcloud ${KINECT_LIB_FILES})
    target_link_libraries(ssp_client_pointcloud ${K4A_BODYTRACK_LIB_FILES})
endif ()
