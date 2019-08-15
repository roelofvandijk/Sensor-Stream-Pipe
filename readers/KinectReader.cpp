//
// Created by amourao on 26-06-2019.
//

#include "KinectReader.h"

std::atomic_bool exiting(false);

KinectReader::KinectReader(uint8_t _device_index,
                           k4a_device_configuration_t *_device_config,
                           int32_t _absoluteExposureValue) {

    device_index = _device_index;
    device_config = _device_config;
    absoluteExposureValue = _absoluteExposureValue;

    streamId = randomString(16);

    const uint32_t installed_devices = k4a_device_get_installed_count();
    if (device_index >= installed_devices) {
        std::cerr << "Device not found." << std::endl;
        exit(1);
    }

    if (K4A_FAILED(k4a_device_open(device_index, &device))) {
        std::cerr << "Runtime error: k4a_device_open() failed " << std::endl;
    }

    char serial_number_buffer[256];
    size_t serial_number_buffer_size = sizeof(serial_number_buffer);
    CHECK(k4a_device_get_serialnum(device, serial_number_buffer, &serial_number_buffer_size), device);

    std::cout << "Device serial number: " << serial_number_buffer << std::endl;

    k4a_hardware_version_t version_info;
    CHECK(k4a_device_get_version(device, &version_info), device);

    std::cout << "Device version: " << (version_info.firmware_build == K4A_FIRMWARE_BUILD_RELEASE ? "Rel" : "Dbg")
              << "; C: " << version_info.rgb.major << "." << version_info.rgb.minor << "." << version_info.rgb.iteration
              << "; D: " << version_info.depth.major << "." << version_info.depth.minor << "."
              << version_info.depth.iteration << "[" << version_info.depth_sensor.major << "."
              << version_info.depth_sensor.minor << "]"
              << "; A: " << version_info.audio.major << "." << version_info.audio.minor << "."
              << version_info.audio.iteration << std::endl;

    uint32_t camera_fps = k4a_convert_fps_to_uint(device_config->camera_fps);

    if (camera_fps <= 0 || (device_config->color_resolution == K4A_COLOR_RESOLUTION_OFF &&
                            device_config->depth_mode == K4A_DEPTH_MODE_OFF)) {
        std::cerr << "Either the color or depth modes must be enabled to record." << std::endl;
        exit(1);
    }

    if (absoluteExposureValue != 0) {
        if (K4A_FAILED(k4a_device_set_color_control(device,
                                                    K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                                    K4A_COLOR_CONTROL_MODE_MANUAL,
                                                    absoluteExposureValue))) {
            std::cerr << "Runtime error: k4a_device_set_color_control() failed " << std::endl;
        }
    } else {
        if (K4A_FAILED(k4a_device_set_color_control(device,
                                                    K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                                    K4A_COLOR_CONTROL_MODE_AUTO,
                                                    0))) {
            std::cerr << "Runtime error: k4a_device_set_color_control() failed " << std::endl;
        }
    }

    CHECK(k4a_device_start_cameras(device, device_config), device);
    if (record_imu) {
        CHECK(k4a_device_start_imu(device), device);
    }

    std::cout << "Device started" << std::endl;

    // Wait for the first capture before starting recording.
    int32_t timeout_sec_for_first_capture = 60;
    if (device_config->wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE) {
        timeout_sec_for_first_capture = 360;
        std::cout << "[subordinate mode] Waiting for signal from master" << std::endl;
    }
    clock_t first_capture_start = clock();
    // Wait for the first capture in a loop so Ctrl-C will still exit.
    while (!exiting && (clock() - first_capture_start) < (CLOCKS_PER_SEC * timeout_sec_for_first_capture)) {
        result = k4a_device_get_capture(device, &capture, 100);
        if (result == K4A_WAIT_RESULT_SUCCEEDED) {
            k4a_capture_release(capture);
            break;
        } else if (result == K4A_WAIT_RESULT_FAILED) {
            std::cerr << "Runtime error: k4a_device_get_capture() returned error: " << result << std::endl;
            exit(1);
        }
    }

    if (exiting) {
        k4a_device_close(device);
        exit(0);
    } else if (result == K4A_WAIT_RESULT_TIMEOUT) {
        std::cerr << "Timed out waiting for first capture." << std::endl;
        exit(1);
    }

    recording_start = clock();
    timeout_ms = 1000 / camera_fps;

    frameTemplate.frameId = 0;
    frameTemplate.deviceId = 0;

    frameTemplate.messageType = 0;
    frameTemplate.sceneDesc = "kinect";
    frameTemplate.streamId = randomString(16);

    currentFrameCounter.push_back(0);
    currentFrameCounter.push_back(0);
    currentFrameCounter.push_back(0);


}

KinectReader::~KinectReader() {
    k4a_device_stop_cameras(device);

    CHECK(k4a_record_flush(recording), device);
    k4a_record_close(recording);

    std::cout << "Done" << std::endl;

    k4a_device_close(device);
}

void KinectReader::nextFrame() {
    currFrame.clear();

    do {
        result = k4a_device_get_capture(device, &capture, timeout_ms);
        if (result == K4A_WAIT_RESULT_TIMEOUT) {
            continue;
        } else if (result != K4A_WAIT_RESULT_SUCCEEDED) {
            std::cerr << "Runtime error: k4a_device_get_capture() returned " << result << std::endl;
            break;
        }
        k4a_image_t colorImage, depthImage, irImage;
        colorImage = k4a_capture_get_color_image(capture);
        depthImage = k4a_capture_get_depth_image(capture);
        irImage = k4a_capture_get_ir_image(capture);

        //std::cout << k4a_image_get_format(colorImage) << " " << k4a_image_get_format(depthImage) << " " << k4a_image_get_format(irImage) << " " << std::endl;

        if (k4a_image_get_format(colorImage) != 6) {
            FrameStruct s = frameTemplate;
            s.sensorId = 0;
            s.frameId = currentFrameCounter.at(0)++;
            uint8_t *buffer = k4a_image_get_buffer(colorImage);
            size_t size = k4a_image_get_size(colorImage);
            s.frame.resize(size);
            memcpy(reinterpret_cast<void *>(&s.frame[0]), buffer, size);
            // convert the raw buffer to cv::Mat
            int rows = k4a_image_get_height_pixels(colorImage);
            int cols = k4a_image_get_width_pixels(colorImage);
            //cv::Mat colorMat(rows, cols, CV_8UC4, (void *) buffer, cv::Mat::AUTO_STEP);
            //imencode(".png", colorMat, s.frame);

            currFrame.push_back(s);
        }

        if (k4a_image_get_format(depthImage) != 6) {
            FrameStruct s = frameTemplate;
            s.sensorId = 1;
            s.frameId = currentFrameCounter.at(1)++;
            uint8_t *buffer = k4a_image_get_buffer(depthImage);
            // convert the raw buffer to cv::Mat
            int rows = k4a_image_get_height_pixels(depthImage);
            int cols = k4a_image_get_width_pixels(depthImage);
            cv::Mat depthMat(rows, cols, CV_16UC1, (void *) buffer, cv::Mat::AUTO_STEP);
            imencode(".png", depthMat, s.frame);
            currFrame.push_back(s);
        }

        if (k4a_image_get_format(irImage) != 6) {
            FrameStruct s = frameTemplate;
            s.sensorId = 2;
            s.frameId = currentFrameCounter.at(2)++;
            uint8_t *buffer = k4a_image_get_buffer(irImage);
            // convert the raw buffer to cv::Mat
            int rows = k4a_image_get_height_pixels(irImage);
            int cols = k4a_image_get_width_pixels(irImage);
            cv::Mat irMat(rows, cols, CV_16UC1, (void *) buffer, cv::Mat::AUTO_STEP);
            imencode(".png", irMat, s.frame);
            currFrame.push_back(s);
        }


        k4a_image_release(colorImage);
        k4a_image_release(depthImage);
        k4a_image_release(irImage);
        k4a_capture_release(capture);
    } while (result == K4A_WAIT_RESULT_FAILED);


}

bool KinectReader::hasNextFrame() {
    return true;
}

void KinectReader::reset() {

}

std::vector<FrameStruct> KinectReader::currentFrame() {
    return currFrame;
}